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

#include <handleapi.h>
#include <processthreadsapi.h>
#include <Psapi.h>

#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/text_iarchive.hpp>
#include <boost/archive/text_oarchive.hpp>
#include "DbgPrint.h"

// Enable Long Paths on Windows 10 version 1607 and later by changing
// the OS configuration (see Microsoft own documentation for the steps)
// and rebuild the mirror with the following line uncommented.
#define WIN10_ENABLE_LONG_PATH
#ifdef WIN10_ENABLE_LONG_PATH
//dirty but should be enough
#define DOKAN_MAX_PATH 32768
#else
#define DOKAN_MAX_PATH MAX_PATH
#endif // DEBUG



BOOL g_UseStdErr;
BOOL g_DebugMode;
BOOL g_CaseSensitive;
BOOL g_HasSeSecurityPrivilege;
BOOL g_ImpersonateCallerUser;
static WCHAR gVolumeName[MAX_PATH + 1] = L"DOKAN";

//std::map<std::wstring,std::wstring> directory;
//std::map<std::wstring,std::wstring> files;

// Global map of filename / filenode for all the filesystem.
//std::unique_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>> _filenodes;
//std::mutex m_mutex;

struct Nodes{
    std::unique_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>> _filenodes;
    std::mutex m_mutex;
};

// Directory map of directoryname / sub filenodes in the scope.
// A directory \foo with 2 files bar and coco will have one entry:
// first: foo - second: set filenode { bar, coco }
//std::unordered_map<std::wstring, std::set<std::shared_ptr<filenode>>> _directoryPaths;



// Helper getting the memfs filenodes context at each Dokan API call.
#define GET_FS_INSTANCE \
  reinterpret_cast<std::unordered_map<std::wstring, std::shared_ptr<filenode>>*>(DokanFileInfo->DokanOptions->GlobalContext)

//static void DbgPrint(LPCWSTR format, ...) {
//    if (g_DebugMode) {
//        const WCHAR *outputString;
//        WCHAR *buffer = NULL;
//        size_t length;
//        va_list argp;

//        va_start(argp, format);
//        length = _vscwprintf(format, argp) + 1;
//        buffer = (WCHAR*)_malloca(length * sizeof(WCHAR));
//        if (buffer) {
//            vswprintf_s(buffer, length, format, argp);
//            outputString = buffer;
//        } else {
//            outputString = format;
//        }
//        if (g_UseStdErr)
//            fputws(outputString, stderr);
//        else
//            OutputDebugStringW(outputString);
//        if (buffer)
//            _freea(buffer);
//        va_end(argp);
//        if (g_UseStdErr)
//            fflush(stderr);
//    }
//}

static WCHAR RootDirectory[DOKAN_MAX_PATH] = L"C:";
static WCHAR MountPoint[DOKAN_MAX_PATH] = L"M:\\";
static WCHAR UNCName[DOKAN_MAX_PATH] = L"";

static void GetFilePath(PWCHAR filePath, ULONG numberOfElements, LPCWSTR FileName) {
    wcsncpy_s(filePath, numberOfElements, RootDirectory, wcslen(RootDirectory));
    size_t unclen = wcslen(UNCName);
    if (unclen > 0 && _wcsnicmp(FileName, UNCName, unclen) == 0) {
        if (_wcsnicmp(FileName + unclen, L".", 1) != 0) {
            wcsncat_s(filePath, numberOfElements, FileName + unclen,
                      wcslen(FileName) - unclen);
        }
    } else {
        wcsncat_s(filePath, numberOfElements, FileName, wcslen(FileName));
    }
}

static void PrintUserName(PDOKAN_FILE_INFO DokanFileInfo) {
    HANDLE handle;
    UCHAR buffer[1024];
    DWORD returnLength;
    WCHAR accountName[256];
    WCHAR domainName[256];
    DWORD accountLength = sizeof(accountName) / sizeof(WCHAR);
    DWORD domainLength = sizeof(domainName) / sizeof(WCHAR);
    PTOKEN_USER tokenUser;
    SID_NAME_USE snu;

    if (!g_DebugMode)
        return;

    handle = DokanOpenRequestorToken(DokanFileInfo);
    if (handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"  DokanOpenRequestorToken failed\n");
        return;
    }

    if (!GetTokenInformation(handle, TokenUser, buffer, sizeof(buffer), &returnLength)) {
        DbgPrint(L"  GetTokenInformaiton failed: %d\n", GetLastError());
        CloseHandle(handle);
        return;
    }

    CloseHandle(handle);

    tokenUser = (PTOKEN_USER)buffer;
    if (!LookupAccountSid(NULL, tokenUser->User.Sid, accountName, &accountLength, domainName, &domainLength, &snu)) {
        DbgPrint(L"  LookupAccountSid failed: %d\n", GetLastError());
        return;
    }

    DbgPrint(L"  AccountName: %s, DomainName: %s\n", accountName, domainName);
}

