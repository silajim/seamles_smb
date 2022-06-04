#include "fileops.h"



FileOps::FileOps(std::shared_ptr<Nodes> context , std::shared_ptr<DbgPrint> print, std::shared_ptr<Globals> globals)
{
    m_print = print;
    m_globals = globals;
    m_context = context;
    m_winsec = std::make_shared<WinSec>(print);
}
void FileOps::GetFilePath(PWCHAR filePath, ULONG numberOfElements, LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) {
    wcsncpy_s(filePath, numberOfElements, GET_GLOBALS_INSTANCE->RootDirectory().c_str(), wcslen(GET_GLOBALS_INSTANCE->RootDirectory().c_str()));
    size_t unclen =  GET_GLOBALS_INSTANCE->UNCName().size(); //wcslen(UNCName);
    if (unclen > 0 && _wcsnicmp(FileName, GET_GLOBALS_INSTANCE->UNCName().c_str(), unclen) == 0) {
        if (_wcsnicmp(FileName + unclen, L".", 1) != 0) {
            wcsncat_s(filePath, numberOfElements, FileName + unclen,
                      wcslen(FileName) - unclen);
        }
    } else {
        wcsncat_s(filePath, numberOfElements, FileName, wcslen(FileName));
    }
}

 NTSTATUS DOKAN_CALLBACK
