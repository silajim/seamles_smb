#include <windows.h>
#include <sddl.h>

#include "dokan/dokan.h"
//#include "dokan/fileinfo.h"


//#include "filesecurity.h"
#include "DbgPrint.h"
#include "nodes.h"
#include "WinSec.h"
#include "globals.h"

#include "fileops.h"

NTSTATUS DOKAN_CALLBACK FileOps::MirrorGetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation,  PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    BOOLEAN requestingSaclInfo;
    auto filenodes = GET_FS_INSTANCE;

    UNREFERENCED_PARAMETER(DokanFileInfo);

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"GetFileSecurity %s\n", filePath);

    //    GET_PRINT_INSTANCE->print(L"  Opening new handle with READ_CONTROL access\n");

    CheckFlag(*SecurityInformation, OWNER_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, GROUP_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, SACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, LABEL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, SCOPE_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, ACCESS_FILTER_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, BACKUP_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROTECTED_DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROTECTED_SACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, UNPROTECTED_DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, UNPROTECTED_SACL_SECURITY_INFORMATION);

    std::wstring strFileName(filePath);
    bool changed = false;

    auto filename_str = std::wstring(FileName);
    std::shared_ptr<filenode> f;
    f= filenodes->findFileNode(filename_str);

    DWORD status = STATUS_SUCCESS;

    if(f){
        LPTSTR pStringBuffer = NULL;


        PSECURITY_DESCRIPTOR heapSecurityDescriptor = nullptr;

        f->security.GetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,&heapSecurityDescriptor);



        GET_WINSEC_INSTANCE->printSD(heapSecurityDescriptor,4);

        ULONG sizeneeded = 0;

        GetPrivateObjectSecurity(heapSecurityDescriptor,*SecurityInformation,NULL,0,&sizeneeded);

        if(sizeneeded>BufferLength){
            *LengthNeeded = sizeneeded;
            return STATUS_BUFFER_OVERFLOW;
        }


        if(!GetPrivateObjectSecurity(heapSecurityDescriptor,*SecurityInformation,SecurityDescriptor,BufferLength,&sizeneeded)){
            GET_PRINT_INSTANCE->print(L"GetPrivateObjectSecurity Error %d\n",GetLastError());
            return GetLastError();
        }



        LocalFree(heapSecurityDescriptor);
        //         LocalFree(newsd);

        GET_WINSEC_INSTANCE->printSD(SecurityDescriptor,5);
        *LengthNeeded = sizeneeded;

        status = STATUS_SUCCESS;
        return status;

    }else{

        PSECURITY_DESCRIPTOR newSecurityDescriptor = NULL;

        GET_WINSEC_INSTANCE->CreateDefaultSelfRelativeSD(&newSecurityDescriptor);
        DWORD sizeneeded;

        GetPrivateObjectSecurity(newSecurityDescriptor,*SecurityInformation,NULL,0,&sizeneeded);

        if(sizeneeded>BufferLength){
            *LengthNeeded = sizeneeded;
            return STATUS_BUFFER_OVERFLOW;
        }

        if(!GetPrivateObjectSecurity(newSecurityDescriptor,*SecurityInformation,SecurityDescriptor,BufferLength,&sizeneeded)){
            GET_PRINT_INSTANCE->print(L"GetPrivateObjectSecurity Error %d\n",GetLastError());
            return GetLastError();
        }

        GET_WINSEC_INSTANCE->printSD(SecurityDescriptor,0);


        status = STATUS_SUCCESS;

        LocalFree(newSecurityDescriptor);


        // Ensure the Security Descriptor Length is set
        //    DWORD securityDescriptorLength = GetSecurityDescriptorLength(SecurityDescriptor);
        //    GET_PRINT_INSTANCE->print(L"  GetUserObjectSecurity return true,  *LengthNeeded = securityDescriptorLength \n");

        *LengthNeeded = sizeneeded;

        //    CloseHandle(handle);

        return status;
    }
}