static BOOL AddSeSecurityNamePrivilege() {
    HANDLE token = 0;
    DbgPrint(
                L"## Attempting to add SE_SECURITY_NAME privilege to process token ##\n");
    DWORD err;
    LUID luid;
    if (!LookupPrivilegeValue(0, SE_SECURITY_NAME, &luid)) {
        err = GetLastError();
        if (err != ERROR_SUCCESS) {
            DbgPrint(L"  failed: Unable to lookup privilege value. error = %u\n",
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
            DbgPrint(L"  failed: Unable obtain process token. error = %u\n", err);
            return FALSE;
        }
    }

    TOKEN_PRIVILEGES oldPriv;
    DWORD retSize;
    AdjustTokenPrivileges(token, FALSE, &priv, sizeof(TOKEN_PRIVILEGES), &oldPriv,
                          &retSize);
    err = GetLastError();
    if (err != ERROR_SUCCESS) {
        DbgPrint(L"  failed: Unable to adjust token privileges: %u\n", err);
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
    DbgPrint(privAlreadyPresent ? L"  success: privilege already present\n"
                                : L"  success: privilege added\n");
    if (token)
        CloseHandle(token);
    return TRUE;
}

#define MirrorCheckFlag(val, flag)                                             \
    if (val & flag) {                                                            \
    DbgPrint(L"\t" L#flag L"\n");                                              \
    }

static NTSTATUS DOKAN_CALLBACK
MirrorCreateFile2(LPCWSTR FileName, PDOKAN_IO_SECURITY_CONTEXT SecurityContext, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    DWORD fileAttr;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD creationDisposition;
    DWORD fileAttributesAndFlags;
    DWORD error = 0;
    SECURITY_ATTRIBUTES securityAttrib;
    ACCESS_MASK genericDesiredAccess;
    // userTokenHandle is for Impersonate Caller User Option
    HANDLE userTokenHandle = INVALID_HANDLE_VALUE;
    auto filenodes = GET_FS_INSTANCE;

    securityAttrib.nLength = sizeof(securityAttrib);
    securityAttrib.lpSecurityDescriptor = SecurityContext->AccessState.SecurityDescriptor;
    securityAttrib.bInheritHandle = FALSE;

    std::wcout << "Create File, FILENAME " << std::wstring(FileName) << std::endl;

}

static NTSTATUS DOKAN_CALLBACK
MirrorCreateFile(LPCWSTR FileName, PDOKAN_IO_SECURITY_CONTEXT SecurityContext, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle=NULL;
    DWORD fileAttr;
    NTSTATUS status = STATUS_SUCCESS;
    DWORD creationDisposition;
    DWORD fileAttributesAndFlags;
    DWORD error = 0;
    SECURITY_ATTRIBUTES securityAttrib;
    ACCESS_MASK genericDesiredAccess;
    // userTokenHandle is for Impersonate Caller User Option
    HANDLE userTokenHandle = INVALID_HANDLE_VALUE;
    auto filenodes = GET_FS_INSTANCE;

    securityAttrib.nLength = sizeof(securityAttrib);
    securityAttrib.lpSecurityDescriptor = SecurityContext->AccessState.SecurityDescriptor;
    securityAttrib.bInheritHandle = FALSE;

//    std::wcout << "Create File, FILENAME " << std::wstring(FileName) << std::endl;

    DokanMapKernelToUserCreateFileFlags(DesiredAccess, FileAttributes, CreateOptions, CreateDisposition,&genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);    

    std::wstring filename_str(FileName);


//    if(filename_str.empty()|| filename_str==LR"(\)"){
////        return STATUS_ACCESS_DENIED;
//        std::cout<< "---Empty Filename!" << std::endl;
//        HANDLE ph = OpenProcess(PROCESS_QUERY_INFORMATION , FALSE , DokanFileInfo->ProcessId);
//        if(ph!=NULL){
//            WCHAR str[128];
//            DWORD length =  GetProcessImageFileNameW(ph,str,128);
//            if(length>0){
//                std::wcout << L"\t process path " << std::wstring(str) << std::endl;
//            }
//        }
//    }

//    if (filename_str == L"\\System Volume Information" || filename_str == L"\\$RECYCLE.BIN") {
//      return STATUS_NO_SUCH_FILE;
//    }


//    if(filename_str==LR"(\test.txt)" || filename_str==LR"(\***REMOVED***)"){
//        std::cout << "TEST detected" << std::endl;
//        SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;
//        LPWSTR strAttrib;
//        LPTSTR tst;
//        ULONG size;
//        std::cout <<"Convert Result " << ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityContext->AccessState.SecurityDescriptor,SDDL_REVISION_1, secInfo , &tst , &size ) << std::endl;
//        DWORD error = GetLastError();
//        std::wcerr<< "Error str " << error << std::endl;
//        std::wstring secstr(tst);
//        std::wcerr << "--- " << secstr << std::endl;
//        DbgPrint(L"--- : %s\n", (const wchar_t*)tst);
//        printf("%ls\n",(const wchar_t*)tst);

//        DbgPrint(L"\tShareMode = 0x%x\n", ShareAccess);

//        MirrorCheckFlag(ShareAccess, FILE_SHARE_READ);
//        MirrorCheckFlag(ShareAccess, FILE_SHARE_WRITE);
//        MirrorCheckFlag(ShareAccess, FILE_SHARE_DELETE);

//        DbgPrint(L"\tDesiredAccess = 0x%x\n", DesiredAccess);

//        MirrorCheckFlag(DesiredAccess, GENERIC_READ);
//        MirrorCheckFlag(DesiredAccess, GENERIC_WRITE);
//        MirrorCheckFlag(DesiredAccess, GENERIC_EXECUTE);

//        MirrorCheckFlag(DesiredAccess, DELETE);
//        MirrorCheckFlag(DesiredAccess, FILE_READ_DATA);
//        MirrorCheckFlag(DesiredAccess, FILE_READ_ATTRIBUTES);
//        MirrorCheckFlag(DesiredAccess, FILE_READ_EA);
//        MirrorCheckFlag(DesiredAccess, READ_CONTROL);
//        MirrorCheckFlag(DesiredAccess, FILE_WRITE_DATA);
//        MirrorCheckFlag(DesiredAccess, FILE_WRITE_ATTRIBUTES);
//        MirrorCheckFlag(DesiredAccess, FILE_WRITE_EA);
//        MirrorCheckFlag(DesiredAccess, FILE_APPEND_DATA);
//        MirrorCheckFlag(DesiredAccess, WRITE_DAC);
//        MirrorCheckFlag(DesiredAccess, WRITE_OWNER);
//        MirrorCheckFlag(DesiredAccess, SYNCHRONIZE);
//        MirrorCheckFlag(DesiredAccess, FILE_EXECUTE);
//        MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_READ);
//        MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_WRITE);
//        MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_EXECUTE);


//        DbgPrint(L"\tFlagsAndAttributes = 0x%x\n", fileAttributesAndFlags);

//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ARCHIVE);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_COMPRESSED);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DEVICE);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DIRECTORY);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ENCRYPTED);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_HIDDEN);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_INTEGRITY_STREAM);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NORMAL);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NO_SCRUB_DATA);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_OFFLINE);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_READONLY);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_REPARSE_POINT);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SPARSE_FILE);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SYSTEM);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_TEMPORARY);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_VIRTUAL);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_WRITE_THROUGH);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OVERLAPPED);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_NO_BUFFERING);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_RANDOM_ACCESS);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_SEQUENTIAL_SCAN);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_DELETE_ON_CLOSE);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_BACKUP_SEMANTICS);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_POSIX_SEMANTICS);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_REPARSE_POINT);
//        MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_NO_RECALL);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_ANONYMOUS);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IDENTIFICATION);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IMPERSONATION);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_DELEGATION);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_CONTEXT_TRACKING);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_EFFECTIVE_ONLY);
//        MirrorCheckFlag(fileAttributesAndFlags, SECURITY_SQOS_PRESENT);

//        volatile int X=0;
//    }

    DbgPrint(L"CreateFile : %s \n", filePath,FileName);

    if(filename_str==LR"(\***REMOVED***)"){
        if(!(fileAttributesAndFlags & FILE_ATTRIBUTE_DIRECTORY) ){
            fileAttributesAndFlags |= FILE_ATTRIBUTE_DIRECTORY;
            DokanFileInfo->IsDirectory = TRUE;

        }
    }


    PrintUserName(DokanFileInfo);



    /*
  if (ShareMode == 0 && AccessMode & FILE_WRITE_DATA)
          ShareMode = FILE_SHARE_WRITE;
  else if (ShareMode == 0)
          ShareMode = FILE_SHARE_READ;
  */



    DbgPrint(L"\tShareMode = 0x%x\n", ShareAccess);

    MirrorCheckFlag(ShareAccess, FILE_SHARE_READ);
    MirrorCheckFlag(ShareAccess, FILE_SHARE_WRITE);
    MirrorCheckFlag(ShareAccess, FILE_SHARE_DELETE);

    DbgPrint(L"\tDesiredAccess = 0x%x\n", DesiredAccess);

    MirrorCheckFlag(DesiredAccess, GENERIC_READ);
    MirrorCheckFlag(DesiredAccess, GENERIC_WRITE);
    MirrorCheckFlag(DesiredAccess, GENERIC_EXECUTE);

    MirrorCheckFlag(DesiredAccess, DELETE);
    MirrorCheckFlag(DesiredAccess, FILE_READ_DATA);
    MirrorCheckFlag(DesiredAccess, FILE_READ_ATTRIBUTES);
    MirrorCheckFlag(DesiredAccess, FILE_READ_EA);
    MirrorCheckFlag(DesiredAccess, READ_CONTROL);
    MirrorCheckFlag(DesiredAccess, FILE_WRITE_DATA);
    MirrorCheckFlag(DesiredAccess, FILE_WRITE_ATTRIBUTES);
    MirrorCheckFlag(DesiredAccess, FILE_WRITE_EA);
    MirrorCheckFlag(DesiredAccess, FILE_APPEND_DATA);
    MirrorCheckFlag(DesiredAccess, WRITE_DAC);
    MirrorCheckFlag(DesiredAccess, WRITE_OWNER);
    MirrorCheckFlag(DesiredAccess, SYNCHRONIZE);
    MirrorCheckFlag(DesiredAccess, FILE_EXECUTE);
    MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_READ);
    MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_WRITE);
    MirrorCheckFlag(DesiredAccess, STANDARD_RIGHTS_EXECUTE);

    // When filePath is a directory, needs to change the flag so that the file can
    // be opened.
    fileAttr = GetFileAttributes(filePath);

    if (fileAttr != INVALID_FILE_ATTRIBUTES && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateOptions & FILE_NON_DIRECTORY_FILE) {
            DbgPrint(L"\tCannot open a dir as a file\n");
            return STATUS_FILE_IS_A_DIRECTORY;
        }
        DokanFileInfo->IsDirectory = TRUE;
        // Needed by FindFirstFile to list files in it
        // TODO: use ReOpenFile in MirrorFindFiles to set share read temporary
        ShareAccess |= FILE_SHARE_READ;
    }

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE  | FILE_SHARE_DELETE;

    if(DesiredAccess & WRITE_DAC){
        DesiredAccess &= ~WRITE_DAC;
    }

    if(DesiredAccess & WRITE_OWNER ){
         DesiredAccess &= ~WRITE_OWNER;
    }

    DbgPrint(L"\tFlagsAndAttributes = 0x%x\n", fileAttributesAndFlags);

    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ARCHIVE);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_COMPRESSED);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DEVICE);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DIRECTORY);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ENCRYPTED);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_HIDDEN);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_INTEGRITY_STREAM);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NORMAL);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NO_SCRUB_DATA);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_OFFLINE);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_READONLY);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_REPARSE_POINT);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SPARSE_FILE);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SYSTEM);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_TEMPORARY);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_VIRTUAL);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_WRITE_THROUGH);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OVERLAPPED);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_NO_BUFFERING);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_RANDOM_ACCESS);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_SEQUENTIAL_SCAN);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_DELETE_ON_CLOSE);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_BACKUP_SEMANTICS);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_POSIX_SEMANTICS);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_REPARSE_POINT);
    MirrorCheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_NO_RECALL);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_ANONYMOUS);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IDENTIFICATION);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_IMPERSONATION);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_DELEGATION);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_CONTEXT_TRACKING);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_EFFECTIVE_ONLY);
    MirrorCheckFlag(fileAttributesAndFlags, SECURITY_SQOS_PRESENT);

    if (g_CaseSensitive)
        fileAttributesAndFlags |= FILE_FLAG_POSIX_SEMANTICS;

    if (creationDisposition == CREATE_NEW) {
        DbgPrint(L"\tCREATE_NEW\n");
    } else if (creationDisposition == OPEN_ALWAYS) {
        DbgPrint(L"\tOPEN_ALWAYS\n");
    } else if (creationDisposition == CREATE_ALWAYS) {
        DbgPrint(L"\tCREATE_ALWAYS\n");
    } else if (creationDisposition == OPEN_EXISTING) {
        DbgPrint(L"\tOPEN_EXISTING\n");
    } else if (creationDisposition == TRUNCATE_EXISTING) {
        DbgPrint(L"\tTRUNCATE_EXISTING\n");
    } else {
        DbgPrint(L"\tUNKNOWN creationDisposition!\n");
    }

    if (g_ImpersonateCallerUser) {
        userTokenHandle = DokanOpenRequestorToken(DokanFileInfo);

        if (userTokenHandle == INVALID_HANDLE_VALUE) {
            DbgPrint(L"  DokanOpenRequestorToken failed\n");
            // Should we return some error?
        }
    }

    // declare a security descriptor

    SECURITY_DESCRIPTOR SD;
    BOOL bSetOk;

    // initializes a new security descriptor. The InitializeSecurityDescriptor()

    // function initializes a security descriptor to have no system access control list (SACL),

    // no discretionary access control list (DACL), no owner, no primary group,

    // and all control flags set to FALSE (NULL). Thus, except for its revision level, it is empty.

    BOOL bInitOk = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

    if(bInitOk)

    {

        //                      wprintf(LInitializeSecurityDescriptor() is OK\n);

        // sets information in a discretionary access control list (DACL).

        // If a DACL is already present in the security descriptor, the DACL is replaced.

        // give the security descriptor a Null Dacl

        // done using the  TRUE, (PACL)NULL here

        bSetOk = SetSecurityDescriptorDacl(&SD, TRUE,(PACL)NULL, FALSE);

//        if(bSetOk){
//            SecurityContext->AccessState.SecurityDescriptor = &SD;
//            securityAttrib.lpSecurityDescriptor=&SD;
//        }
    }

    if (DokanFileInfo->IsDirectory) {
        // It is a create directory request

        if (creationDisposition == CREATE_NEW || creationDisposition == OPEN_ALWAYS) {

            if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
                // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
                if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                    // handle the error if failed to impersonate
                    DbgPrint(L"\tImpersonateLoggedOnUser failed.\n");
                }
            }

            if(creationDisposition == CREATE_NEW || creationDisposition == CREATE_ALWAYS){
            //            filenodes->operator[](filename_str);
                filenodes->emplace(filename_str,std::make_shared<filenode>(filename_str, true, FILE_ATTRIBUTE_DIRECTORY, SecurityContext));
                SecurityContext->AccessState.SecurityDescriptor = NULL;
                securityAttrib.lpSecurityDescriptor = NULL;

//                // declare a security descriptor

//                    SECURITY_DESCRIPTOR SD;

//                    // initializes a new security descriptor. The InitializeSecurityDescriptor()

//                    // function initializes a security descriptor to have no system access control list (SACL),

//                    // no discretionary access control list (DACL), no owner, no primary group,

//                    // and all control flags set to FALSE (NULL). Thus, except for its revision level, it is empty.

//                   BOOL bInitOk = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

//                    if(bInitOk)

//                    {

//    //                      wprintf(LInitializeSecurityDescriptor() is OK\n);

//                          // sets information in a discretionary access control list (DACL).

//                          // If a DACL is already present in the security descriptor, the DACL is replaced.

//                          // give the security descriptor a Null Dacl

//                          // done using the  TRUE, (PACL)NULL here

//                         BOOL bSetOk = SetSecurityDescriptorDacl(&SD, TRUE,(PACL)NULL, FALSE);

//                         if(bSetOk){
//                             SecurityContext->AccessState.SecurityDescriptor = &SD;
//                             securityAttrib.lpSecurityDescriptor=&SD;
//                         }
//                    }
            }


            //We create folder
            if (!CreateDirectory(filePath, &securityAttrib)) {
                error = GetLastError();
                // Fail to create folder for OPEN_ALWAYS is not an error
                if (error != ERROR_ALREADY_EXISTS || creationDisposition == CREATE_NEW) {
                    DbgPrint(L"\terror code = %d\n\n", error);
                    status = DokanNtStatusFromWin32(error);
                }
            }

            if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
                // Clean Up operation for impersonate
                DWORD lastError = GetLastError();
                if (status != STATUS_SUCCESS) //Keep the handle open for CreateFile
                    CloseHandle(userTokenHandle);
                RevertToSelf();
                SetLastError(lastError);
            }
        }

        if (status == STATUS_SUCCESS) {

            //Check first if we're trying to open a file as a directory.
            if (fileAttr != INVALID_FILE_ATTRIBUTES && !(fileAttr & FILE_ATTRIBUTE_DIRECTORY) && (CreateOptions & FILE_DIRECTORY_FILE)) {
                return STATUS_NOT_A_DIRECTORY;
            }

            if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
                // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
                if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                    // handle the error if failed to impersonate
                    DbgPrint(L"\tImpersonateLoggedOnUser failed.\n");
                }
            }