FileOps::MirrorCreateFile(LPCWSTR FileName, PDOKAN_IO_SECURITY_CONTEXT SecurityContext, ACCESS_MASK DesiredAccess, ULONG FileAttributes, ULONG ShareAccess, ULONG CreateDisposition, ULONG CreateOptions, PDOKAN_FILE_INFO DokanFileInfo) {
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


    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    std::wstring filename_str(FileName);

    GET_PRINT_INSTANCE->print(L"CreateFile : %s \n", filePath,FileName);

    // Windows will automatically try to create and access different system
    // directories.
//    if (filename_str == L"\\System Volume Information" ||
//        filename_str == L"\\$RECYCLE.BIN") {
//        return STATUS_NO_SUCH_FILE;
//    }


//    PrintUserName(DokanFileInfo);

    GET_WINSEC_INSTANCE->printSD(SecurityContext->AccessState.SecurityDescriptor,7);


    GET_PRINT_INSTANCE->print(L"\tShareMode = 0x%x\n", ShareAccess);

    CheckFlag(ShareAccess, FILE_SHARE_READ);
    CheckFlag(ShareAccess, FILE_SHARE_WRITE);
    CheckFlag(ShareAccess, FILE_SHARE_DELETE);

    GET_PRINT_INSTANCE->print(L"\tDesiredAccess = 0x%x\n", DesiredAccess);

    if(DesiredAccess & WRITE_DAC){
        DesiredAccess &= ~WRITE_DAC;
    }

    if(DesiredAccess & WRITE_OWNER ){
         DesiredAccess &= ~WRITE_OWNER;
    }

//    if(!(DesiredAccess & SYNCHRONIZE)){
//        DesiredAccess|= SYNCHRONIZE;
//    }

    if(!(DesiredAccess & ACCESS_SYSTEM_SECURITY)){
        DesiredAccess &= ~ACCESS_SYSTEM_SECURITY;
    }

    CheckFlag(DesiredAccess, GENERIC_READ);
    CheckFlag(DesiredAccess, GENERIC_WRITE);
    CheckFlag(DesiredAccess, GENERIC_EXECUTE);

    CheckFlag(DesiredAccess, DELETE);
    CheckFlag(DesiredAccess, FILE_READ_DATA);
    CheckFlag(DesiredAccess, FILE_READ_ATTRIBUTES);
    CheckFlag(DesiredAccess, FILE_READ_EA);
    CheckFlag(DesiredAccess, READ_CONTROL);
    CheckFlag(DesiredAccess, FILE_WRITE_DATA);
    CheckFlag(DesiredAccess, FILE_WRITE_ATTRIBUTES);
    CheckFlag(DesiredAccess, FILE_WRITE_EA);
    CheckFlag(DesiredAccess, FILE_APPEND_DATA);
    CheckFlag(DesiredAccess, WRITE_DAC);
    CheckFlag(DesiredAccess, WRITE_OWNER);
    CheckFlag(DesiredAccess, SYNCHRONIZE);
    CheckFlag(DesiredAccess, FILE_EXECUTE);
    CheckFlag(DesiredAccess, STANDARD_RIGHTS_READ);
    CheckFlag(DesiredAccess, STANDARD_RIGHTS_WRITE);
    CheckFlag(DesiredAccess, STANDARD_RIGHTS_EXECUTE);

    // When filePath is a directory, needs to change the flag so that the file can
    // be opened.
    fileAttr = GetFileAttributes(filePath);

    if (fileAttr != INVALID_FILE_ATTRIBUTES && fileAttr & FILE_ATTRIBUTE_DIRECTORY) {
        if (CreateOptions & FILE_NON_DIRECTORY_FILE) {
            GET_PRINT_INSTANCE->print(L"\tCannot open a dir as a file\n");
            return STATUS_FILE_IS_A_DIRECTORY;
        }
        DokanFileInfo->IsDirectory = TRUE;
        // Needed by FindFirstFile to list files in it
        // TODO: use ReOpenFile in MirrorFindFiles to set share read temporary
        ShareAccess |= FILE_SHARE_READ;
    }

    ShareAccess = FILE_SHARE_READ | FILE_SHARE_WRITE  | FILE_SHARE_DELETE;



    DokanMapKernelToUserCreateFileFlags(DesiredAccess, FileAttributes, CreateOptions, CreateDisposition,&genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

    if(fileAttributesAndFlags & FILE_FLAG_OPEN_REPARSE_POINT){
        fileAttributesAndFlags &= ~FILE_FLAG_OPEN_REPARSE_POINT;
    }

    GET_PRINT_INSTANCE->print(L"\tFlagsAndAttributes = 0x%x\n", fileAttributesAndFlags);

    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ARCHIVE);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_COMPRESSED);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DEVICE);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_DIRECTORY);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_ENCRYPTED);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_HIDDEN);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_INTEGRITY_STREAM);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NORMAL);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_NO_SCRUB_DATA);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_OFFLINE);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_READONLY);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_REPARSE_POINT);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SPARSE_FILE);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_SYSTEM);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_TEMPORARY);
    CheckFlag(fileAttributesAndFlags, FILE_ATTRIBUTE_VIRTUAL);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_WRITE_THROUGH);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_OVERLAPPED);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_NO_BUFFERING);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_RANDOM_ACCESS);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_SEQUENTIAL_SCAN);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_DELETE_ON_CLOSE);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_BACKUP_SEMANTICS);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_POSIX_SEMANTICS);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_REPARSE_POINT);
    CheckFlag(fileAttributesAndFlags, FILE_FLAG_OPEN_NO_RECALL);
    CheckFlag(fileAttributesAndFlags, SECURITY_ANONYMOUS);
    CheckFlag(fileAttributesAndFlags, SECURITY_IDENTIFICATION);
    CheckFlag(fileAttributesAndFlags, SECURITY_IMPERSONATION);
    CheckFlag(fileAttributesAndFlags, SECURITY_DELEGATION);
    CheckFlag(fileAttributesAndFlags, SECURITY_CONTEXT_TRACKING);
    CheckFlag(fileAttributesAndFlags, SECURITY_EFFECTIVE_ONLY);
    CheckFlag(fileAttributesAndFlags, SECURITY_SQOS_PRESENT);



    if (GET_GLOBALS_INSTANCE->CaseSensitive())
        fileAttributesAndFlags |= FILE_FLAG_POSIX_SEMANTICS;

    if (creationDisposition == CREATE_NEW) {
        GET_PRINT_INSTANCE->print(L"\tCREATE_NEW\n");
    } else if (creationDisposition == OPEN_ALWAYS) {
        GET_PRINT_INSTANCE->print(L"\tOPEN_ALWAYS\n");
    } else if (creationDisposition == CREATE_ALWAYS) {
        GET_PRINT_INSTANCE->print(L"\tCREATE_ALWAYS\n");
    } else if (creationDisposition == OPEN_EXISTING) {
        GET_PRINT_INSTANCE->print(L"\tOPEN_EXISTING\n");
    } else if (creationDisposition == TRUNCATE_EXISTING) {
        GET_PRINT_INSTANCE->print(L"\tTRUNCATE_EXISTING\n");
    } else {
        GET_PRINT_INSTANCE->print(L"\tUNKNOWN creationDisposition!\n");
    }

    if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser()) {
        userTokenHandle = DokanOpenRequestorToken(DokanFileInfo);

        if (userTokenHandle == INVALID_HANDLE_VALUE) {
            GET_PRINT_INSTANCE->print(L"  DokanOpenRequestorToken failed\n");
            // Should we return some error?
        }
    }

     DokanMapKernelToUserCreateFileFlags(DesiredAccess, FileAttributes, CreateOptions, CreateDisposition,&genericDesiredAccess, &fileAttributesAndFlags, &creationDisposition);

    // declare a security descriptor

    SECURITY_DESCRIPTOR SD;
    BOOL bSetOk;

    // initializes a new security descriptor. The InitializeSecurityDescriptor()

    // function initializes a security descriptor to have no system access control list (SACL),

    // no discretionary access control list (DACL), no owner, no primary group,

    // and all control flags set to FALSE (NULL). Thus, except for its revision level, it is empty.

//    BOOL bInitOk = InitializeSecurityDescriptor(&SD, SECURITY_DESCRIPTOR_REVISION);

//    if(bInitOk)

//    {

//        //                      wprintf(LInitializeSecurityDescriptor() is OK\n);

//        // sets information in a discretionary access control list (DACL).

//        // If a DACL is already present in the security descriptor, the DACL is replaced.

//        // give the security descriptor a Null Dacl

//        // done using the  TRUE, (PACL)NULL here

//        bSetOk = SetSecurityDescriptorDacl(&SD, TRUE,(PACL)NULL, FALSE);

