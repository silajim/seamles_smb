/*
Dokan : user-mode file system library for Windows

Copyright (C) 2020 - 2021 Google, Inc.
Copyright (C) 2015 - 2019 Adrien J. <liryna.stark@gmail.com> and Maxime C. <maxime@islog.com>
Copyright (C) 2007 - 2011 Hiroki Asakawa <info@dokan-dev.net>

http://dokan-dev.github.io

    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
                                                                          in the Software without restriction, including without limitation the rights
                                                                          to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
                                                                          copies of the Software, and to permit persons to whom the Software is
                                                                          furnished to do so, subject to the following conditions:

                                                                          The above copyright notice and this permission notice shall be included in
                                                                          all copies or substantial portions of the Software.

                                                                          THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
                                                                          IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
                                                                          FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
                                                                          AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
                                                                          LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
                                                                          OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
                                                                          THE SOFTWARE.

*/

#include "include/dokan/dokan.h"
#include "include/dokan/fileinfo.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <map>
#include <unordered_map>
#include "filenode.h"

#include <consoleapi.h>
#include <handleapi.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#include <Mq.h>

#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "DbgPrint.h"
#include "globals.h"

#include <ostream>
#include <algorithm>

#include "securityprocessor.h"
#include "WinSec.h"
#include "filesecurity.h"
#include "globals.h"
#include "nodes.h"
#include "fileops.h"
#include "Context.h"

/// https://localcoder.org/using-a-c-class-member-function-as-a-c-callback-function
/// Hack to map c++ member functions to C callbacks
/*
 * #include <stdio.h>
#include <functional>

template <typename T>
struct Callback;

template <typename Ret, typename... Params>
struct Callback<Ret(Params...)> {
   template <typename... Args>
   static Ret callback(Args... args) {
      return func(args...);
   }
   static std::function<Ret(Params...)> func;
};

template <typename Ret, typename... Params>
std::function<Ret(Params...)> Callback<Ret(Params...)>::func;

void register_with_library(int (*func)(int *k, int *e)) {
   int x = 0, y = 1;
   int o = func(&x, &y);
   printf("Value: %i\n", o);
}

class A {
   public:
      A();
      ~A();
      int e(int *k, int *j);
};

typedef int (*callback_t)(int*,int*);

A::A() {
   Callback<int(int*,int*)>::func = std::bind(&A::e, this, std::placeholders::_1, std::placeholders::_2);
   callback_t func = static_cast<callback_t>(Callback<int(int*,int*)>::callback);
   register_with_library(func);
}

int A::e(int *k, int *j) {
   return *k - *j;
}

A::~A() { }

int main() {
   A a;
}

*/

#pragma warning(push)
#pragma warning(disable : 4305)

/// \brief This class is derives from basic_stringbuf which will output
/// all the written data using the OutputDebugString function
template<typename TChar, typename TTraits = std::char_traits<TChar>>
class OutputDebugStringBuf : public std::basic_stringbuf<TChar,TTraits> {
public:
    explicit OutputDebugStringBuf() : _buffer(256) {
        setg(nullptr, nullptr, nullptr);
        setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
    }

    ~OutputDebugStringBuf() {
    }

    static_assert(std::is_same<TChar,char>::value ||
    std::is_same<TChar,wchar_t>::value,
    "OutputDebugStringBuf only supports char and wchar_t types");

    int sync() try {
        MessageOutputer<TChar,TTraits>()(pbase(), pptr());
        setp(_buffer.data(), _buffer.data(), _buffer.data() + _buffer.size());
        return 0;
    }
    catch(...) {
    return -1;
}

int_type overflow(int_type c = TTraits::eof()) {
    auto syncRet = sync();
    if (c != TTraits::eof()) {
        _buffer[0] = c;
        setp(_buffer.data(), _buffer.data() + 1, _buffer.data() + _buffer.size());
    }
    return syncRet == -1 ? TTraits::eof() : 0;
}


private:
std::vector<TChar> _buffer;

template<typename TChar, typename TTraits>
struct MessageOutputer;

template<>
struct MessageOutputer<char,std::char_traits<char>> {
    template<typename TIterator>
    void operator()(TIterator begin, TIterator end) const {
        std::string s(begin, end);
        OutputDebugStringA(s.c_str());
    }
};

template<>
struct MessageOutputer<wchar_t,std::char_traits<wchar_t>> {
    template<typename TIterator>
    void operator()(TIterator begin, TIterator end) const {
        std::wstring s(begin, end);
        OutputDebugStringW(s.c_str());
    }
};
};