//            auto fit = filenodes->find(filename_str);
//            std::shared_ptr<filenode> f;
//            f=  (fit != filenodes->end()) ? fit->second : nullptr;
//            if(f)

            if(bSetOk){
                SecurityContext->AccessState.SecurityDescriptor = &SD;
                securityAttrib.lpSecurityDescriptor=&SD;
            }

            // FILE_FLAG_BACKUP_SEMANTICS is required for opening directory handles
            handle = CreateFile(filePath, genericDesiredAccess, ShareAccess, &securityAttrib, OPEN_EXISTING, fileAttributesAndFlags | FILE_FLAG_BACKUP_SEMANTICS, NULL);

            if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
                // Clean Up operation for impersonate
                DWORD lastError = GetLastError();
                CloseHandle(userTokenHandle);
                RevertToSelf();
                SetLastError(lastError);
            }

            if (handle == INVALID_HANDLE_VALUE) {
                error = GetLastError();
                DbgPrint(L"\terror code = %d\n\n", error);


//                SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;
//                LPWSTR strAttrib;
//                LPTSTR tst;
//                ULONG size;
//                std::cout <<"Convert Result " << ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityContext->AccessState.SecurityDescriptor,SDDL_REVISION_1, secInfo , &tst , &size ) << std::endl;
////                DWORD error = GetLastError();
////                std::wcerr<< "Error str " << error << std::endl;
//                std::wstring secstr(tst);
////                std::wcerr << "--- " << secstr << std::endl;
////                DbgPrint(L"--- : %s\n", (const wchar_t*)tst);
////                printf("%ls\n",(const wchar_t*)tst);
                status = DokanNtStatusFromWin32(error);

                if(error == 1307 || error == 1308 || error == 5){
//                    directory.emplace(std::wstring(filePath),secstr);
//                    LocalFree(SecurityContext->AccessState.SecurityDescriptor);
//                    SecurityContext->AccessState.SecurityDescriptor = NULL;
//                    status = MirrorCreateFile(FileName, SecurityContext,DesiredAccess,  FileAttributes,ShareAccess,  CreateDisposition, CreateOptions,  DokanFileInfo);
//                    auto filename_str = std::wstring(FileName);
//                    _filenodes[filename_str] = std::make_shared<filenode>(filename_str, true, FILE_ATTRIBUTE_DIRECTORY, SecurityContext);
//                    SecurityContext->AccessState.SecurityDescriptor = NULL;
//                    status = MirrorCreateFile(FileName, SecurityContext,DesiredAccess,  FileAttributes,ShareAccess,  CreateDisposition, CreateOptions,  DokanFileInfo);

                }

            } else {
                DokanFileInfo->Context = (ULONG64)handle; // save the file handle in Context

                // Open succeed but we need to inform the driver
                // that the dir open and not created by returning STATUS_OBJECT_NAME_COLLISION
                if (creationDisposition == OPEN_ALWAYS && fileAttr != INVALID_FILE_ATTRIBUTES)
                    return STATUS_OBJECT_NAME_COLLISION;
            }
        }
    } else {
        // It is a create file request

        // Cannot overwrite a hidden or system file if flag not set
//        if (fileAttr != INVALID_FILE_ATTRIBUTES && ((!(fileAttributesAndFlags & FILE_ATTRIBUTE_HIDDEN) && (fileAttr & FILE_ATTRIBUTE_HIDDEN)) ||(!(fileAttributesAndFlags & FILE_ATTRIBUTE_SYSTEM) && (fileAttr & FILE_ATTRIBUTE_SYSTEM))) && (creationDisposition == TRUNCATE_EXISTING ||  creationDisposition == CREATE_ALWAYS))
//            return STATUS_ACCESS_DENIED;

        // Cannot delete a read only file
        if ((fileAttr != INVALID_FILE_ATTRIBUTES && (fileAttr & FILE_ATTRIBUTE_READONLY) || (fileAttributesAndFlags & FILE_ATTRIBUTE_READONLY)) && (fileAttributesAndFlags & FILE_FLAG_DELETE_ON_CLOSE))
            return STATUS_CANNOT_DELETE;

        // Truncate should always be used with write access
        if (creationDisposition == TRUNCATE_EXISTING)
            genericDesiredAccess |= GENERIC_WRITE;

        if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
            // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
            if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                // handle the error if failed to impersonate
                DbgPrint(L"\tImpersonateLoggedOnUser failed.\n");
            }
        }

//        ACCESS_MASK generic_desiredaccess;
//        DWORD creation_disposition;
//        DWORD file_attributes_and_flags;

//        DokanMapKernelToUserCreateFileFlags(DesiredAccess, FileAttributes, CreateOptions, CreateDisposition, &generic_desiredaccess, &file_attributes_and_flags, &creation_disposition);
//        auto f =  std::make_shared<filenode>(filename_str, false, file_attributes_and_flags, SecurityContext);

        if(creationDisposition == CREATE_NEW){
            filenodes->emplace(filename_str,std::make_shared<filenode>(filename_str, false, fileAttributesAndFlags, SecurityContext));
            SecurityContext->AccessState.SecurityDescriptor = NULL;
            securityAttrib.lpSecurityDescriptor=NULL;

//            // declare a security descriptor

//                SECURITY_DESCRIPTOR SD;

//                // initializes a new security descriptor. The InitializeSecurityDescriptor()

//                // function initializes a security descriptor to have no system access control list (SACL),

//                // no discretionary access control list (DACL), no owner, no primary group,

//                // and all control flags set to FALSE (NULL). Thus, except for its revision level, it is empty.

//               BOOL bInitOk = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

//                if(bInitOk)

//                {

////                      wprintf(LInitializeSecurityDescriptor() is OK\n);

//                      // sets information in a discretionary access control list (DACL).

//                      // If a DACL is already present in the security descriptor, the DACL is replaced.

//                      // give the security descriptor a Null Dacl

//                      // done using the  TRUE, (PACL)NULL here

//                     BOOL bSetOk = SetSecurityDescriptorDacl(&SD, TRUE,(PACL)NULL, FALSE);

//                     if(bSetOk){
//                         SecurityContext->AccessState.SecurityDescriptor = &SD;
//                         securityAttrib.lpSecurityDescriptor=&SD;
//                     }
//                }
        }


//        creationDisposition = FILE_READ_ATTRIBUTES;
//        fileAttributesAndFlags = FILE_FLAG_OPEN_REPARSE_POINT;

//        DesiredAccess = FILE_READ_ATTRIBUTES | READ_CONTROL | FILE_WRITE_DATA | FILE_WRITE_ATTRIBUTES| FILE_WRITE_EA |  FILE_APPEND_DATA |  SYNCHRONIZE |  STANDARD_RIGHTS_READ | STANDARD_RIGHTS_WRITE | STANDARD_RIGHTS_EXECUTE;
//        ShareAccess = FILE_SHARE_READ;
//        FileAttributes = 0;

        if(bSetOk){
            SecurityContext->AccessState.SecurityDescriptor = &SD;
            securityAttrib.lpSecurityDescriptor=&SD;
        }

        handle = CreateFile(filePath, genericDesiredAccess, ShareAccess, &securityAttrib, creationDisposition, fileAttributesAndFlags, NULL);
