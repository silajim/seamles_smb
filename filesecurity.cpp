#include "filesecurity.h"
#include "DbgPrint.h"
#include "context.h"
#include "WinSec.h"
#include "globals.h"


#define GET_FS_INSTANCE \
  reinterpret_cast<Context*>(DokanFileInfo->DokanOptions->GlobalContext)

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

#define MirrorCheckFlag(val, flag)                                             \
    if (val & flag) {                                                            \
    DbgPrint(L"\t" L#flag L"\n");                                              \
    }

NTSTATUS DOKAN_CALLBACK MirrorGetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation,  PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo) {
    WCHAR filePath[DOKAN_MAX_PATH];
    BOOLEAN requestingSaclInfo;
    auto filenodes = GET_FS_INSTANCE;

    UNREFERENCED_PARAMETER(DokanFileInfo);

    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"GetFileSecurity %s\n", filePath);

//    DbgPrint(L"  Opening new handle with READ_CONTROL access\n");

    MirrorCheckFlag(*SecurityInformation, OWNER_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, GROUP_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, SACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, LABEL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, SCOPE_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, ACCESS_FILTER_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, BACKUP_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROTECTED_DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROTECTED_SACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_SACL_SECURITY_INFORMATION);

    std::wstring strFileName(filePath);
    bool changed = false;

    auto filename_str = std::wstring(FileName);
//    std::lock_guard<std::mutex> lk(filenodes->m_mutex);
    filenodes->m_mutex.lock();
    auto fit = filenodes->_filenodes->find(filename_str);
    std::shared_ptr<filenode> f;
    f=  (fit != filenodes->_filenodes->end()) ? fit->second : nullptr;

    filenodes->m_mutex.unlock();

    DWORD status = STATUS_SUCCESS;

    if(f){
        LPTSTR pStringBuffer = NULL;


        PSECURITY_DESCRIPTOR heapSecurityDescriptor = nullptr;

        f->security.GetDescriptor(&heapSecurityDescriptor);



        printSD(heapSecurityDescriptor,4);

        ULONG sizeneeded = 0;

        GetPrivateObjectSecurity(heapSecurityDescriptor,*SecurityInformation,NULL,0,&sizeneeded);

        if(sizeneeded>BufferLength){
            *LengthNeeded = sizeneeded;
            return STATUS_BUFFER_OVERFLOW;
        }


        if(!GetPrivateObjectSecurity(heapSecurityDescriptor,*SecurityInformation,SecurityDescriptor,BufferLength,&sizeneeded)){
            DbgPrint(L"GetPrivateObjectSecurity Error %d\n",GetLastError());
            return GetLastError();
        }



        LocalFree(heapSecurityDescriptor);
        //         LocalFree(newsd);

        printSD(SecurityDescriptor,5);
        *LengthNeeded = sizeneeded;

        status = STATUS_SUCCESS;
        return status;

    }else{

        PSECURITY_DESCRIPTOR newSecurityDescriptor = NULL;

        CreateDefaultSelfRelativeSD(&newSecurityDescriptor);
        DWORD sizeneeded;

//        if(!GetPrivateObjectSecurity(SecurityDescriptor,*SecurityInformation,NULL,0,LengthNeeded)){
//            DbgPrint(L"GetPrivateObjectSecurity Error %d\n",GetLastError());
////            if(GetLastError())
//            return GetLastError();
//        }

        //----------------------------------------------
        //----------------------------------------------
        //----------------------------------------------

//        LPCWSTR str = L"O:S-1-5-21-1142882320-715474635-772293125-1001G:S-1-5-21-1142882320-715474635-772293125-513D:ARAI(D;;FA;;;S-1-5-5-0-505363)(A;;FA;;;SY)(A;;FA;;;S-1-5-21-1142882320-715474635-772293125-1001)";
//        PSECURITY_DESCRIPTOR nsd;
//        ConvertStringSecurityDescriptorToSecurityDescriptorW(str,SDDL_REVISION_1,&nsd,NULL);

//        static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE, FILE_GENERIC_EXECUTE, FILE_ALL_ACCESS};

//       if(SetPrivateObjectSecurity(DACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION ,nsd,&newSecurityDescriptor, &memfs_mapping, 0)==0){
//           DbgPrint(L"  --TEST SetUserObjectSecurity error: %d\n", stat);
//            LocalFree(newSecurityDescriptor);
//        return DokanNtStatusFromWin32(GetLastError());
//       }

        GetPrivateObjectSecurity(newSecurityDescriptor,*SecurityInformation,NULL,0,&sizeneeded);

        if(sizeneeded>BufferLength){
            *LengthNeeded = sizeneeded;
            return STATUS_BUFFER_OVERFLOW;
        }

        if(!GetPrivateObjectSecurity(newSecurityDescriptor,*SecurityInformation,SecurityDescriptor,BufferLength,&sizeneeded)){
            DbgPrint(L"GetPrivateObjectSecurity Error %d\n",GetLastError());
            return GetLastError();
        }

        printSD(SecurityDescriptor,0);


        status = STATUS_SUCCESS;

        LocalFree(newSecurityDescriptor);


    // Ensure the Security Descriptor Length is set
//    DWORD securityDescriptorLength = GetSecurityDescriptorLength(SecurityDescriptor);
//    DbgPrint(L"  GetUserObjectSecurity return true,  *LengthNeeded = securityDescriptorLength \n");

    *LengthNeeded = sizeneeded;

//    CloseHandle(handle);

    return status;
    }
}