static BOOL AddSeSecurityNamePrivilege(std::shared_ptr<DbgPrint> print) {
    HANDLE token = 0;
    print->print(L"## Attempting to add SE_SECURITY_NAME privilege to process token ##\n");
    DWORD err;
    LUID luid;
    if (!LookupPrivilegeValue(0, SE_SECURITY_NAME, &luid)) {
        err = GetLastError();
        if (err != ERROR_SUCCESS) {
            print->print(L"  failed: Unable to lookup privilege value. error = %u\n",
                         err);
            return FALSE;
        }
    }

    LUID_AND_ATTRIBUTES attr;
    attr.Attributes = SE_PRIVILEGE_ENABLED;
    attr.Luid = luid;

    TOKEN_PRIVILEGES priv;
    priv.PrivilegeCount = 1;
    priv.Privileges[0] = attr;

    if (!OpenProcessToken(GetCurrentProcess(),
                          TOKEN_ADJUST_PRIVILEGES | TOKEN_QUERY, &token)) {
        err = GetLastError();
        if (err != ERROR_SUCCESS) {
            print->print(L"  failed: Unable obtain process token. error = %u\n", err);
            return FALSE;
        }
    }

    TOKEN_PRIVILEGES oldPriv;
    DWORD retSize;
    AdjustTokenPrivileges(token, FALSE, &priv, sizeof(TOKEN_PRIVILEGES), &oldPriv,
                          &retSize);
    err = GetLastError();
    if (err != ERROR_SUCCESS) {
        print->print(L"  failed: Unable to adjust token privileges: %u\n", err);
        CloseHandle(token);
        return FALSE;
    }

    BOOL privAlreadyPresent = FALSE;
    for (unsigned int i = 0; i < oldPriv.PrivilegeCount; i++) {
        if (oldPriv.Privileges[i].Luid.HighPart == luid.HighPart &&
                oldPriv.Privileges[i].Luid.LowPart == luid.LowPart) {
            privAlreadyPresent = TRUE;
            break;
        }
    }
    print->print(privAlreadyPresent ? L"  success: privilege already present\n": L"  success: privilege added\n");
    if (token)
        CloseHandle(token);
    return TRUE;
}



#pragma warning(pop)

std::shared_ptr<Globals> globals;
BOOL WINAPI CtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        SetConsoleCtrlHandler(CtrlHandler, FALSE);
        DokanRemoveMountPoint(globals->MountPoint().c_str());
        return TRUE;
    default:
        return FALSE;
    }
}

