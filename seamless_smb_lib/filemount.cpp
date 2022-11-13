#include "filemount.h"

#include "securityprocessor.h"
#include "WinSec.h"
#include "filesecurity.h"
#include "globals.h"
#include "nodes.h"
#include "fileops.h"
#include "context.h"
#include <boost/dll/runtime_symbol_info.hpp>


FileMount::FileMount(std::shared_ptr<Globals> globals, bool debug, bool usestderr, DOKAN_OPTIONS dokanOptions, std::shared_ptr<DbgPrint> print)
{
    std::cout << "FileMount::FileMount" << std::endl;

    memset(&m_dokanOptions,0,sizeof(DOKAN_OPTIONS));    


    m_context = std::make_shared<Context>();
    m_context->globals = globals;

    if(!print)
        m_context->print = std::make_shared<DbgPrint>(usestderr,debug);
    else
        m_context->print = print;
    m_context->nodes = std::make_shared<Nodes>();
    m_context->winsec = std::make_shared<WinSec>( m_context->print);

    dokanOptions.Version = DOKAN_VERSION;
    dokanOptions.MountPoint = globals->MountPoint().data();
    dokanOptions.UNCName = globals->UNCName().c_str();

    dokanOptions.GlobalContext = reinterpret_cast<ULONG64>(m_context.get());

    m_dokanOptions = dokanOptions;


    std::wstring rootdir = globals->RootDirectory();
    boost::filesystem::path program_location = boost::dll::program_location().parent_path();
    std::wstring exec_location = program_location.generic_wstring();
    std::replace(rootdir.begin(),rootdir.end(),L'\\',L'_');

    std::wcout << "Program Location " << exec_location << std::endl;

    std::wstring datfile = exec_location+L"\\"+rootdir+L".dat";

    if(boost::filesystem::exists(datfile)){
         m_context->print->print(L"FILEMOUNT Dat Location -- %s",datfile.c_str());

        std::filebuf filer;
        filer.open(datfile,std::ios_base::in|std::ios_base::binary);
        std::istream is(&filer);
        boost::archive::binary_iarchive ir(is, boost::archive::no_header);

        ir >> *(m_context->nodes);
    }

    std::cout << "FileMount::FileMount END" << std::endl;
}

int FileMount::mount()
{
    if(mounted)
        return DOKAN_SUCCESS;

    std::lock_guard<std::mutex> lg (mutex);


    memset(&dokanOperations,0,sizeof(DOKAN_OPERATIONS));

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

    int status = DokanCreateFileSystem(&m_dokanOptions, &dokanOperations,&handle);
    if(status == DOKAN_SUCCESS){
        mounted = true;
        m_context->print->print(L"%s,%s successfully Mounted\n",getMountPoint().c_str(),getRootDir().c_str());
    }else{
        m_context->print->print(L"%s,%s NOT Mounted, ERROR: %d\n",getMountPoint().c_str(),getRootDir().c_str(),status);
    }
    return status;
}

void FileMount::unmount()
{
    std::lock_guard<std::mutex> lg (mutex);

    if(mounted && handle && DokanIsFileSystemRunning(handle)){
        //        DokanRemoveMountPoint(m_context->globals->MountPoint().c_str());
        DokanCloseHandle(handle);

        mounted = false;
        handle = NULL;

        saveSecurity();
    }

}

bool FileMount::isRunning()
{
    std::lock_guard<std::mutex> lg (mutex);

    if(mounted && handle){
        bool running = DokanIsFileSystemRunning(handle);
        if(!running){
            mounted = false;
            DokanCloseHandle(handle);
            handle = NULL;
        }
        return running;

    }
    return false;
}

void FileMount::join()
{
    DokanWaitForFileSystemClosed(handle,INFINITE);
}

void FileMount::saveSecurity()
{
    std::lock_guard<std::mutex> lg(m_context->nodes->m_mutex);

    std::wstring rootdir = m_context->globals->RootDirectory();
    boost::filesystem::path program_location = boost::dll::program_location().parent_path();
    std::wstring exec_location = program_location.generic_wstring();
    std::replace(rootdir.begin(),rootdir.end(),L'\\',L'_');

    std::wstring datfile = exec_location+L"\\"+rootdir+L".dat";
    std::wcout << "Root dir" << rootdir << std::endl;
    std::wcout << "Write file " << datfile << std::endl;

    std::filebuf file;

    if(file.open(datfile,std::ios_base::out|std::ios_base::binary|std::ios_base::trunc)){
        std::ostream os(&file);
        boost::archive::binary_oarchive ar(os, boost::archive::no_header);

        ar << *(m_context->nodes);
    }
}

std::wstring FileMount::getRootDir()
{
    return m_context->globals->RootDirectory();
}

std::wstring FileMount::getMountPoint()
{
    return m_context->globals->MountPoint();
}

std::wstring FileMount::getVolumeName()
{
    return m_context->globals->volname();
}
