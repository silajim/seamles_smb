#ifndef FILEOPS_H
#define FILEOPS_H

#include <memory>
#include "DbgPrint.h"
//#include "DbgPrint.h"
#include "globals.h"
#include "Context.h"

#include <ostream>
#include <algorithm>

#include "securityprocessor.h"
#include "WinSec.h"
#include "filesecurity.h"


#include "nodes.h"

class FileOps
{

#define GET_FS_INSTANCE \
  reinterpret_cast<Context*>(DokanFileInfo->DokanOptions->GlobalContext)->nodes

#define GET_PRINT_INSTANCE \
  reinterpret_cast<Context*>(DokanFileInfo->DokanOptions->GlobalContext)->print

#define GET_GLOBALS_INSTANCE \
  reinterpret_cast<Context*>(DokanFileInfo->DokanOptions->GlobalContext)->globals

#define GET_WINSEC_INSTANCE \
  reinterpret_cast<Context*>(DokanFileInfo->DokanOptions->GlobalContext)->winsec

#define CheckFlag(val, flag)                                             \
    if (val & flag) {                                                            \
    GET_PRINT_INSTANCE->print(L"\t" L#flag L"\n");                                              \
    }

public:
    FileOps(std::shared_ptr<Nodes> context ,std::shared_ptr<DbgPrint> print, std::shared_ptr<Globals> globals );

    static NTSTATUS MirrorCreateFile(LPCWSTR FileName, PDOKAN_IO_SECURITY_CONTEXT SecurityContext, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PDOKAN_FILE_INFO DokanFileInfo);
    static void MirrorCloseFile(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);
    static void MirrorCleanup(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorReadFile(LPCWSTR FileName, LPVOID Buffer, DWORD BufferLength, LPDWORD ReadLength, LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorWriteFile(LPCWSTR FileName, LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorFlushFileBuffers(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorGetFileInformation(LPCWSTR FileName, LPBY_HANDLE_FILE_INFORMATION HandleFileInformation, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorFindFiles(LPCWSTR FileName, PFillFindData FillFindData, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorDeleteFile(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS DeleteDirectory(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorMoveFile(LPCWSTR FileName, LPCWSTR NewFileName, BOOL ReplaceIfExisting, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorLockFile(LPCWSTR FileName, LONGLONG ByteOffset, LONGLONG Length, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorSetEndOfFile(LPCWSTR FileName, LONGLONG ByteOffset, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorSetAllocationSize(LPCWSTR FileName, LONGLONG AllocSize, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorSetFileAttributes(LPCWSTR FileName, DWORD FileAttributes, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorSetFileTime(LPCWSTR FileName, const FILETIME *CreationTime, const FILETIME *LastAccessTime, const FILETIME *LastWriteTime, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorUnlockFile(LPCWSTR FileName, LONGLONG ByteOffset, LONGLONG Length, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorGetVolumeInformation(LPWSTR VolumeNameBuffer, DWORD VolumeNameSize, LPDWORD VolumeSerialNumber, LPDWORD MaximumComponentLength, LPDWORD FileSystemFlags, LPWSTR FileSystemNameBuffer, DWORD FileSystemNameSize, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorDokanGetDiskFreeSpace(PULONGLONG FreeBytesAvailable, PULONGLONG TotalNumberOfBytes, PULONGLONG TotalNumberOfFreeBytes, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorFindStreams(LPCWSTR FileName, PFillFindStreamData FillFindStreamData, PVOID FindStreamContext, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorMounted(LPCWSTR MountPoint, PDOKAN_FILE_INFO DokanFileInfo);
    static NTSTATUS MirrorUnmounted(PDOKAN_FILE_INFO DokanFileInfo);

    static NTSTATUS MirrorGetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo);

    static NTSTATUS MirrorSetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo);
private:
    std::shared_ptr<DbgPrint> m_print;
    std::shared_ptr<Globals> m_globals;
    std::shared_ptr<Nodes> m_context;
    std::shared_ptr<WinSec> m_winsec;
    static void GetFilePath(PWCHAR filePath, ULONG numberOfElements, LPCWSTR FileName ,  PDOKAN_FILE_INFO DokanFileInfo);
};

#endif // FILEOPS_H