void ShowUsage() {
    // clang-format off
    fprintf(stderr, "mirror.exe - Mirror a local device or folder to secondary device, an NTFS folder or a network device.\n"
                    "  /r RootDirectory (ex. /r c:\\test)\t\t Directory source to mirror.\n"
                    "  /l MountPoint (ex. /l m)\t\t\t Mount point. Can be M:\\ (drive letter) or empty NTFS folder C:\\mount\\dokan .\n"
                    "  /t ThreadCount (ex. /t 5)\t\t\t Number of threads to be used internally by Dokan library.\n\t\t\t\t\t\t More threads will handle more event at the same time.\n"
                    "  /d (enable debug output)\t\t\t Enable debug output to an attached debugger.\n"
                    "  /s (use stderr for output)\t\t\t Enable debug output to stderr.\n"
                    "  /n (use network drive)\t\t\t Show device as network device.\n"
                    "  /m (use removable drive)\t\t\t Show device as removable media.\n"
                    "  /w (write-protect drive)\t\t\t Read only filesystem.\n"
                    "  /b (case sensitive drive)\t\t\t Supports case-sensitive file names.\n"
                    "  /o (use mount manager)\t\t\t Register device to Windows mount manager.\n\t\t\t\t\t\t This enables advanced Windows features like recycle bin and more...\n"
                    "  /c (mount for current session only)\t\t Device only visible for current user session.\n"
                    "  /u (UNC provider name ex. \\localhost\\myfs)\t UNC name used for network volume.\n"
                    "  /p (Impersonate Caller User)\t\t\t Impersonate Caller User when getting the handle in CreateFile for operations.\n\t\t\t\t\t\t This option requires administrator right to work properly.\n"
                    "  /a Allocation unit size (ex. /a 512)\t\t Allocation Unit Size of the volume. This will behave on the disk file size.\n"
                    "  /k Sector size (ex. /k 512)\t\t\t Sector Size of the volume. This will behave on the disk file size.\n"
                    "  /f User mode Lock\t\t\t\t Enable Lockfile/Unlockfile operations. Otherwise Dokan will take care of it.\n"
                    "  /i Timeout in Milliseconds (ex. /i 30000)\t Timeout until a running operation is aborted and the device is unmounted.\n"
                    "  /z Enabled FCB GCt\t\t\t\t Might speed up on env with filter drivers (Anti-virus) slowing down the system.\n"
                    "  /x Network unmount\t\t\t\t Allows unmounting network drive from file explorer.\n"
                    "  /e Enable Driver Logs\t\t\t\t Forward Driver logs to userland.\n\n"
                    "  /v Volume name\t\t\t\t Personalize the volume name.\n\n"
                    "Examples:\n"
                    "\tmirror.exe /r C:\\Users /l M:\t\t\t# Mirror C:\\Users as RootDirectory into a drive of letter M:\\.\n"
                    "\tmirror.exe /r C:\\Users /l C:\\mount\\dokan\t# Mirror C:\\Users as RootDirectory into NTFS folder C:\\mount\\dokan.\n"
                    "\tmirror.exe /r C:\\Users /l M: /n /u \\myfs\\myfs1\t# Mirror C:\\Users as RootDirectory into a network drive M:\\. with UNC \\\\myfs\\myfs1\n\n"
                    "Unmount the drive with CTRL + C in the console or alternatively via \"dokanctl /u MountPoint\".\n");
    // clang-format on
}

#define CHECK_CMD_ARG(commad, argc)                                            \
{                                                                            \
    if (++command == argc) {                                                   \
    fwprintf(stderr, L"Option is missing an argument.\n");                   \
    return EXIT_FAILURE;                                                     \
    }                                                                          \
    }