//        handle = CreateFile(
//                    filePath,
//                    DesiredAccess, // GENERIC_READ|GENERIC_WRITE|GENERIC_EXECUTE,
//                    ShareAccess,
//                    NULL, // security attribute
//                    creationDisposition,
//                    FileAttributes, // |FILE_FLAG_NO_BUFFERING,
//                    NULL);                  // template file handle

        if (g_ImpersonateCallerUser && userTokenHandle != INVALID_HANDLE_VALUE) {
            // Clean Up operation for impersonate
            DWORD lastError = GetLastError();
            CloseHandle(userTokenHandle);
            RevertToSelf();
            SetLastError(lastError);
        }

        if (handle == INVALID_HANDLE_VALUE || handle==NULL ) {
            error = GetLastError();
            DbgPrint(L"\terror code = %d\n\n", error);

//            SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;
//            SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION;
//            LPWSTR strAttrib;
//            LPTSTR tst;
//            ULONG size;
//            std::cout <<"Convert Result " << ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityContext->AccessState.SecurityDescriptor,SDDL_REVISION_1, secInfo , &tst , &size ) << std::endl;
//            std::wcerr<< "Error str " << error << std::endl;
//            std::wstring secstr(tst);
//            std::wcerr << "--- " << secstr << std::endl;
//            DbgPrint(L"--- : %s\n", (const wchar_t*)tst);
//            printf("%ls\n",(const wchar_t*)tst);
            status = DokanNtStatusFromWin32(error);

            if(error == 1307 || error == 1308 || error == 5){
//                std::wcout << "Emplace " << filePath<< " --- " << std::wstring(FileName) << std::endl;
//                files.emplace(std::wstring(filePath),secstr);
////                LocalFree(SecurityContext->AccessState.SecurityDescriptor);
//                SecurityContext->AccessState.SecurityDescriptor = NULL;
//                status = MirrorCreateFile(FileName, SecurityContext,DesiredAccess,  FileAttributes,ShareAccess,  CreateDisposition, CreateOptions,  DokanFileInfo);

//                ACCESS_MASK generic_desiredaccess;
//                DWORD creation_disposition;
//                DWORD file_attributes_and_flags;

//                DokanMapKernelToUserCreateFileFlags(DesiredAccess, FileAttributes, CreateOptions, CreateDisposition, &generic_desiredaccess, &file_attributes_and_flags, &creation_disposition);
//                _filenodes[filename_str] = std::make_shared<filenode>(filename_str, false, file_attributes_and_flags, SecurityContext);
//                SecurityContext->AccessState.SecurityDescriptor = NULL;
//                status = MirrorCreateFile(FileName, SecurityContext,DesiredAccess,  FileAttributes,ShareAccess,  CreateDisposition, CreateOptions,  DokanFileInfo);

            }
        } else {

            //Need to update FileAttributes with previous when Overwrite file
            if (fileAttr != INVALID_FILE_ATTRIBUTES && creationDisposition == TRUNCATE_EXISTING) {
                SetFileAttributes(filePath, fileAttributesAndFlags | fileAttr);
            }

            DokanFileInfo->Context = (ULONG64)handle; // save the file handle in Context

            if (creationDisposition == OPEN_ALWAYS ||
                    creationDisposition == CREATE_ALWAYS) {
                error = GetLastError();
                if (error == ERROR_ALREADY_EXISTS) {
                    DbgPrint(L"\tOpen an already existing file\n");
                    // Open succeed but we need to inform the driver
                    // that the file open and not created by returning STATUS_OBJECT_NAME_COLLISION
                    status = STATUS_OBJECT_NAME_COLLISION;
                }
            }
        }
    }

    DbgPrint(L"\n");
    return status;
}

#pragma warning(push)
#pragma warning(disable : 4305)

static void DOKAN_CALLBACK MirrorCloseFile(LPCWSTR FileName,  PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    if(DokanFileInfo->Context!=NULL && reinterpret_cast<HANDLE>(DokanFileInfo->Context) != INVALID_HANDLE_VALUE){
        DWORD flags;
        if(GetHandleInformation(reinterpret_cast<HANDLE>(DokanFileInfo->Context),&flags)!=0){
            if (DokanFileInfo->Context) {
                DbgPrint(L"CloseFile: %s\n", filePath);
                DbgPrint(L"\terror : not cleanuped file\n\n");
                CloseHandle((HANDLE)DokanFileInfo->Context);
                DokanFileInfo->Context = reinterpret_cast<ULONG64>(INVALID_HANDLE_VALUE);
            } else {
                DbgPrint(L"Close: %s\n\n", filePath);
            }
        }else{
            DokanFileInfo->Context = reinterpret_cast<ULONG64>(INVALID_HANDLE_VALUE);
            std::cout << "Invalid Handle!" << DokanFileInfo->Context << std::endl;
            std::wcout << L"\t Error code" << GetLastError() << std::endl;
        }
    }


}

static void DOKAN_CALLBACK MirrorCleanup(LPCWSTR FileName,PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    if (DokanFileInfo->Context) {
        DbgPrint(L"Cleanup: %s\n\n", filePath);
        CloseHandle((HANDLE)(DokanFileInfo->Context));
        DokanFileInfo->Context = 0;
    } else {
        DbgPrint(L"Cleanup: %s\n\tinvalid handle\n\n", filePath);
    }

    if (DokanFileInfo->DeleteOnClose) {
        // Should already be deleted by CloseHandle
        // if open with FILE_FLAG_DELETE_ON_CLOSE
        DbgPrint(L"\tDeleteOnClose\n");
        if (DokanFileInfo->IsDirectory) {
            DbgPrint(L"  DeleteDirectory ");
            if (!RemoveDirectory(filePath)) {
                DbgPrint(L"error code = %d\n\n", GetLastError());
            } else {
                DbgPrint(L"success\n\n");
            }
        } else {
            DbgPrint(L"  DeleteFile ");
            if (DeleteFile(filePath) == 0) {
                DbgPrint(L" error code = %d\n\n", GetLastError());
            } else {
                DbgPrint(L"success\n\n");
            }
        }
    }
}

static NTSTATUS DOKAN_CALLBACK MirrorReadFile(LPCWSTR FileName, LPVOID Buffer, DWORD BufferLength, LPDWORD ReadLength,  LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;
    ULONG offset = (ULONG)Offset;
    BOOL opened = FALSE;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"ReadFile : %s\n", filePath);

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle, cleanuped?\n");
        handle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                            OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            DbgPrint(L"\tCreateFile error : %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
        opened = TRUE;
    }

    LARGE_INTEGER distanceToMove;
    distanceToMove.QuadPart = Offset;
    if (!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tseek error, offset = %d\n\n", offset);
        if (opened)
            CloseHandle(handle);
        return DokanNtStatusFromWin32(error);
    }

    if (!ReadFile(handle, Buffer, BufferLength, ReadLength, NULL)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tread error = %u, buffer length = %d, read length = %d\n\n",
                 error, BufferLength, *ReadLength);
        if (opened)
            CloseHandle(handle);
        return DokanNtStatusFromWin32(error);

    } else {
        DbgPrint(L"\tByte to read: %d, Byte read %d, offset %d\n\n", BufferLength,
                 *ReadLength, offset);
    }

    if (opened)
        CloseHandle(handle);

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorWriteFile(LPCWSTR FileName, LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;
    BOOL opened = FALSE;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"WriteFile : %s, offset %I64d, length %d\n", filePath, Offset,
             NumberOfBytesToWrite);

    // reopen the file
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle, cleanuped?\n");
        handle = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            DbgPrint(L"\tCreateFile error : %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
        opened = TRUE;
    }

    UINT64 fileSize = 0;
    DWORD fileSizeLow = 0;
    DWORD fileSizeHigh = 0;
    fileSizeLow = GetFileSize(handle, &fileSizeHigh);
    if (fileSizeLow == INVALID_FILE_SIZE) {
        DWORD error = GetLastError();
        DbgPrint(L"\tcan not get a file size error = %d\n", error);
        if (opened)
            CloseHandle(handle);
        return DokanNtStatusFromWin32(error);
    }

    fileSize = ((UINT64)fileSizeHigh << 32) | fileSizeLow;

    LARGE_INTEGER distanceToMove;
    if (DokanFileInfo->WriteToEndOfFile) {
        LARGE_INTEGER z;
        z.QuadPart = 0;
        if (!SetFilePointerEx(handle, z, NULL, FILE_END)) {
            DWORD error = GetLastError();
            DbgPrint(L"\tseek error, offset = EOF, error = %d\n", error);
            if (opened)
                CloseHandle(handle);
            return DokanNtStatusFromWin32(error);
        }
    } else {
        // Paging IO cannot write after allocate file size.
        if (DokanFileInfo->PagingIo) {
            if ((UINT64)Offset >= fileSize) {
                *NumberOfBytesWritten = 0;
                if (opened)
                    CloseHandle(handle);
                return STATUS_SUCCESS;
            }

            if (((UINT64)Offset + NumberOfBytesToWrite) > fileSize) {
                UINT64 bytes = fileSize - Offset;
                if (bytes >> 32) {
                    NumberOfBytesToWrite = (DWORD)(bytes & 0xFFFFFFFFUL);
                } else {
                    NumberOfBytesToWrite = (DWORD)bytes;
                }
            }
        }

        if ((UINT64)Offset > fileSize) {
            // In the mirror sample helperZeroFileData is not necessary. NTFS will
            // zero a hole.
            // But if user's file system is different from NTFS( or other Windows's
            // file systems ) then  users will have to zero the hole themselves.
        }

        distanceToMove.QuadPart = Offset;
        if (!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)) {
            DWORD error = GetLastError();
            DbgPrint(L"\tseek error, offset = %I64d, error = %d\n", Offset, error);
            if (opened)
                CloseHandle(handle);
            return DokanNtStatusFromWin32(error);
        }
    }

    if (!WriteFile(handle, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten,
                   NULL)) {
        DWORD error = GetLastError();
        DbgPrint(L"\twrite error = %u, buffer length = %d, write length = %d\n",
                 error, NumberOfBytesToWrite, *NumberOfBytesWritten);
        if (opened)
            CloseHandle(handle);
        return DokanNtStatusFromWin32(error);

    } else {
        DbgPrint(L"\twrite %d, offset %I64d\n\n", *NumberOfBytesWritten, Offset);
    }

    // close the file when it is reopened
    if (opened)
        CloseHandle(handle);

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorFlushFileBuffers(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"FlushFileBuffers : %s\n", filePath);

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_SUCCESS;
    }

    if (FlushFileBuffers(handle)) {
        return STATUS_SUCCESS;
    } else {
        DWORD error = GetLastError();
        DbgPrint(L"\tflush error code = %d\n", error);
        return DokanNtStatusFromWin32(error);
    }
}