NTSTATUS DOKAN_CALLBACK MirrorSetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo) {
    HANDLE handle;
    WCHAR filePath[DOKAN_MAX_PATH];
     auto filenodes = GET_FS_INSTANCE;


    GetFilePath(filePath, DOKAN_MAX_PATH, FileName);

    DbgPrint(L"SetFileSecurity %s\n", filePath);

    handle = (HANDLE)DokanFileInfo->Context;
    if (!handle || handle == INVALID_HANDLE_VALUE) {
        DbgPrint(L"\tinvalid handle\n\n");
        return STATUS_INVALID_HANDLE;
    }

    MirrorCheckFlag(*SecurityInformation, OWNER_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, GROUP_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, SACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, LABEL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, ATTRIBUTE_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, SCOPE_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROCESS_TRUST_LABEL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, ACCESS_FILTER_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, BACKUP_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROTECTED_DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, PROTECTED_SACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_DACL_SECURITY_INFORMATION);
    MirrorCheckFlag(*SecurityInformation, UNPROTECTED_SACL_SECURITY_INFORMATION);

    SECURITY_DESCRIPTOR_RELATIVE *cast = (SECURITY_DESCRIPTOR_RELATIVE*)SecurityDescriptor;
    SECURITY_DESCRIPTOR *cast2 = (SECURITY_DESCRIPTOR*)SecurityDescriptor;

//    if(*SecurityInformation & DACL_SECURITY_INFORMATION){
//        PACL acl=NULL;
//        SecurityProcessor sp;
//        sp.getAllData(SecurityDescriptor);
//    }

    auto filename_str = std::wstring(FileName);
//    std::lock_guard<std::mutex> lk(filenodes->m_mutex);
    filenodes->m_mutex.lock();
    auto fit = filenodes->_filenodes->find(filename_str);
    std::shared_ptr<filenode> f;
    f=  (fit != filenodes->_filenodes->end()) ? fit->second : nullptr;
    filenodes->m_mutex.unlock();

    if(f){
        std::lock_guard<std::mutex> securityLock(f->_data_mutex);

        // SetPrivateObjectSecurity - ObjectsSecurityDescriptor
        // The memory for the security descriptor must be allocated from the process
        // heap (GetProcessHeap) with the HeapAlloc function.
        // https://devblogs.microsoft.com/oldnewthing/20170727-00/?p=96705


//        HANDLE pHeap = GetProcessHeap();
//        DWORD descSize =  f->security.descriptor_size == 0 ? SecurityDescriptorLength : f->security.descriptor_size;
        PSECURITY_DESCRIPTOR heapSecurityDescriptor; //= HeapAlloc(pHeap, 0, descSize);
        f->security.GetDescriptor(&heapSecurityDescriptor);
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

        stat = RtlpSetSecurityObject(NULL,*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor, 0 ,&memfs_mapping, 0);
        if(stat != ERROR_SUCCESS){
            LocalFree(heapSecurityDescriptor);
//          HeapFree(pHeap, 0, heapSecurityDescriptor);
          return DokanNtStatusFromWin32(GetLastError());
        }

        f->security.SetDescriptor(heapSecurityDescriptor);
//        HeapFree(pHeap, 0, heapSecurityDescriptor);
        LocalFree(heapSecurityDescriptor);

        return STATUS_SUCCESS;

    }else{

        PSECURITY_DESCRIPTOR heapSecurityDescriptor = NULL;

        CreateDefaultSelfRelativeSD(&heapSecurityDescriptor);

        static GENERIC_MAPPING memfs_mapping = {FILE_GENERIC_READ, FILE_GENERIC_WRITE,
                                                FILE_GENERIC_EXECUTE,
                                                FILE_ALL_ACCESS};
        printSD(SecurityDescriptor,2);

        NTSTATUS stat;
//         stat = RtlpSetSecurityObject(NULL,*SecurityInformation, SecurityDescriptor,&heapSecurityDescriptor,0, &memfs_mapping, 0);
//         if(stat!=ERROR_SUCCESS){
//             DbgPrint(L"  SetUserObjectSecurity2 error: %d\n", stat);
////          HeapFree(GetProcessHeap(), 0, heapSecurityDescriptor);
//              LocalFree(heapSecurityDescriptor);
//          return DokanNtStatusFromWin32(GetLastError());
//        }

         if(SetPrivateObjectSecurity(*SecurityInformation,SecurityDescriptor,&heapSecurityDescriptor, &memfs_mapping, 0)==0){
             DbgPrint(L"  SetUserObjectSecurity2 error: %d\n", stat);
//          HeapFree(GetProcessHeap(), 0, heapSecurityDescriptor);
              LocalFree(heapSecurityDescriptor);
          return DokanNtStatusFromWin32(GetLastError());
         }

         PSID owner2=NULL;
         BOOL ownerDefaulted=0;
         BOOL valid;
         LPWSTR ssid=NULL;

         GetSecurityDescriptorOwner(heapSecurityDescriptor,&owner2,&ownerDefaulted);
         if(owner2==NULL){
             DbgPrint(L"after owner22 NULL");
         }else{
             valid = IsValidSid(owner2);
             DbgPrint(L"after Owner22 sid valid %d \n",valid);
             ConvertSidToStringSid(owner2,&ssid);
             DbgPrint(L"after owner22 sid %s\n", ssid);
         }

         printSD(heapSecurityDescriptor,3);



        filenodes->m_mutex.lock();

        auto fil = std::make_shared<filenode>(filename_str, DokanFileInfo->IsDirectory, 0, nullptr);
        fil->security.SetDescriptor(heapSecurityDescriptor);
        filenodes->_filenodes->emplace(filename_str,fil);

        filenodes->m_mutex.unlock();
//        HeapFree(GetProcessHeap(), 0, heapSecurityDescriptor);
        LocalFree(heapSecurityDescriptor);

//        return STATUS_SUCCESS;

//        return DokanNtStatusFromWin32(error);
    }
    return STATUS_SUCCESS;
}