////        if(bSetOk){
////            SecurityContext->AccessState.SecurityDescriptor = &SD;
////            securityAttrib.lpSecurityDescriptor=&SD;
////        }
//    }

    if (DokanFileInfo->IsDirectory) {
        // It is a create directory request

        if (creationDisposition == CREATE_NEW || creationDisposition == OPEN_ALWAYS) {

            if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
                // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
                if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                    // handle the error if failed to impersonate
                    GET_PRINT_INSTANCE->print(L"\tImpersonateLoggedOnUser failed.\n");
                }
            }

            std::shared_ptr<filenode> f=nullptr;

            if(creationDisposition == CREATE_NEW || creationDisposition == CREATE_ALWAYS){
                f = std::make_shared<filenode>(filename_str, true, FileAttributes, nullptr);
                f->security.SetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,SecurityContext->AccessState.SecurityDescriptor);
                SecurityContext->AccessState.SecurityDescriptor = NULL;
                securityAttrib.lpSecurityDescriptor = NULL;

            }


            //We create folder
            if (!CreateDirectory(filePath, &securityAttrib)) {
                error = GetLastError();
                // Fail to create folder for OPEN_ALWAYS is not an error
                if (error != ERROR_ALREADY_EXISTS || creationDisposition == CREATE_NEW) {
                    GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
                    status = DokanNtStatusFromWin32(error);
                }
            }else{
                if(f){
                    filenodes->addFileNode(f);
                }
            }

            if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
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

            if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
                // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
                if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                    // handle the error if failed to impersonate
                    GET_PRINT_INSTANCE->print(L"\tImpersonateLoggedOnUser failed.\n");
                }
            }

//            if(bSetOk){
//                SecurityContext->AccessState.SecurityDescriptor = &SD;
//                securityAttrib.lpSecurityDescriptor=&SD;
//            }

            // FILE_FLAG_BACKUP_SEMANTICS is required for opening directory handles
            handle = CreateFile(filePath, genericDesiredAccess, ShareAccess, &securityAttrib, OPEN_EXISTING, fileAttributesAndFlags | FILE_FLAG_BACKUP_SEMANTICS, NULL);

            if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
                // Clean Up operation for impersonate
                DWORD lastError = GetLastError();
                CloseHandle(userTokenHandle);
                RevertToSelf();
                SetLastError(lastError);
            }

            if (handle == INVALID_HANDLE_VALUE) {
                error = GetLastError();
                GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);


                status = DokanNtStatusFromWin32(error);

            } else {
                DokanFileInfo->Context = (ULONG64)handle; // save the file handle in Context

                // Open succeed but we need to inform the driver
                // that the dir open and not created by returning STATUS_OBJECT_NAME_COLLISION
                if (creationDisposition == OPEN_ALWAYS && fileAttr != INVALID_FILE_ATTRIBUTES)
                    return STATUS_OBJECT_NAME_COLLISION;
            }
        }
    } else {
        // Cannot delete a read only file
        if ((fileAttr != INVALID_FILE_ATTRIBUTES && (fileAttr & FILE_ATTRIBUTE_READONLY) || (fileAttributesAndFlags & FILE_ATTRIBUTE_READONLY)) && (fileAttributesAndFlags & FILE_FLAG_DELETE_ON_CLOSE))
            return STATUS_CANNOT_DELETE;

        // Truncate should always be used with write access
        if (creationDisposition == TRUNCATE_EXISTING)
            genericDesiredAccess |= GENERIC_WRITE;

        if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
            // if g_ImpersonateCallerUser option is on, call the ImpersonateLoggedOnUser function.
            if (!ImpersonateLoggedOnUser(userTokenHandle)) {
                // handle the error if failed to impersonate
                GET_PRINT_INSTANCE->print(L"\tImpersonateLoggedOnUser failed.\n");
            }
        }
        std::shared_ptr<filenode> f=nullptr;

        if(creationDisposition == CREATE_NEW){
            f = std::make_shared<filenode>(filename_str, false, FileAttributes, nullptr);
            f->security.SetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,SecurityContext->AccessState.SecurityDescriptor);
//            std::lock_guard<std::mutex> lk(filenodes->m_mutex);
//            filenodes->_filenodes->emplace(filename_str,std::make_shared<filenode>(filename_str, false, fileAttributesAndFlags, SecurityContext));
            SecurityContext->AccessState.SecurityDescriptor = NULL;
            securityAttrib.lpSecurityDescriptor=NULL;
        }