int __cdecl wmain(ULONG argc, PWCHAR argv[]) {
    int status;
    ULONG command;
    DOKAN_OPERATIONS dokanOperations;
    DOKAN_OPTIONS dokanOptions;
    DOKAN_OPTIONS dokanOptions2;





    if (argc < 3) {
        ShowUsage();
        return EXIT_FAILURE;
    }



    ZeroMemory(&dokanOptions, sizeof(DOKAN_OPTIONS));
    ZeroMemory(&dokanOptions2, sizeof(DOKAN_OPTIONS));
    dokanOptions.Version = DOKAN_VERSION;
    //    dokanOptions.ThreadCount = 0; // use default

    OutputDebugStringBuf<wchar_t> wcharDebugOutput;
    std::wcerr.rdbuf(&wcharDebugOutput);
    std::wclog.rdbuf(&wcharDebugOutput);
    std::wcout.rdbuf(&wcharDebugOutput);


    bool g_DebugMode=false , g_UseStdErr=false;
    globals = std::make_shared<Globals>();


    for (command = 1; command < argc; command++) {
        switch (towlower(argv[command][1])) {
        case L'r':
            CHECK_CMD_ARG(command, argc)
                    globals->setRootDirectory(argv[command]);
            if (globals->RootDirectory().size()==0) {
                std::wcerr << "Invalid RootDirectory" << std::endl;
                return EXIT_FAILURE;
            }

            std::wcerr << "RootDirectory: " << globals->RootDirectory() << std::endl;
            break;
        case L'l':
            CHECK_CMD_ARG(command, argc)
                    globals->setMountPoint(argv[command]);
            dokanOptions.MountPoint = globals->MountPoint().data();
            break;
        case L't':
            dokanOptions.SingleThread = TRUE;
            break;
        case L'd':
            g_DebugMode = true;
            break;
        case L's':
            g_UseStdErr = true;
            break;
        case L'n':
            dokanOptions.Options |= DOKAN_OPTION_NETWORK;
            break;
        case L'm':
            dokanOptions.Options |= DOKAN_OPTION_REMOVABLE;
            break;
        case L'w':
            dokanOptions.Options |= DOKAN_OPTION_WRITE_PROTECT;
            break;
        case L'o':
            dokanOptions.Options |= DOKAN_OPTION_MOUNT_MANAGER;
            break;
        case L'c':
            dokanOptions.Options |= DOKAN_OPTION_CURRENT_SESSION;
            break;
        case L'f':
            dokanOptions.Options |= DOKAN_OPTION_FILELOCK_USER_MODE;
            break;
        case L'x':
            dokanOptions.Options |= DOKAN_OPTION_ENABLE_UNMOUNT_NETWORK_DRIVE;
            break;
        case L'e':
            dokanOptions.Options |= DOKAN_OPTION_DISPATCH_DRIVER_LOGS;
            break;
        case L'b':
            // Only work when mirroring a folder with setCaseSensitiveInfo option enabled on win10
            dokanOptions.Options |= DOKAN_OPTION_CASE_SENSITIVE;
            globals->setCaseSensitive(true);
            break;
        case L'u':
            CHECK_CMD_ARG(command, argc)
                    //                    wcscpy_s(UNCName, sizeof(UNCName) / sizeof(WCHAR), argv[command]);
                    globals->setUNCName(std::wstring(argv[command]));
            dokanOptions.UNCName = globals->UNCName().c_str();
            std::wcerr << L"UNC Name: "<< globals->UNCName() << std::endl;
            //            m_print->print(L"UNC Name: %ls\n", UNCName);
            break;
        case L'p':
            globals->setImpersonateCallerUser(true);
            break;
        case L'i':
            CHECK_CMD_ARG(command, argc)
                    dokanOptions.Timeout = (ULONG)_wtol(argv[command]);
            break;
        case L'a':
            CHECK_CMD_ARG(command, argc)
                    dokanOptions.AllocationUnitSize = (ULONG)_wtol(argv[command]);
            break;
        case L'k':
            CHECK_CMD_ARG(command, argc)
                    dokanOptions.SectorSize = (ULONG)_wtol(argv[command]);
            break;
        case L'v':
            CHECK_CMD_ARG(command, argc)
                    //              wcscpy_s(gVolumeName, sizeof(gVolumeName) / sizeof(WCHAR), argv[command]);
                    globals->setVolname(argv[command]);
            std::wcerr << L"Volume Name:  "<<  globals->volname();
            //              m_print->print(L"Volume Name: %ls\n", gVolumeName);
            break;
        default:
            fwprintf(stderr, L"unknown command: %ls\n", argv[command]);
            return EXIT_FAILURE;
        }
    }

    if (globals->UNCName().size() != 0 && !(dokanOptions.Options & DOKAN_OPTION_NETWORK)) {
        fwprintf(
                    stderr,
                    L"  Warning: UNC provider name should be set on network drive only.\n");
    }

    if (dokanOptions.Options & DOKAN_OPTION_NETWORK &&  dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) {
        fwprintf(stderr, L"Mount manager cannot be used on network drive.\n");
        return EXIT_FAILURE;
    }

    if (!(dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) && (globals->MountPoint().size() == 0)) {
        fwprintf(stderr, L"Mount Point required.\n");
        return EXIT_FAILURE;
    }

    if ((dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) && (dokanOptions.Options & DOKAN_OPTION_CURRENT_SESSION)) {
        fwprintf(stderr,
                 L"Mount Manager always mount the drive for all user sessions.\n");
        return EXIT_FAILURE;
    }

    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        fwprintf(stderr, L"Control Handler is not set.\n");
    }

    std::shared_ptr<DbgPrint> dbgp = std::make_shared<DbgPrint>(g_UseStdErr,g_DebugMode);

    // Add security name privilege. Required here to handle GetFileSecurity
    // properly.
    globals->setHasSeSecurityPrivilege(AddSeSecurityNamePrivilege(dbgp));
    if (!globals->HasSeSecurityPrivilege()) {
        fwprintf(stderr,
                 L"[Mirror] Failed to add security privilege to process\n"
                 L"\t=> GetFileSecurity/SetFileSecurity may not work properly\n"
                 L"\t=> Please restart mirror sample with administrator rights to fix it\n");
    }

    if (globals->ImpersonateCallerUser() && !globals->HasSeSecurityPrivilege()) {
        fwprintf(
                    stderr,
                    L"[Mirror] Impersonate Caller User requires administrator right to work properly\n"
                    L"\t=> Other users may not use the drive properly\n"
                    L"\t=> Please restart mirror sample with administrator rights to fix it\n");
    }

    if (g_DebugMode) {
        dokanOptions.Options |= DOKAN_OPTION_DEBUG;
    }
    if (g_UseStdErr) {
        dokanOptions.Options |= DOKAN_OPTION_STDERR;
    }

    dokanOptions.Options |= DOKAN_OPTION_ALT_STREAM;

    std::shared_ptr<Nodes> m_nodes = std::make_shared<Nodes>();
    std::shared_ptr<WinSec> winsec = std::make_shared<WinSec>(dbgp);
    std::shared_ptr<Context> context = std::make_shared<Context>();
    context->globals = globals;
    context->print = dbgp;
    context->nodes = m_nodes;
    context->winsec = winsec;

    PSECURITY_DESCRIPTOR sd;
    winsec->CreateDefaultSelfRelativeSD(&sd);
    LocalFree(sd);

    std::shared_ptr<FileOps> fops = std::make_shared<FileOps>(m_nodes,dbgp,globals);



    ZeroMemory(&dokanOperations, sizeof(DOKAN_OPERATIONS));
    dokanOperations.ZwCreateFile = &FileOps::MirrorCreateFile;
    dokanOperations.Cleanup = &FileOps::MirrorCleanup;
    dokanOperations.CloseFile = &FileOps::MirrorCloseFile;
    dokanOperations.ReadFile = &FileOps::MirrorReadFile;
    dokanOperations.WriteFile = &FileOps::MirrorWriteFile;
    dokanOperations.FlushFileBuffers = &FileOps::MirrorFlushFileBuffers;
    dokanOperations.GetFileInformation = &FileOps::MirrorGetFileInformation;
    dokanOperations.FindFiles = &FileOps::MirrorFindFiles;
    dokanOperations.FindFilesWithPattern = NULL;
    dokanOperations.SetFileAttributes = &FileOps::MirrorSetFileAttributes;
    dokanOperations.SetFileTime = &FileOps::MirrorSetFileTime;
    dokanOperations.DeleteFile = &FileOps::MirrorDeleteFile;
    dokanOperations.DeleteDirectory = &FileOps::DeleteDirectory;
    dokanOperations.MoveFile = &FileOps::MirrorMoveFile;
    dokanOperations.SetEndOfFile = &FileOps::MirrorSetEndOfFile;
    dokanOperations.SetAllocationSize = &FileOps::MirrorSetAllocationSize;
    dokanOperations.LockFile = &FileOps::MirrorLockFile;
    dokanOperations.UnlockFile = &FileOps::MirrorUnlockFile;
    dokanOperations.GetFileSecurity = &FileOps::MirrorGetFileSecurity;
    dokanOperations.SetFileSecurity = &FileOps::MirrorSetFileSecurity;
    dokanOperations.GetDiskFreeSpace = &FileOps::MirrorDokanGetDiskFreeSpace;
    dokanOperations.GetVolumeInformation = &FileOps::MirrorGetVolumeInformation;
    dokanOperations.Unmounted = &FileOps::MirrorUnmounted;
    dokanOperations.FindStreams = &FileOps::MirrorFindStreams;
    dokanOperations.Mounted = &FileOps::MirrorMounted;

    //    std::shared_ptr<Context> m_nodes;