NTSTATUS DOKAN_CALLBACK FileOps::MirrorSetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo) {
    HANDLE handle;
    WCHAR filePath[DOKAN_MAX_PATH];
    auto filenodes = GET_FS_INSTANCE;


    GetFilePath(filePath, DOKAN_MAX_PATH, FileName,DokanFileInfo);

    GET_PRINT_INSTANCE->print(L"SetFileSecurity %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        GET_PRINT_INSTANCE->print(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    CheckFlag(*SecurityInformation, OWNER_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, GROUP_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, SACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, LABEL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, SCOPE_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, ACCESS_FILTER_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, BACKUP_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROTECTED_DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, PROTECTED_SACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, UNPROTECTED_DACL_SECURITY_INFORMATION);
    CheckFlag(*SecurityInformation, UNPROTECTED_SACL_SECURITY_INFORMATION);

    SECURITY_DESCRIPTOR_RELATIVE *cast = (SECURITY_DESCRIPTOR_RELATIVE*)SecurityDescriptor;
    SECURITY_DESCRIPTOR *cast2 = (SECURITY_DESCRIPTOR*)SecurityDescriptor;

    //    if(*SecurityInformation & DACL_SECURITY_INFORMATION){
    //        PACL acl=NULL;
    //        SecurityProcessor sp;
    //        sp.getAllData(SecurityDescriptor);
    //    }

    auto filename_str = std::wstring(FileName);
    //    std::lock_guard<std::mutex> lk(filenodes->m_mutex);
    std::shared_ptr<filenode> f;
    f= filenodes->findFileNode(filename_str);

    if(f){
        std::lock_guard<std::mutex> securityLock(f->_data_mutex);

        // SetPrivateObjectSecurity - ObjectsSecurityDescriptor
        // The memory for the security descriptor must be allocated from the process
        // heap (GetProcessHeap) with the HeapAlloc function.
        // https://devblogs.microsoft.com/oldnewthing/20170727-00/?p=96705


        //        HANDLE pHeap = GetProcessHeap();
        //        DWORD descSize =  f->security.descriptor_size == 0 ? SecurityDescriptorLength : f->security.descriptor_size;
        PSECURITY_DESCRIPTOR heapSecurityDescriptor; //= HeapAlloc(pHeap, 0, descSize);
        f->security.GetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,&heapSecurityDescriptor);
        //        if (!heapSecurityDescriptor)
        //            return STATUS_INSUFFICIENT_RESOURCES;

        // Copy our current descriptor into heap memory
        //        if(f->security.descriptor_size!=0){
        ////            SecurityProcessor sp;
        ////            sp.generateDescriptor(heapSecurityDescriptor,f->security.strdescriptor);
        //            heapSecurityDescriptor = HeapAlloc(GetProcessHeap(),0,f->security.descriptor_size);
        //            memcpy(heapSecurityDescriptor,f->security.descriptor.get(),f->security.descriptor_size);
        //        }else{
        //            heapSecurityDescriptor = HeapAlloc(pHeap, 0, descSize);
        //            InitializeSecurityDescriptor(heapSecurityDescriptor,SECURITY_DESCRIPTOR_REVISION);
        //        }



        static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE,
                                                FILE_GENERIC_EXECUTE,
                                                FILE_ALL_ACCESS};

        NTSTATUS stat;

//        stat = GET_WINSEC_INSTANCE->RtlpSetSecurityObject(NULL,*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor, 0 ,&memfs_mapping, 0);
        SetPrivateObjectSecurity(*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor, &memfs_mapping, 0);
        GET_WINSEC_INSTANCE->printSD(SecurityDescriptor,6);

        if(stat != ERROR_SUCCESS){
            LocalFree(heapSecurityDescriptor);
            return DokanNtStatusFromWin32(GetLastError());
        }

        f->security.SetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,heapSecurityDescriptor);
        LocalFree(heapSecurityDescriptor);

        return STATUS_SUCCESS;

    }else{

        PSECURITY_DESCRIPTOR heapSecurityDescriptor = NULL;

        GET_WINSEC_INSTANCE->CreateDefaultSelfRelativeSD(&heapSecurityDescriptor);

        static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE,
                                                FILE_GENERIC_EXECUTE,
                                                FILE_ALL_ACCESS};
        GET_WINSEC_INSTANCE->printSD(SecurityDescriptor,2);

        NTSTATUS stat;

        if(SetPrivateObjectSecurity(*SecurityInformation,SecurityDescriptor,&heapSecurityDescriptor, &memfs_mapping, 0)==0){
            GET_PRINT_INSTANCE->print(L"  SetUserObjectSecurity2 error: %d\n", stat);
            LocalFree(heapSecurityDescriptor);
            return DokanNtStatusFromWin32(GetLastError());
        }

        PSID owner2=NULL;
        BOOL ownerDefaulted=0;
        BOOL valid;
        LPWSTR ssid=NULL;

        GetSecurityDescriptorOwner(heapSecurityDescriptor,&owner2,&ownerDefaulted);
        if(owner2==NULL){
            GET_PRINT_INSTANCE->print(L"after owner22 NULL");
        }else{
            valid = IsValidSid(owner2);
            GET_PRINT_INSTANCE->print(L"after Owner22 sid valid %d \n",valid);
            ConvertSidToStringSid(owner2,&ssid);
            GET_PRINT_INSTANCE->print(L"after owner22 sid %s\n", ssid);
        }

        GET_WINSEC_INSTANCE->printSD(heapSecurityDescriptor,3);


//        std::lock_guard<std::mutex> lg (filenodes->m_mutex);

        auto fil = std::make_shared<filenode>(filename_str, DokanFileInfo->IsDirectory, 0, nullptr);
        fil->security.SetDescriptor(GET_WINSEC_INSTANCE,GET_PRINT_INSTANCE,heapSecurityDescriptor);
        filenodes->addFileNode(fil);
//        filenodes->_filenodes->emplace(filename_str,fil);

        LocalFree(heapSecurityDescriptor);

    }
    return STATUS_SUCCESS;
}