//        if(bSetOk){
//            SecurityContext->AccessState.SecurityDescriptor = &SD;
//            securityAttrib.lpSecurityDescriptor=&SD;
//        }

        handle = CreateFileW(filePath, genericDesiredAccess, ShareAccess, &securityAttrib, creationDisposition, fileAttributesAndFlags, NULL);
        if (GET_GLOBALS_INSTANCE->ImpersonateCallerUser() && userTokenHandle != INVALID_HANDLE_VALUE) {
            // Clean Up operation for impersonate
            DWORD lastError = GetLastError();
            CloseHandle(userTokenHandle);
            RevertToSelf();
            SetLastError(lastError);
        }

        if (handle != INVALID_HANDLE_VALUE || handle!=NULL ) {
            if(f)
                filenodes->addFileNode(f);

        }

        if (handle == INVALID_HANDLE_VALUE || handle==NULL ) {
            error = GetLastError();
            GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);

            status = DokanNtStatusFromWin32(error);

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
                    GET_PRINT_INSTANCE->print(L"\tOpen an already existing file\n");
                    // Open succeed but we need to inform the driver
                    // that the file open and not created by returning STATUS_OBJECT_NAME_COLLISION
                    status = STATUS_OBJECT_NAME_COLLISION;
                }
            }
        }
    }

    GET_PRINT_INSTANCE->print(L"\n");
    return status;
}

 void DOKAN_CALLBACK FileOps::MirrorCloseFile(LPCWSTR FileName,  PDOKAN_FILE_INFO DokanFileInfo) {
     WCHAR filePath[DOKAN_MAX_PATH];
     GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

     if(DokanFileInfo->Context!=NULL && reinterpret_cast<HANDLE>(DokanFileInfo->Context) != INVALID_HANDLE_VALUE){
         DWORD flags;
         if(GetHandleInformation(reinterpret_cast<HANDLE>(DokanFileInfo->Context),&flags)!=0){
             if (DokanFileInfo->Context) {
                 GET_PRINT_INSTANCE->print(L"CloseFile: %s\n", filePath);
                 GET_PRINT_INSTANCE->print(L"\terror : not cleanuped file\n\n");
                 CloseHandle((HANDLE)DokanFileInfo->Context);
                 DokanFileInfo->Context = reinterpret_cast<ULONG64>(INVALID_HANDLE_VALUE);
             } else {
                 GET_PRINT_INSTANCE->print(L"Close: %s\n\n", filePath);
             }
         }else{
             DokanFileInfo->Context = reinterpret_cast<ULONG64>(INVALID_HANDLE_VALUE);
             std::cout << "Invalid Handle!" << DokanFileInfo->Context << std::endl;
             std::wcout << L"\t Error code" << GetLastError() << std::endl;
         }
     }

 }

 void DOKAN_CALLBACK FileOps::MirrorCleanup(LPCWSTR FileName,PDOKAN_FILE_INFO DokanFileInfo) {
     WCHAR filePath[DOKAN_MAX_PATH];
     GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

     if (DokanFileInfo->Context) {
         GET_PRINT_INSTANCE->print(L"Cleanup: %s\n\n", filePath);
         CloseHandle((HANDLE)(DokanFileInfo->Context));
         DokanFileInfo->Context = 0;
     } else {
         GET_PRINT_INSTANCE->print(L"Cleanup: %s\n\tinvalid handle\n\n", filePath);
     }

     if (DokanFileInfo->DeleteOnClose) {
         // Should already be deleted by CloseHandle
         // if open with FILE_FLAG_DELETE_ON_CLOSE
         GET_PRINT_INSTANCE->print(L"\tDeleteOnClose\n");
         if (DokanFileInfo->IsDirectory) {
             GET_PRINT_INSTANCE->print(L"  DeleteDirectory ");
             if (!RemoveDirectory(filePath)) {
                 GET_PRINT_INSTANCE->print(L"error code = %d\n\n", GetLastError());
             } else {
                 GET_PRINT_INSTANCE->print(L"success\n\n");
             }
         } else {
             GET_PRINT_INSTANCE->print(L"  DeleteFile ");
             if (DeleteFile(filePath) == 0) {
                 GET_PRINT_INSTANCE->print(L" error code = %d\n\n", GetLastError());
             } else {
                 GET_PRINT_INSTANCE->print(L"success\n\n");
             }
         }
     }
 }

NTSTATUS DOKAN_CALLBACK FileOps::MirrorReadFile(LPCWSTR FileName, LPVOID Buffer, DWORD BufferLength, LPDWORD ReadLength,  LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo) {
     WCHAR filePath[DOKAN_MAX_PATH];
     HANDLE handle = (HANDLE)DokanFileInfo->Context;
     ULONG offset = (ULONG)Offset;
     BOOL opened = FALSE;

     GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

     GET_PRINT_INSTANCE->print(L"ReadFile : %s\n", filePath);

     if (!handle || handle == INVALID_HANDLE_VALUE) {
         GET_PRINT_INSTANCE->print(L"\tinvalid handle, cleanuped?\n");
         handle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL,
                             OPEN_EXISTING, 0, NULL);
         if (handle == INVALID_HANDLE_VALUE) {
             DWORD error = GetLastError();
             GET_PRINT_INSTANCE->print(L"\tCreateFile error : %d\n\n", error);
             return DokanNtStatusFromWin32(error);
         }
         opened = TRUE;
     }

     LARGE_INTEGER distanceToMove;
     distanceToMove.QuadPart = Offset;
     if (!SetFilePointerEx(handle, distanceToMove, NULL, FILE_BEGIN)) {
         DWORD error = GetLastError();
         GET_PRINT_INSTANCE->print(L"\tseek error, offset = %d\n\n", offset);
         if (opened)
             CloseHandle(handle);
         return DokanNtStatusFromWin32(error);
     }

     if (!ReadFile(handle, Buffer, BufferLength, ReadLength, NULL)) {
         DWORD error = GetLastError();
         GET_PRINT_INSTANCE->print(L"\tread error = %u, buffer length = %d, read length = %d\n\n",
                  error, BufferLength, *ReadLength);
         if (opened)
             CloseHandle(handle);
         return DokanNtStatusFromWin32(error);

     } else {
         GET_PRINT_INSTANCE->print(L"\tByte to read: %d, Byte read %d, offset %d\n\n", BufferLength,
                  *ReadLength, offset);
     }

     if (opened)
         CloseHandle(handle);

     return STATUS_SUCCESS;
 }