//    m_nodes->_filenodes = std::make_shared<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>();

    dokanOptions.GlobalContext = reinterpret_cast<ULONG64>(context.get());

    std::wstring rootdir = globals->RootDirectory();
    std::replace(rootdir.begin(),rootdir.end(),L'\\',L'_');

    if(boost::filesystem::exists(rootdir+L".dat")){

        std::filebuf filer;
        filer.open(rootdir+L".dat",std::ios_base::in|std::ios_base::binary);
        std::istream is(&filer);
        boost::archive::binary_iarchive ir(is, boost::archive::no_header);

        ir >> m_nodes;
    }

    DokanInit();
    //    status = DokanMain(&dokanOptions, &dokanOperations);
    DOKAN_HANDLE handle1;
    status = DokanCreateFileSystem(&dokanOptions, &dokanOperations,&handle1);
    if(status == DOKAN_SUCCESS){
        //       dokanOptions2 = dokanOptions;

        //       std::shared_ptr<Globals> globals2 = std::make_shared<Globals>(globals);
        //       std::shared_ptr<DbgPrint> dbgp2 = std::make_shared<DbgPrint>(g_UseStdErr,g_DebugMode);
        //       std::shared_ptr<Nodes> m_nodes2 = std::make_shared<Nodes>();
        //       std::shared_ptr<WinSec> winsec2 = std::make_shared<WinSec>(dbgp);
        //       std::shared_ptr<Context> context2 = std::make_shared<Context>();
        //       context2->globals = globals2;
        //       context2->print = dbgp2;
        //       context2->nodes = m_nodes2;
        //       context2->winsec = winsec2;

        //       globals2->setRootDirectory(LR"(\\192.168.1.4\temp2)");
        //       globals2->setMountPoint(L"W:");
        //       globals2->setVolname(L"temp2");

        //       dokanOptions2.MountPoint = globals2->MountPoint().data();
        //       dokanOptions2.GlobalContext = reinterpret_cast<ULONG64>(context2.get());

        //       DOKAN_HANDLE handle2;
        //       status = DokanCreateFileSystem(&dokanOptions2, &dokanOperations,&handle2);

        //       if(status == DOKAN_SUCCESS){
        //          DokanWaitForFileSystemClosed(handle2,INFINITE);
        //       }


        DokanWaitForFileSystemClosed(handle1,INFINITE);
    }



    DokanShutdown();
    switch (status) {
    case DOKAN_SUCCESS:
        fprintf(stderr, "Success\n");
        break;
    case DOKAN_ERROR:
        fprintf(stderr, "Error\n");
        break;
    case DOKAN_DRIVE_LETTER_ERROR:
        fprintf(stderr, "Bad Drive letter\n");
        break;
    case DOKAN_DRIVER_INSTALL_ERROR:
        fprintf(stderr, "Can't install driver\n");
        break;
    case DOKAN_START_ERROR:
        fprintf(stderr, "Driver something wrong\n");
        break;
    case DOKAN_MOUNT_ERROR:
        fprintf(stderr, "Can't assign a drive letter\n");
        break;
    case DOKAN_MOUNT_POINT_ERROR:
        fprintf(stderr, "Mount point error\n");
        break;
    case DOKAN_VERSION_ERROR:
        fprintf(stderr, "Version error\n");
        break;
    default:
        fprintf(stderr, "Unknown error: %d\n", status);
        break;
    }

    //    std::wstring rootdir (RootDirectory);
    //    std::replace(rootdir.begin(),rootdir.end(),L'\\',L'_');
    std::filebuf file;
    file.open(rootdir+L".dat",std::ios_base::out|std::ios_base::binary|std::ios_base::trunc);
    std::ostream os(&file);
    boost::archive::binary_oarchive ar(os, boost::archive::no_header);

    ar << m_nodes;

    file.close();

    //    std::cout << "Directories Cached" << std::endl;
    //    for(auto d : directory){
    //        std::wcerr << d.first;
    //    }

    //    std::cout << "Files Cached" << std::endl;
    //    for(auto f : *_filenodes.get()){
    //        std::wcerr << f.first;
    //    }
    return EXIT_SUCCESS;
}