static NTSTATUS DOKAN_CALLBACK MirrorGetFileInformation( LPCWSTR FileName, LPBY_HANDLE_FILE_INFORMATION HandleFileInformation,  PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;
    BOOL opened = FALSE;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"GetFileInfo : %s\n", filePath);

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle, cleanuped?\n");
        handle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            DbgPrint(L"\tCreateFile error : %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
        opened = TRUE;
    }

    if (!GetFileInformationByHandle(handle, HandleFileInformation)) {
        DbgPrint(L"\terror code = %d\n", GetLastError());

        // FileName is a root directory
        // in this case, FindFirstFile can't get directory information
        if (wcslen(FileName) == 1) {
            DbgPrint(L"  root dir\n");
            HandleFileInformation->dwFileAttributes = GetFileAttributes(filePath);

        } else {
            WIN32_FIND_DATAW find;
            ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
            HANDLE findHandle = FindFirstFile(filePath, &find);
            if (findHandle == INVALID_HANDLE_VALUE) {
                DWORD error = GetLastError();
                DbgPrint(L"\tFindFirstFile error code = %d\n\n", error);
                if (opened)
                    CloseHandle(handle);
                return DokanNtStatusFromWin32(error);
            }

            HandleFileInformation->dwFileAttributes = find.dwFileAttributes;
            HandleFileInformation->ftCreationTime = find.ftCreationTime;
            HandleFileInformation->ftLastAccessTime = find.ftLastAccessTime;
            HandleFileInformation->ftLastWriteTime = find.ftLastWriteTime;
            HandleFileInformation->nFileSizeHigh = find.nFileSizeHigh;
            HandleFileInformation->nFileSizeLow = find.nFileSizeLow;
            DbgPrint(L"\tFindFiles OK, file size = %d\n", find.nFileSizeLow);
            FindClose(findHandle);
        }
    } else {
        DbgPrint(L"\tGetFileInformationByHandle success, file size = %d\n",
                 HandleFileInformation->nFileSizeLow);
    }

    DbgPrint(L"FILE ATTRIBUTE  = %d\n", HandleFileInformation->dwFileAttributes);
    DbgPrint(L"\tFileAttributes = 0x%x\n", HandleFileInformation->dwFileAttributes);

    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_COMPRESSED);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_DEVICE);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_ENCRYPTED);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_HIDDEN);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_INTEGRITY_STREAM);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NORMAL);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NO_SCRUB_DATA);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_OFFLINE);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_READONLY);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_SPARSE_FILE);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_SYSTEM);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_TEMPORARY);
    MirrorCheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_VIRTUAL);

    if(HandleFileInformation->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE){
        HandleFileInformation->dwFileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
        if(HandleFileInformation->dwFileAttributes==0)
            HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }


    if (opened)
        CloseHandle(handle);

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorFindFiles(LPCWSTR FileName, PFillFindData FillFindData, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    size_t fileLen;
    HANDLE hFind;
    WIN32_FIND_DATAW findData;
    DWORD error;
    int count = 0;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"FindFiles : %s\n", filePath);

    fileLen = wcslen(filePath);
    if (filePath[fileLen - 1] != L'\\') {
        filePath[fileLen++] = L'\\';
    }
    if (fileLen + 1 >= DOKAN_MAX_PATH)
        return STATUS_BUFFER_OVERFLOW;
    filePath[fileLen] = L'*';
    filePath[fileLen + 1] = L'\0';

    hFind = FindFirstFile(filePath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        error = GetLastError();
        DbgPrint(L"\tinvalid file handle. Error is %u\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    // Root folder does not have . and .. folder - we remove them
    BOOLEAN rootFolder = (wcscmp(FileName, L"\\") == 0);
    do {
        if (!rootFolder || (wcscmp(findData.cFileName, L".") != 0 &&
                            wcscmp(findData.cFileName, L"..") != 0))
            FillFindData(&findData, DokanFileInfo);
        count++;
    } while (FindNextFile(hFind, &findData) != 0);

    error = GetLastError();
    FindClose(hFind);

    if (error != ERROR_NO_MORE_FILES) {
        DbgPrint(L"\tFindNextFile error. Error is %u\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    DbgPrint(L"\tFindFiles return %d entries in %s\n\n", count, filePath);

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorDeleteFile(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) { WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);
    DbgPrint(L"DeleteFile %s - %d\n", filePath, DokanFileInfo->DeleteOnClose);

    DWORD dwAttrib = GetFileAttributes(filePath);

    if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        return STATUS_ACCESS_DENIED;

    if (handle && handle != INVALID_HANDLE_VALUE) {
        FILE_DISPOSITION_INFO fdi;
        fdi.DeleteFile = DokanFileInfo->DeleteOnClose;
        if (!SetFileInformationByHandle(handle, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO)))
            return DokanNtStatusFromWin32(GetLastError());
    }

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorDeleteDirectory(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    // HANDLE	handle = (HANDLE)DokanFileInfo->Context;
    HANDLE hFind;
    WIN32_FIND_DATAW findData;
    size_t fileLen;

    ZeroMemory(filePath, sizeof(filePath));
    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"DeleteDirectory %s - %d\n", filePath,
             DokanFileInfo->DeleteOnClose);

    if (!DokanFileInfo->DeleteOnClose)
        //Dokan notify that the file is requested not to be deleted.
        return STATUS_SUCCESS;

    fileLen = wcslen(filePath);
    if (filePath[fileLen - 1] != L'\\') {
        filePath[fileLen++] = L'\\';
    }
    if (fileLen + 1 >= DOKAN_MAX_PATH)
        return STATUS_BUFFER_OVERFLOW;
    filePath[fileLen] = L'*';
    filePath[fileLen + 1] = L'\0';

    hFind = FindFirstFile(filePath, &findData);

    if (hFind == INVALID_HANDLE_VALUE) {
        DWORD error = GetLastError();
        DbgPrint(L"\tDeleteDirectory error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    do {
        if (wcscmp(findData.cFileName, L"..") != 0 &&
                wcscmp(findData.cFileName, L".") != 0) {
            FindClose(hFind);
            DbgPrint(L"\tDirectory is not empty: %s\n", findData.cFileName);
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    } while (FindNextFile(hFind, &findData) != 0);

    DWORD error = GetLastError();

    FindClose(hFind);

    if (error != ERROR_NO_MORE_FILES) {
        DbgPrint(L"\tDeleteDirectory error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorMoveFile(LPCWSTR FileName, LPCWSTR NewFileName, BOOL ReplaceIfExisting,  PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    WCHAR newFilePath[DOKAN_MAX_PATH];
    HANDLE handle;
    DWORD bufferSize;
    BOOL result;
    size_t newFilePathLen;

    PFILE_RENAME_INFO renameInfo = NULL;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);
    if (wcslen(NewFileName) && NewFileName[0] != ':') {
        GetFilePath(newFilePath, DOKAN_MAX_PATH, NewFileName);
    } else {
        // For a stream rename, FileRenameInfo expect the FileName param without the filename
        // like :<stream name>:<stream type>
        wcsncpy_s(newFilePath, DOKAN_MAX_PATH, NewFileName, wcslen(NewFileName));
    }

    DbgPrint(L"MoveFile %s -> %s\n\n", filePath, newFilePath);
    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    newFilePathLen = wcslen(newFilePath);

    // the PFILE_RENAME_INFO struct has space for one WCHAR for the name at
    // the end, so that
    // accounts for the null terminator

    bufferSize = (DWORD)(sizeof(FILE_RENAME_INFO) +
                         newFilePathLen * sizeof(newFilePath[0]));

    renameInfo = (PFILE_RENAME_INFO)malloc(bufferSize);
    if (!renameInfo) {
        return STATUS_BUFFER_OVERFLOW;
    }
    ZeroMemory(renameInfo, bufferSize);

    renameInfo->ReplaceIfExists =
            ReplaceIfExisting
            ? TRUE
            : FALSE; // some warning about converting BOOL to BOOLEAN
    renameInfo->RootDirectory = NULL; // hope it is never needed, shouldn't be
    renameInfo->FileNameLength =
            (DWORD)newFilePathLen *
            sizeof(newFilePath[0]); // they want length in bytes

    wcscpy_s(renameInfo->FileName, newFilePathLen + 1, newFilePath);

    result = SetFileInformationByHandle(handle, FileRenameInfo, renameInfo,
                                        bufferSize);

    free(renameInfo);

    if (result) {
        return STATUS_SUCCESS;
    } else {
        DWORD error = GetLastError();
        DbgPrint(L"\tMoveFile error = %u\n", error);
        return DokanNtStatusFromWin32(error);
    }
}

static NTSTATUS DOKAN_CALLBACK MirrorLockFile(LPCWSTR FileName, LONGLONG ByteOffset,  LONGLONG Length, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER offset;
    LARGE_INTEGER length;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"LockFile %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    length.QuadPart = Length;
    offset.QuadPart = ByteOffset;

    if (!LockFile(handle, offset.LowPart, offset.HighPart, length.LowPart,
                  length.HighPart)) {
        DWORD error = GetLastError();
        DbgPrint(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    DbgPrint(L"\tsuccess\n\n");
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorSetEndOfFile(
        LPCWSTR FileName, LONGLONG ByteOffset, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER offset;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetEndOfFile %s, %I64d\n", filePath, ByteOffset);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    offset.QuadPart = ByteOffset;
    if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tSetFilePointer error: %d, offset = %I64d\n\n", error,
                 ByteOffset);
        return DokanNtStatusFromWin32(error);
    }

    if (!SetEndOfFile(handle)) {
        DWORD error = GetLastError();
        DbgPrint(L"\tSetEndOfFile error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorSetAllocationSize(
        LPCWSTR FileName, LONGLONG AllocSize, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER fileSize;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetAllocationSize %s, %I64d\n", filePath, AllocSize);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    if (GetFileSizeEx(handle, &fileSize)) {
        if (AllocSize < fileSize.QuadPart) {
            fileSize.QuadPart = AllocSize;
            if (!SetFilePointerEx(handle, fileSize, NULL, FILE_BEGIN)) {
                DWORD error = GetLastError();
                DbgPrint(L"\tSetAllocationSize: SetFilePointer eror: %d, "
                         L"offset = %I64d\n\n",
                         error, AllocSize);
                return DokanNtStatusFromWin32(error);
            }
            if (!SetEndOfFile(handle)) {
                DWORD error = GetLastError();
                DbgPrint(L"\tSetEndOfFile error code = %d\n\n", error);
                return DokanNtStatusFromWin32(error);
            }
        }
    } else {
        DWORD error = GetLastError();
        DbgPrint(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorSetFileAttributes(LPCWSTR FileName, DWORD FileAttributes, PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    WCHAR filePath[DOKAN_MAX_PATH];

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetFileAttributes %s 0x%x\n", filePath, FileAttributes);

    if (FileAttributes != 0) {
        if (!SetFileAttributes(filePath, FileAttributes)) {
            DWORD error = GetLastError();
            DbgPrint(L"\terror code = %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
    } else {
        // case FileAttributes == 0 :
        // MS-FSCC 2.6 File Attributes : There is no file attribute with the value 0x00000000
        // because a value of 0x00000000 in the FileAttributes field means that the file attributes for this file MUST NOT be changed when setting basic information for the file
        DbgPrint(L"Set 0 to FileAttributes means MUST NOT be changed. Didn't call "
                 L"SetFileAttributes function. \n");
    }

    DbgPrint(L"\n");
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorSetFileTime(LPCWSTR FileName, CONST FILETIME *CreationTime, CONST FILETIME *LastAccessTime, CONST FILETIME *LastWriteTime,PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetFileTime %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    if (!SetFileTime(handle, CreationTime, LastAccessTime, LastWriteTime)) {
        DWORD error = GetLastError();
        DbgPrint(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    DbgPrint(L"\n");
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK
MirrorUnlockFile(LPCWSTR FileName, LONGLONG ByteOffset, LONGLONG Length,
                 PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"UnlockFile %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    length.QuadPart = Length;
    offset.QuadPart = ByteOffset;

    if (!UnlockFile(handle, offset.LowPart, offset.HighPart, length.LowPart,
                    length.HighPart)) {
        DWORD error = GetLastError();
        DbgPrint(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    DbgPrint(L"\tsuccess\n\n");
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorGetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation,  PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    BOOLEAN requestingSaclInfo;
    auto filenodes = GET_FS_INSTANCE;

    UNREFERENCED_PARAMETER(DokanFileInfo);

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"GetFileSecurity %s\n", filePath);

//    MirrorCheckFlag(*SecurityInformation, FILE_SHARE_READ);
//    MirrorCheckFlag(*SecurityInformation, OWNER_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, GROUP_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, DACL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, SACL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, LABEL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, SCOPE_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, BACKUP_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, PROTECTED_DACL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, PROTECTED_SACL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_DACL_SECURITY_INFORMATION);
//    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_SACL_SECURITY_INFORMATION);

//    requestingSaclInfo = ((*SecurityInformation & SACL_SECURITY_INFORMATION) || (*SecurityInformation & BACKUP_SECURITY_INFORMATION));

//    if (!g_HasSeSecurityPrivilege) {
//        *SecurityInformation &= ~SACL_SECURITY_INFORMATION;
//        *SecurityInformation &= ~BACKUP_SECURITY_INFORMATION;
//    }

    DbgPrint(L"  Opening new handle with READ_CONTROL access\n");
//    HANDLE handle = CreateFile(filePath, READ_CONTROL | ((requestingSaclInfo && g_HasSeSecurityPrivilege)? ACCESS_SYSTEM_SECURITY: 0),
//                FILE_SHARE_WRITE | FILE_SHARE_READ | FILE_SHARE_DELETE,
//                NULL, // security attribute
//                OPEN_EXISTING,
//                FILE_FLAG_BACKUP_SEMANTICS, // |FILE_FLAG_NO_BUFFERING,
//                NULL);

//    if (!handle || handle == INVALID_HANDLE_VALUE) {
//        DbgPrint(L"\tinvalid handle\n\n");
//        int error = GetLastError();
//        return DokanNtStatusFromWin32(error);
//    }
    std::wstring strFileName(filePath);
    bool changed = false;
//    if(DokanFileInfo->IsDirectory){
//        if(directory.count(strFileName)!=0){
//            ConvertStringSecurityDescriptorToSecurityDescriptorW(directory[strFileName].c_str(),SDDL_REVISION_1,&SecurityDescriptor,NULL);
//            changed = true;
//        }

//    }else{
//        if(files.count(strFileName)!=0){
//            ConvertStringSecurityDescriptorToSecurityDescriptorW(files[strFileName].c_str(),SDDL_REVISION_1,&SecurityDescriptor,NULL);
//            changed = true;
//        }
//    }

    auto filename_str = std::wstring(FileName);
    auto fit = filenodes->find(filename_str);
    std::shared_ptr<filenode> f;
    f=  (fit != filenodes->end()) ? fit->second : nullptr;
//    auto fnodes = _filenodes;

    DWORD status = STATUS_SUCCESS;

    if(f){
        // We have a Security Descriptor but we need to extract only informations
        // requested 1 - Convert the Security Descriptor to SDDL string with the
        // informations requested
        LPTSTR pStringBuffer = NULL;
        if (!ConvertSecurityDescriptorToStringSecurityDescriptor(f->security.descriptor.get(), SDDL_REVISION_1, *SecurityInformation, &pStringBuffer, NULL)) {
          return STATUS_NOT_IMPLEMENTED;
        }

        // 2 - Convert the SDDL string back to Security Descriptor
        PSECURITY_DESCRIPTOR SecurityDescriptorTmp = NULL;
        ULONG Size = 0;
        if (!ConvertStringSecurityDescriptorToSecurityDescriptor(pStringBuffer, SDDL_REVISION_1, &SecurityDescriptorTmp, &Size)) {
          LocalFree(pStringBuffer);
          return STATUS_NOT_IMPLEMENTED;
        }
        LocalFree(pStringBuffer);

        *LengthNeeded = Size;
        if (Size > BufferLength) {
          LocalFree(SecurityDescriptorTmp);
          return STATUS_BUFFER_OVERFLOW;
        }

        // 3 - Copy the new SecurityDescriptor to destination
        memcpy(SecurityDescriptor, SecurityDescriptorTmp, Size);
        LocalFree(SecurityDescriptorTmp);

        status = STATUS_SUCCESS;

    }else{

//        PSECURITY_DESCRIPTOR pSD = NULL;
//        DWORD pSD_size=0;
//        GetSecurityInfo(handle,SE_FILE_OBJECT,secInfo,NULL,NULL,NULL,NULL,&pSD);
//        pSD_size = GetSecurityDescriptorLength(pSD);


         if (!GetFileSecurityW(filePath, *SecurityInformation, SecurityDescriptor, BufferLength, LengthNeeded)) {
             int error = GetLastError();
             if (error == ERROR_INSUFFICIENT_BUFFER) {
                 DbgPrint(L"  GetUserObjectSecurity error: ERROR_INSUFFICIENT_BUFFER\n");
//                 CloseHandle(handle);
                 return  STATUS_BUFFER_OVERFLOW;
             } else {
                 DbgPrint(L"  GetUserObjectSecurity error: %d\n", error);
//                 CloseHandle(handle);
                 status = DokanNtStatusFromWin32(error);
             }
         }


//        if (!GetUserObjectSecurity(handle, SecurityInformation, SecurityDescriptor, BufferLength, LengthNeeded)) {
//            int error = GetLastError();
//            if (error == ERROR_INSUFFICIENT_BUFFER) {
//                DbgPrint(L"  GetUserObjectSecurity error: ERROR_INSUFFICIENT_BUFFER\n");
//                CloseHandle(handle);
//                return  STATUS_BUFFER_OVERFLOW;
//            } else {
//                DbgPrint(L"  GetUserObjectSecurity error: %d\n", error);
//                CloseHandle(handle);
//                status = DokanNtStatusFromWin32(error);
//            }
//        }
    }

//        std::wcout << "Str Filename " << strFileName << std::endl;

//        if(DokanFileInfo->IsDirectory){
//            if(directory.count(strFileName)!=0){
//                ConvertStringSecurityDescriptorToSecurityDescriptorW(directory[strFileName].c_str(),SDDL_REVISION_1,&SecurityDescriptor,NULL);
//                status = STATUS_SUCCESS;
//            }

//        }else{
//            if(files.count(strFileName)!=0){
//                std::wcout << "Cache file " << files[strFileName]<< std::endl;
//                ConvertStringSecurityDescriptorToSecurityDescriptorW(files[strFileName].c_str(),SDDL_REVISION_1,&SecurityDescriptor,NULL);
//                status = STATUS_SUCCESS;
//            }
//        }


    // Ensure the Security Descriptor Length is set
    DWORD securityDescriptorLength = GetSecurityDescriptorLength(SecurityDescriptor);
    DbgPrint(L"  GetUserObjectSecurity return true,  *LengthNeeded = "
             L"securityDescriptorLength \n");
    *LengthNeeded = securityDescriptorLength;

//    CloseHandle(handle);

    return status;
}

static NTSTATUS DOKAN_CALLBACK MirrorSetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo) {
    HANDLE handle;
    WCHAR filePath[DOKAN_MAX_PATH];
     auto filenodes = GET_FS_INSTANCE;

//    UNREFERENCED_PARAMETER(SecurityDescriptorLength);

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetFileSecurity %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    auto filename_str = std::wstring(FileName);
    auto fit = filenodes->find(filename_str);
    std::shared_ptr<filenode> f;
    f=  (fit != filenodes->end()) ? fit->second : nullptr;

    if(f){
        std::lock_guard<std::mutex> securityLock(f->_data_mutex);

        // SetPrivateObjectSecurity - ObjectsSecurityDescriptor
        // The memory for the security descriptor must be allocated from the process
        // heap (GetProcessHeap) with the HeapAlloc function.
        // https://devblogs.microsoft.com/oldnewthing/20170727-00/?p=96705


        if(f->security.descriptor_size==0){
             f->security.SetDescriptor(SecurityDescriptor);
             return STATUS_SUCCESS;
        }

        HANDLE pHeap = GetProcessHeap();
        DWORD descSize =  f->security.descriptor_size == 0 ? SecurityDescriptorLength : f->security.descriptor_size;
        PSECURITY_DESCRIPTOR heapSecurityDescriptor = HeapAlloc(pHeap, 0, descSize);
        if (!heapSecurityDescriptor)
            return STATUS_INSUFFICIENT_RESOURCES;

        // Copy our current descriptor into heap memory
        if(f->security.descriptor.get()!=nullptr)
            memcpy(heapSecurityDescriptor, f->security.descriptor.get(), descSize);

        static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE,
                                                FILE_GENERIC_EXECUTE,
                                                FILE_ALL_ACCESS};

        if (!SetPrivateObjectSecurity(*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor, &memfs_mapping, 0)) {
          HeapFree(pHeap, 0, heapSecurityDescriptor);
          return DokanNtStatusFromWin32(GetLastError());
        }

        f->security.SetDescriptor(heapSecurityDescriptor);
        HeapFree(pHeap, 0, heapSecurityDescriptor);

        return STATUS_SUCCESS;

    }else
    if (!SetUserObjectSecurity(handle, SecurityInformation, SecurityDescriptor)) {
        int error = GetLastError();        
        DbgPrint(L"  SetUserObjectSecurity error: %d\n", error);

        DOKAN_IO_SECURITY_CONTEXT SecurityContext;
        SecurityContext.AccessState.SecurityDescriptor = SecurityDescriptor;

        filenodes->emplace(filename_str,std::make_shared<filenode>(filename_str, false, 0, &SecurityContext));

        return STATUS_SUCCESS;

//        if(error == 1307 || error == 1308 || error == 5){

////            SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;
////            LPWSTR strAttrib;
////            LPTSTR tst;
////            ULONG size;
////            ConvertSecurityDescriptorToStringSecurityDescriptorW(SecurityDescriptor,SDDL_REVISION_1, secInfo , &tst , &size );
////            DWORD error = GetLastError();
////            std::wcerr<< "Error str " << error << std::endl;
////            std::wstring secstr(tst);

////            if(DokanFileInfo->IsDirectory){
////                directory.emplace(std::wstring(filePath),secstr);
////                return STATUS_SUCCESS;
////            }else{
////                files.emplace(std::wstring(filePath),secstr);
////                return STATUS_SUCCESS;
////            }
////            LocalFree(SecurityDescriptor);
////            SecurityContext->AccessState.SecurityDescriptor = NULL;
////            MirrorCreateFile(FileName, SecurityContext,DesiredAccess,  FileAttributes,ShareAccess,  CreateDisposition, CreateOptions,  DokanFileInfo);

////            auto filename_str = std::wstring(FileName);
////            auto fit = _filenodes.find(filename_str);
////            std::shared_ptr<filenode> f;
////            f=  (fit != _filenodes.end()) ? fit->second : nullptr;

////            if (!f) return STATUS_OBJECT_NAME_NOT_FOUND;




//            std::lock_guard<std::mutex> securityLock(f->security);

//            // SetPrivateObjectSecurity - ObjectsSecurityDescriptor
//            // The memory for the security descriptor must be allocated from the process
//            // heap (GetProcessHeap) with the HeapAlloc function.
//            // https://devblogs.microsoft.com/oldnewthing/20170727-00/?p=96705

////            GetUserObjectSecurity(handle, SecurityInformation, SecurityDescriptor, BufferLength, LengthNeeded)

//            SECURITY_INFORMATION secInfo = ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | OWNER_SECURITY_INFORMATION;

//            PSECURITY_DESCRIPTOR pSD = NULL;
//            DWORD pSD_size=0;
//            GetSecurityInfo(handle,SE_FILE_OBJECT,secInfo,NULL,NULL,NULL,NULL,&pSD);
//            pSD_size = GetSecurityDescriptorLength(pSD);


//            HANDLE pHeap = GetProcessHeap();
//            PSECURITY_DESCRIPTOR heapSecurityDescriptor = HeapAlloc(pHeap, 0, pSD_size);
//            if (!heapSecurityDescriptor) return STATUS_INSUFFICIENT_RESOURCES;
//            // Copy our current descriptor into heap memory
//            memcpy(heapSecurityDescriptor, pSD, pSD_size);

//            static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE,
//                                                    FILE_GENERIC_EXECUTE,
//                                                    FILE_ALL_ACCESS};

//            if (!SetPrivateObjectSecurity(*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor, &memfs_mapping, 0)) {
//              HeapFree(pHeap, 0, heapSecurityDescriptor);
//              return DokanNtStatusFromWin32(GetLastError());
//            }


//            DWORD fileattrib = GetFileAttributesW(FileName);
//            DOKAN_IO_SECURITY_CONTEXT sec_context;
//            sec_context.AccessState.SecurityDescriptor = heapSecurityDescriptor;
//            filenodes->emplace(filename_str , std::make_shared<filenode>(filename_str, false, fileattrib, &sec_context));

//            HeapFree(pHeap, 0, heapSecurityDescriptor);
//            LocalFree(pSD);

//            return STATUS_SUCCESS;


//        }

        return DokanNtStatusFromWin32(error);
    }
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorGetVolumeInformation(
        LPWSTR VolumeNameBuffer, DWORD VolumeNameSize, LPDWORD VolumeSerialNumber,
        LPDWORD MaximumComponentLength, LPDWORD FileSystemFlags,
        LPWSTR FileSystemNameBuffer, DWORD FileSystemNameSize,
        PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    WCHAR volumeRoot[4];
    DWORD fsFlags = 0;

    wcscpy_s(VolumeNameBuffer, VolumeNameSize, gVolumeName);

    if (VolumeSerialNumber)
        *VolumeSerialNumber = 0x19831116;
    if (MaximumComponentLength)
        *MaximumComponentLength = 255;
    if (FileSystemFlags) {
        *FileSystemFlags = FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK |
                FILE_PERSISTENT_ACLS | FILE_NAMED_STREAMS;
        if (g_CaseSensitive)
            *FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
    }

    volumeRoot[0] = RootDirectory[0];
    volumeRoot[1] = ':';
    volumeRoot[2] = '\\';
    volumeRoot[3] = '\0';

    if (GetVolumeInformation(volumeRoot, NULL, 0, NULL, MaximumComponentLength,
                             &fsFlags, FileSystemNameBuffer,
                             FileSystemNameSize)) {

        if (FileSystemFlags)
            *FileSystemFlags &= fsFlags;

        if (MaximumComponentLength) {
            DbgPrint(L"GetVolumeInformation: max component length %u\n",
                     *MaximumComponentLength);
        }
        if (FileSystemNameBuffer) {
            DbgPrint(L"GetVolumeInformation: file system name %s\n",
                     FileSystemNameBuffer);
        }
        if (FileSystemFlags) {
            DbgPrint(L"GetVolumeInformation: got file system flags 0x%08x,"
                     L" returning 0x%08x\n",
                     fsFlags, *FileSystemFlags);
        }
    } else {

        DbgPrint(L"GetVolumeInformation: unable to query underlying fs,"
                 L" using defaults.  Last error = %u\n",
                 GetLastError());

        // File system name could be anything up to 10 characters.
        // But Windows check few feature availability based on file system name.
        // For this, it is recommended to set NTFS or FAT here.
        wcscpy_s(FileSystemNameBuffer, FileSystemNameSize, L"NTFS");
    }

    return STATUS_SUCCESS;
}

// Uncomment the function and set dokanOperations.GetDiskFreeSpace to personalize disk space
/*
static NTSTATUS DOKAN_CALLBACK MirrorDokanGetDiskFreeSpace(
    PULONGLONG FreeBytesAvailable, PULONGLONG TotalNumberOfBytes,
    PULONGLONG TotalNumberOfFreeBytes, PDOKAN_FILE_INFO DokanFileInfo) {
  UNREFERENCED_PARAMETER(DokanFileInfo);

  *FreeBytesAvailable = (ULONGLONG)(512 * 1024 * 1024);
  *TotalNumberOfBytes = 9223372036854775807;
  *TotalNumberOfFreeBytes = 9223372036854775807;

  return STATUS_SUCCESS;
}
*/

static NTSTATUS DOKAN_CALLBACK MirrorDokanGetDiskFreeSpace(
        PULONGLONG FreeBytesAvailable, PULONGLONG TotalNumberOfBytes,
        PULONGLONG TotalNumberOfFreeBytes, PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);
    WCHAR DriveLetter[3] = {'C', ':', 0};
    PWCHAR RootPathName;

    if (RootDirectory[0] == L'\\') { // UNC as Root
        RootPathName = RootDirectory;
    } else {
        DriveLetter[0] = RootDirectory[0];
        RootPathName = DriveLetter;
    }

    if (!GetDiskFreeSpaceExW(RootPathName, (PULARGE_INTEGER)FreeBytesAvailable,
                             (PULARGE_INTEGER)TotalNumberOfBytes,
                             (PULARGE_INTEGER)TotalNumberOfFreeBytes)) {
        DWORD error = GetLastError();
        DbgPrint(L"GetDiskFreeSpaceEx failed for path %ws", RootPathName);
        return DokanNtStatusFromWin32(error);
    }
    return STATUS_SUCCESS;
}

/**
 * Avoid #include <winternl.h> which as conflict with FILE_INFORMATION_CLASS
 * definition.
 * This only for MirrorFindStreams. Link with ntdll.lib still required.
 *
 * Not needed if you're not using NtQueryInformationFile!
 *
 * BEGIN
 */
#pragma warning(push)
#pragma warning(disable : 4201)
typedef struct _IO_STATUS_BLOCK {
    union {
        NTSTATUS Status;
        PVOID Pointer;
    } DUMMYUNIONNAME;

    ULONG_PTR Information;
} IO_STATUS_BLOCK, *PIO_STATUS_BLOCK;
#pragma warning(pop)

NTSYSCALLAPI NTSTATUS NTAPI NtQueryInformationFile(
        _In_ HANDLE FileHandle, _Out_ PIO_STATUS_BLOCK IoStatusBlock,
        _Out_writes_bytes_(Length) PVOID FileInformation, _In_ ULONG Length,
        _In_ FILE_INFORMATION_CLASS FileInformationClass);
/**
 * END
 */

NTSTATUS DOKAN_CALLBACK
MirrorFindStreams(LPCWSTR FileName, PFillFindStreamData FillFindStreamData,PVOID FindStreamContext,
                  PDOKAN_FILE_INFO DokanFileInfo) {
//    WCHAR filePath[DOKAN_MAX_PATH];
//    HANDLE hFind;
//    WIN32_FIND_STREAM_DATA findData;
//    DWORD error;
//    int count = 0;

//    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

//    DbgPrint(L"FindStreams :%s\n", filePath);

//    hFind = FindFirstStreamW(filePath, FindStreamInfoStandard, &findData, 0);

//    if (hFind == INVALID_HANDLE_VALUE) {
//        error = GetLastError();
//        DbgPrint(L"\tinvalid file handle. Error is %u\n\n", error);
//        return DokanNtStatusFromWin32(error);
//    }

//    FillFindStreamData(&findData, DokanFileInfo);
//    count++;

//    while (FindNextStreamW(hFind, &findData) != 0) {
//        FillFindStreamData(&findData, DokanFileInfo);
//        count++;
//    }

//    error = GetLastError();
//    FindClose(hFind);

//    if (error != ERROR_HANDLE_EOF) {
//        DbgPrint(L"\tFindNextStreamW error. Error is %u\n\n", error);
//        return DokanNtStatusFromWin32(error);
//    }

//    DbgPrint(L"\tFindStreams return %d entries in %s\n\n", count, filePath);

//    return STATUS_SUCCESS;
    UNREFERENCED_PARAMETER(DokanFileInfo);

     WCHAR filePath[DOKAN_MAX_PATH];
     HANDLE hFind;
     WIN32_FIND_STREAM_DATA findData;
     DWORD error;
     int count = 0;

     GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

     DbgPrint(L"FindStreams :%s\n", filePath);

     hFind = FindFirstStreamW(filePath, FindStreamInfoStandard, &findData, 0);

     if (hFind == INVALID_HANDLE_VALUE) {
       error = GetLastError();
       DbgPrint(L"\tinvalid file handle. Error is %u\n\n", error);
       return DokanNtStatusFromWin32(error);
     }

     BOOL bufferFull = FillFindStreamData(&findData, FindStreamContext);
     if (bufferFull) {
       count++;
       while (FindNextStreamW(hFind, &findData) != 0) {
         bufferFull = FillFindStreamData(&findData, FindStreamContext);
         if (!bufferFull)
           break;
         count++;
       }
     }

     error = GetLastError();
     FindClose(hFind);

     if (!bufferFull) {
       DbgPrint(L"\tFindStreams returned %d entries in %s with "
                L"STATUS_BUFFER_OVERFLOW\n\n",
                count, filePath);
       // https://msdn.microsoft.com/en-us/library/windows/hardware/ff540364(v=vs.85).aspx
       return STATUS_BUFFER_OVERFLOW;
     }

     if (error != ERROR_HANDLE_EOF) {
       DbgPrint(L"\tFindNextStreamW error. Error is %u\n\n", error);
       return DokanNtStatusFromWin32(error);
     }

     DbgPrint(L"\tFindStreams return %d entries in %s\n\n", count, filePath);

     return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorMounted(LPCWSTR MountPoint , PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    DbgPrint(L"Mounted\n");
    return STATUS_SUCCESS;
}

static NTSTATUS DOKAN_CALLBACK MirrorUnmounted(PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    DbgPrint(L"Unmounted\n");
    return STATUS_SUCCESS;
}

#pragma warning(pop)

BOOL WINAPI CtrlHandler(DWORD dwCtrlType) {
    switch (dwCtrlType) {
    case CTRL_C_EVENT:
    case CTRL_BREAK_EVENT:
    case CTRL_CLOSE_EVENT:
    case CTRL_LOGOFF_EVENT:
    case CTRL_SHUTDOWN_EVENT:
        SetConsoleCtrlHandler(CtrlHandler, FALSE);
        DokanRemoveMountPoint(MountPoint);
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

    if (argc < 3) {
        ShowUsage();
        return EXIT_FAILURE;
    }

    g_DebugMode = FALSE;
    g_UseStdErr = FALSE;
    g_CaseSensitive = FALSE;

    ZeroMemory(&dokanOptions, sizeof(DOKAN_OPTIONS));
    dokanOptions.Version = DOKAN_VERSION;
//    dokanOptions.ThreadCount = 0; // use default

    for (command = 1; command < argc; command++) {
        switch (towlower(argv[command][1])) {
        case L'r':
            CHECK_CMD_ARG(command, argc)
                    wcscpy_s(RootDirectory, sizeof(RootDirectory) / sizeof(WCHAR),
                             argv[command]);
            if (!wcslen(RootDirectory)) {
                DbgPrint(L"Invalid RootDirectory\n");
                return EXIT_FAILURE;
            }

            DbgPrint(L"RootDirectory: %ls\n", RootDirectory);
            break;
        case L'l':
            CHECK_CMD_ARG(command, argc)
                    wcscpy_s(MountPoint, sizeof(MountPoint) / sizeof(WCHAR), argv[command]);
            dokanOptions.MountPoint = MountPoint;
            break;
        case L't':
            CHECK_CMD_ARG(command, argc)
                   dokanOptions.SingleThread = TRUE;
            break;
        case L'd':
            g_DebugMode = TRUE;
            break;
        case L's':
            g_UseStdErr = TRUE;
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
            g_CaseSensitive = TRUE;
            break;
        case L'u':
            CHECK_CMD_ARG(command, argc)
                    wcscpy_s(UNCName, sizeof(UNCName) / sizeof(WCHAR), argv[command]);
            dokanOptions.UNCName = UNCName;
            DbgPrint(L"UNC Name: %ls\n", UNCName);
            break;
        case L'p':
            g_ImpersonateCallerUser = TRUE;
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
              wcscpy_s(gVolumeName, sizeof(gVolumeName) / sizeof(WCHAR), argv[command]);
              DbgPrint(L"Volume Name: %ls\n", gVolumeName);
              break;
        default:
            fwprintf(stderr, L"unknown command: %ls\n", argv[command]);
            return EXIT_FAILURE;
        }
    }

    if (wcscmp(UNCName, L"") != 0 &&
            !(dokanOptions.Options & DOKAN_OPTION_NETWORK)) {
        fwprintf(
                    stderr,
                    L"  Warning: UNC provider name should be set on network drive only.\n");
    }

    if (dokanOptions.Options & DOKAN_OPTION_NETWORK &&
            dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) {
        fwprintf(stderr, L"Mount manager cannot be used on network drive.\n");
        return EXIT_FAILURE;
    }

    if (!(dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) &&
            wcscmp(MountPoint, L"") == 0) {
        fwprintf(stderr, L"Mount Point required.\n");
        return EXIT_FAILURE;
    }

    if ((dokanOptions.Options & DOKAN_OPTION_MOUNT_MANAGER) &&
            (dokanOptions.Options & DOKAN_OPTION_CURRENT_SESSION)) {
        fwprintf(stderr,
                 L"Mount Manager always mount the drive for all user sessions.\n");
        return EXIT_FAILURE;
    }

    if (!SetConsoleCtrlHandler(CtrlHandler, TRUE)) {
        fwprintf(stderr, L"Control Handler is not set.\n");
    }

    // Add security name privilege. Required here to handle GetFileSecurity
    // properly.
    g_HasSeSecurityPrivilege = AddSeSecurityNamePrivilege();
    if (!g_HasSeSecurityPrivilege) {
        fwprintf(stderr,
                 L"[Mirror] Failed to add security privilege to process\n"
                 L"\t=> GetFileSecurity/SetFileSecurity may not work properly\n"
                 L"\t=> Please restart mirror sample with administrator rights to fix it\n");
    }

    if (g_ImpersonateCallerUser && !g_HasSeSecurityPrivilege) {
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

    ZeroMemory(&dokanOperations, sizeof(DOKAN_OPERATIONS));
    dokanOperations.ZwCreateFile = MirrorCreateFile;
    dokanOperations.Cleanup = MirrorCleanup;
    dokanOperations.CloseFile = MirrorCloseFile;
    dokanOperations.ReadFile = MirrorReadFile;
    dokanOperations.WriteFile = MirrorWriteFile;
    dokanOperations.FlushFileBuffers = MirrorFlushFileBuffers;
    dokanOperations.GetFileInformation = MirrorGetFileInformation;
    dokanOperations.FindFiles = MirrorFindFiles;
    dokanOperations.FindFilesWithPattern = NULL;
    dokanOperations.SetFileAttributes = MirrorSetFileAttributes;
    dokanOperations.SetFileTime = MirrorSetFileTime;
    dokanOperations.DeleteFile = MirrorDeleteFile;
    dokanOperations.DeleteDirectory = MirrorDeleteDirectory;
    dokanOperations.MoveFile = MirrorMoveFile;
    dokanOperations.SetEndOfFile = MirrorSetEndOfFile;
    dokanOperations.SetAllocationSize = MirrorSetAllocationSize;
    dokanOperations.LockFile = MirrorLockFile;
    dokanOperations.UnlockFile = MirrorUnlockFile;
    dokanOperations.GetFileSecurity = MirrorGetFileSecurity;
    dokanOperations.SetFileSecurity = MirrorSetFileSecurity;
    dokanOperations.GetDiskFreeSpace = MirrorDokanGetDiskFreeSpace;
    dokanOperations.GetVolumeInformation = MirrorGetVolumeInformation;
    dokanOperations.Unmounted = MirrorUnmounted;
    dokanOperations.FindStreams = MirrorFindStreams;
    dokanOperations.Mounted = MirrorMounted;

    Nodes m_nodes;

    m_nodes._filenodes = std::make_unique<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>();
    dokanOptions.GlobalContext = reinterpret_cast<ULONG64>(&m_nodes);


    DokanInit();
    status = DokanMain(&dokanOptions, &dokanOperations);
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