NTSTATUS DOKAN_CALLBACK FileOps::MirrorWriteFile(LPCWSTR FileName, LPCVOID Buffer, DWORD NumberOfBytesToWrite, LPDWORD NumberOfBytesWritten, LONGLONG Offset, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;
    BOOL opened = FALSE;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"WriteFile : %s, offset %I64d, length %d\n", filePath, Offset,
             NumberOfBytesToWrite);

    // reopen the file
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle, cleanuped?\n");
        handle = CreateFile(filePath, GENERIC_WRITE, FILE_SHARE_WRITE, NULL,
                            OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            GET_PRINT_INSTANCE->print(L"\tCreateFile error : %d\n\n", error);
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
        GET_PRINT_INSTANCE->print(L"\tcan not get a file size error = %d\n", error);
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
            GET_PRINT_INSTANCE->print(L"\tseek error, offset = EOF, error = %d\n", error);
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
            GET_PRINT_INSTANCE->print(L"\tseek error, offset = %I64d, error = %d\n", Offset, error);
            if (opened)
                CloseHandle(handle);
            return DokanNtStatusFromWin32(error);
        }
    }

    if (!WriteFile(handle, Buffer, NumberOfBytesToWrite, NumberOfBytesWritten,
                   NULL)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\twrite error = %u, buffer length = %d, write length = %d\n",
                 error, NumberOfBytesToWrite, *NumberOfBytesWritten);
        if (opened)
            CloseHandle(handle);
        return DokanNtStatusFromWin32(error);

    } else {
        GET_PRINT_INSTANCE->print(L"\twrite %d, offset %I64d\n\n", *NumberOfBytesWritten, Offset);
    }

    // close the file when it is reopened
    if (opened)
        CloseHandle(handle);

    return STATUS_SUCCESS;
}
NTSTATUS DOKAN_CALLBACK FileOps::MirrorFlushFileBuffers(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName , DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"FlushFileBuffers : %s\n", filePath);

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_SUCCESS;
    }

    if (FlushFileBuffers(handle)) {
        return STATUS_SUCCESS;
    } else {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\tflush error code = %d\n", error);
        return DokanNtStatusFromWin32(error);
    }
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorGetFileInformation( LPCWSTR FileName, LPBY_HANDLE_FILE_INFORMATION HandleFileInformation,  PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;
    BOOL opened = FALSE;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"GetFileInfo : %s\n", filePath);

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle, cleanuped?\n");
        handle = CreateFile(filePath, GENERIC_READ, FILE_SHARE_READ, NULL, OPEN_EXISTING, 0, NULL);
        if (handle == INVALID_HANDLE_VALUE) {
            DWORD error = GetLastError();
            GET_PRINT_INSTANCE->print(L"\tCreateFile error : %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
        opened = TRUE;
    }

    if (!GetFileInformationByHandle(handle, HandleFileInformation)) {
        GET_PRINT_INSTANCE->print(L"\terror code = %d\n", GetLastError());

        // FileName is a root directory
        // in this case, FindFirstFile can't get directory information
        if (wcslen(FileName) == 1) {
            GET_PRINT_INSTANCE->print(L"  root dir\n");
            HandleFileInformation->dwFileAttributes = GetFileAttributes(filePath);

        } else {
            WIN32_FIND_DATAW find;
            ZeroMemory(&find, sizeof(WIN32_FIND_DATAW));
            HANDLE findHandle = FindFirstFile(filePath, &find);
            if (findHandle == INVALID_HANDLE_VALUE) {
                DWORD error = GetLastError();
                GET_PRINT_INSTANCE->print(L"\tFindFirstFile error code = %d\n\n", error);
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
            GET_PRINT_INSTANCE->print(L"\tFindFiles OK, file size = %d\n", find.nFileSizeLow);
            FindClose(findHandle);
        }
    } else {
        GET_PRINT_INSTANCE->print(L"\tGetFileInformationByHandle success, file size = %d\n",
                 HandleFileInformation->nFileSizeLow);
    }

    GET_PRINT_INSTANCE->print(L"FILE ATTRIBUTE  = %d\n", HandleFileInformation->dwFileAttributes);
    GET_PRINT_INSTANCE->print(L"\tFileAttributes = 0x%x\n", HandleFileInformation->dwFileAttributes);

    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_ARCHIVE);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_COMPRESSED);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_DEVICE);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_DIRECTORY);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_ENCRYPTED);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_HIDDEN);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_INTEGRITY_STREAM);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NORMAL);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NOT_CONTENT_INDEXED);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_NO_SCRUB_DATA);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_OFFLINE);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_READONLY);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_REPARSE_POINT);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_SPARSE_FILE);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_SYSTEM);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_TEMPORARY);
    CheckFlag(HandleFileInformation->dwFileAttributes, FILE_ATTRIBUTE_VIRTUAL);

    if(HandleFileInformation->dwFileAttributes & FILE_ATTRIBUTE_ARCHIVE){
        HandleFileInformation->dwFileAttributes &= ~FILE_ATTRIBUTE_ARCHIVE;
        if(HandleFileInformation->dwFileAttributes==0)
            HandleFileInformation->dwFileAttributes = FILE_ATTRIBUTE_NORMAL;
    }


    if (opened)
        CloseHandle(handle);

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorFindFiles(LPCWSTR FileName, PFillFindData FillFindData, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    size_t fileLen;
    HANDLE hFind;
    WIN32_FIND_DATAW findData;
    DWORD error;
    int count = 0;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"FindFiles : %s\n", filePath);

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
        GET_PRINT_INSTANCE->print(L"\tinvalid file handle. Error is %u\n\n", error);
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
        GET_PRINT_INSTANCE->print(L"\tFindNextFile error. Error is %u\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    GET_PRINT_INSTANCE->print(L"\tFindFiles return %d entries in %s\n\n", count, filePath);

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorDeleteFile(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo)
{
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle = (HANDLE)DokanFileInfo->Context;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);
    GET_PRINT_INSTANCE->print(L"DeleteFile %s - %d\n", filePath, DokanFileInfo->DeleteOnClose);

    DWORD dwAttrib = GetFileAttributes(filePath);

    if (dwAttrib != INVALID_FILE_ATTRIBUTES && (dwAttrib & FILE_ATTRIBUTE_DIRECTORY))
        return STATUS_ACCESS_DENIED;

    if (handle && handle != INVALID_HANDLE_VALUE) {
        FILE_DISPOSITION_INFO fdi;
        fdi.DeleteFile = DokanFileInfo->DeleteOnClose;
        if (!SetFileInformationByHandle(handle, FileDispositionInfo, &fdi, sizeof(FILE_DISPOSITION_INFO))){
            GET_PRINT_INSTANCE->print(L"DeleteFile ERROR %s - %d\n", filePath,GetLastError());
            return DokanNtStatusFromWin32(GetLastError());
        }
    }

//     if(DeleteFile(filePath)==0){
//         GET_PRINT_INSTANCE->print(L"DeleteFile ERROR %s - %d\n", filePath,GetLastError());
//         return DokanNtStatusFromWin32(GetLastError());
//     }

    GET_FS_INSTANCE->deleteFileNode(FileName);
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::DeleteDirectory(LPCWSTR FileName, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    // HANDLE	handle = (HANDLE)DokanFileInfo->Context;
    HANDLE hFind;
    WIN32_FIND_DATAW findData;
    size_t fileLen;

    ZeroMemory(filePath, sizeof(filePath));
    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"DeleteDirectory %s - %d\n", filePath,
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
        GET_PRINT_INSTANCE->print(L"\tDeleteDirectory error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    do {
        if (wcscmp(findData.cFileName, L"..") != 0 && wcscmp(findData.cFileName, L".") != 0) {
            FindClose(hFind);
            GET_PRINT_INSTANCE->print(L"\tDirectory is not empty: %s\n", findData.cFileName);
            return STATUS_DIRECTORY_NOT_EMPTY;
        }
    } while (FindNextFile(hFind, &findData) != 0);

    DWORD error = GetLastError();

    FindClose(hFind);

    if (error != ERROR_NO_MORE_FILES) {
        GET_PRINT_INSTANCE->print(L"\tDeleteDirectory error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    GET_FS_INSTANCE->deleteFileNode(FileName);
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorMoveFile(LPCWSTR FileName, LPCWSTR NewFileName, BOOL ReplaceIfExisting,  PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    WCHAR newFilePath[DOKAN_MAX_PATH];
    HANDLE handle;
    DWORD bufferSize;
    BOOL result;
    size_t newFilePathLen;

    PFILE_RENAME_INFO renameInfo = NULL;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);
    if (wcslen(NewFileName) && NewFileName[0] != ':') {
        GetFilePath(newFilePath, DOKAN_MAX_PATH, NewFileName,DokanFileInfo);
    } else {
        // For a stream rename, FileRenameInfo expect the FileName param without the filename
        // like :<stream name>:<stream type>
        wcsncpy_s(newFilePath, DOKAN_MAX_PATH, NewFileName, wcslen(NewFileName));
    }

    GET_PRINT_INSTANCE->print(L"MoveFile %s -> %s\n\n", filePath, newFilePath);
    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
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

    renameInfo->ReplaceIfExists =  ReplaceIfExisting ? TRUE: FALSE; // some warning about converting BOOL to BOOLEAN
    renameInfo->RootDirectory = NULL; // hope it is never needed, shouldn't be
    renameInfo->FileNameLength =    (DWORD)newFilePathLen *sizeof(newFilePath[0]); // they want length in bytes

    wcscpy_s(renameInfo->FileName, newFilePathLen + 1, newFilePath);

    result = SetFileInformationByHandle(handle, FileRenameInfo, renameInfo,  bufferSize);

    free(renameInfo);

    if (result) {
        GET_FS_INSTANCE->rename(std::wstring(FileName),std::wstring(NewFileName));
        return STATUS_SUCCESS;
    } else {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\tMoveFile error = %u\n", error);
        return DokanNtStatusFromWin32(error);
    }
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorLockFile(LPCWSTR FileName, LONGLONG ByteOffset,  LONGLONG Length, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER offset;
    LARGE_INTEGER length;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"LockFile %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    length.QuadPart = Length;
    offset.QuadPart = ByteOffset;

    if (!LockFile(handle, offset.LowPart, offset.HighPart, length.LowPart,
                  length.HighPart)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    GET_PRINT_INSTANCE->print(L"\tsuccess\n\n");
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorSetEndOfFile(LPCWSTR FileName, LONGLONG ByteOffset, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER offset;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"SetEndOfFile %s, %I64d\n", filePath, ByteOffset);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    offset.QuadPart = ByteOffset;
    if (!SetFilePointerEx(handle, offset, NULL, FILE_BEGIN)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\tSetFilePointer error: %d, offset = %I64d\n\n", error,
                 ByteOffset);
        return DokanNtStatusFromWin32(error);
    }

    if (!SetEndOfFile(handle)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\tSetEndOfFile error code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorSetAllocationSize(LPCWSTR FileName, LONGLONG AllocSize, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER fileSize;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"SetAllocationSize %s, %I64d\n", filePath, AllocSize);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    if (GetFileSizeEx(handle, &fileSize)) {
        if (AllocSize < fileSize.QuadPart) {
            fileSize.QuadPart = AllocSize;
            if (!SetFilePointerEx(handle, fileSize, NULL, FILE_BEGIN)) {
                DWORD error = GetLastError();
                GET_PRINT_INSTANCE->print(L"\tSetAllocationSize: SetFilePointer eror: %d, "
                         L"offset = %I64d\n\n",
                         error, AllocSize);
                return DokanNtStatusFromWin32(error);
            }
            if (!SetEndOfFile(handle)) {
                DWORD error = GetLastError();
                GET_PRINT_INSTANCE->print(L"\tSetEndOfFile error code = %d\n\n", error);
                return DokanNtStatusFromWin32(error);
            }
        }
    } else {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorSetFileAttributes(LPCWSTR FileName, DWORD FileAttributes, PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    WCHAR filePath[DOKAN_MAX_PATH];

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"SetFileAttributes %s 0x%x\n", filePath, FileAttributes);

    if (FileAttributes != 0) {
        if (!SetFileAttributes(filePath, FileAttributes)) {
            DWORD error = GetLastError();
            GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
            return DokanNtStatusFromWin32(error);
        }
    } else {
        // case FileAttributes == 0 :
        // MS-FSCC 2.6 File Attributes : There is no file attribute with the value 0x00000000
        // because a value of 0x00000000 in the FileAttributes field means that the file attributes for this file MUST NOT be changed when setting basic information for the file
        GET_PRINT_INSTANCE->print(L"Set 0 to FileAttributes means MUST NOT be changed. Didn't call "
                 L"SetFileAttributes function. \n");
    }

    GET_PRINT_INSTANCE->print(L"\n");
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorSetFileTime(LPCWSTR FileName, CONST FILETIME *CreationTime, CONST FILETIME *LastAccessTime, CONST FILETIME *LastWriteTime,PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"SetFileTime %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;

    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    if (!SetFileTime(handle, CreationTime, LastAccessTime, LastWriteTime)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    GET_PRINT_INSTANCE->print(L"\n");
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorUnlockFile(LPCWSTR FileName, LONGLONG ByteOffset, LONGLONG Length, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    HANDLE handle;
    LARGE_INTEGER length;
    LARGE_INTEGER offset;

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"UnlockFile %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    length.QuadPart = Length;
    offset.QuadPart = ByteOffset;

    if (!UnlockFile(handle, offset.LowPart, offset.HighPart, length.LowPart,
                    length.HighPart)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"\terror code = %d\n\n", error);
        return DokanNtStatusFromWin32(error);
    }

    GET_PRINT_INSTANCE->print(L"\tsuccess\n\n");
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorGetVolumeInformation(
        LPWSTR VolumeNameBuffer, DWORD VolumeNameSize, LPDWORD VolumeSerialNumber,
        LPDWORD MaximumComponentLength, LPDWORD FileSystemFlags,
        LPWSTR FileSystemNameBuffer, DWORD FileSystemNameSize,
        PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    WCHAR volumeRoot[4];
    DWORD fsFlags = 0;

    wcscpy_s(VolumeNameBuffer, VolumeNameSize, GET_GLOBALS_INSTANCE->volname().c_str());

    if (VolumeSerialNumber)
        *VolumeSerialNumber = 0x19831116;
    if (MaximumComponentLength)
        *MaximumComponentLength = 255;
    if (FileSystemFlags) {
        *FileSystemFlags = FILE_SUPPORTS_REMOTE_STORAGE | FILE_UNICODE_ON_DISK |
                FILE_PERSISTENT_ACLS | FILE_NAMED_STREAMS;
        if (GET_GLOBALS_INSTANCE->CaseSensitive())
            *FileSystemFlags = FILE_CASE_SENSITIVE_SEARCH | FILE_CASE_PRESERVED_NAMES;
    }

    volumeRoot[0] = GET_GLOBALS_INSTANCE->RootDirectory()[0]; //RootDirectory[0];
    volumeRoot[1] = ':';
    volumeRoot[2] = '\\';
    volumeRoot[3] = '\0';

    if (GetVolumeInformation(volumeRoot, NULL, 0, NULL, MaximumComponentLength,
                             &fsFlags, FileSystemNameBuffer,
                             FileSystemNameSize)) {

        if (FileSystemFlags)
            *FileSystemFlags &= fsFlags;

        if (MaximumComponentLength) {
            GET_PRINT_INSTANCE->print(L"GetVolumeInformation: max component length %u\n",
                     *MaximumComponentLength);
        }
        if (FileSystemNameBuffer) {
            GET_PRINT_INSTANCE->print(L"GetVolumeInformation: file system name %s\n",
                     FileSystemNameBuffer);
        }
        if (FileSystemFlags) {
            GET_PRINT_INSTANCE->print(L"GetVolumeInformation: got file system flags 0x%08x,"
                     L" returning 0x%08x\n",
                     fsFlags, *FileSystemFlags);
        }
    } else {

        GET_PRINT_INSTANCE->print(L"GetVolumeInformation: unable to query underlying fs,"
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



NTSTATUS DOKAN_CALLBACK FileOps::MirrorDokanGetDiskFreeSpace(
        PULONGLONG FreeBytesAvailable, PULONGLONG TotalNumberOfBytes,
        PULONGLONG TotalNumberOfFreeBytes, PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);
    WCHAR DriveLetter[3] = {'C', ':', 0};
    PWCHAR RootPathName;
    std::wstring rootdir = GET_GLOBALS_INSTANCE->RootDirectory();

    if (rootdir[0] == L'\\') { // UNC as Root
        RootPathName = &rootdir[0];
    } else {
        DriveLetter[0] = rootdir[0];
        RootPathName = DriveLetter;
    }

    if (!GetDiskFreeSpaceExW(RootPathName, (PULARGE_INTEGER)FreeBytesAvailable,
                             (PULARGE_INTEGER)TotalNumberOfBytes,
                             (PULARGE_INTEGER)TotalNumberOfFreeBytes)) {
        DWORD error = GetLastError();
        GET_PRINT_INSTANCE->print(L"GetDiskFreeSpaceEx failed for path %ws", RootPathName);
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

NTSTATUS DOKAN_CALLBACK FileOps::MirrorFindStreams(LPCWSTR FileName, PFillFindStreamData FillFindStreamData,PVOID FindStreamContext,
                  PDOKAN_FILE_INFO DokanFileInfo) {

    UNREFERENCED_PARAMETER(DokanFileInfo);

     WCHAR filePath[DOKAN_MAX_PATH];
     HANDLE hFind;
     WIN32_FIND_STREAM_DATA findData;
     DWORD error;
     int count = 0;

     GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

     GET_PRINT_INSTANCE->print(L"FindStreams :%s\n", filePath);

     hFind = FindFirstStreamW(filePath, FindStreamInfoStandard, &findData, 0);

     if (hFind == INVALID_HANDLE_VALUE) {
       error = GetLastError();
       GET_PRINT_INSTANCE->print(L"\tinvalid file handle. Error is %u\n\n", error);
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
       GET_PRINT_INSTANCE->print(L"\tFindStreams returned %d entries in %s with "
                L"STATUS_BUFFER_OVERFLOW\n\n",
                count, filePath);
       // https://msdn.microsoft.com/en-us/library/windows/hardware/ff540364(v=vs.85).aspx
       return STATUS_BUFFER_OVERFLOW;
     }

     if (error != ERROR_HANDLE_EOF) {
       GET_PRINT_INSTANCE->print(L"\tFindNextStreamW error. Error is %u\n\n", error);
       return DokanNtStatusFromWin32(error);
     }

     GET_PRINT_INSTANCE->print(L"\tFindStreams return %d entries in %s\n\n", count, filePath);

     return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorMounted(LPCWSTR MountPoint , PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"Mounted\n");
    return STATUS_SUCCESS;
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorUnmounted(PDOKAN_FILE_INFO DokanFileInfo) {
    UNREFERENCED_PARAMETER(DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"Unmounted\n");
    return STATUS_SUCCESS;
}
