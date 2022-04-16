#include "filesecutiryprocessor.h"
#include "ramdiskmanager.h"
#include <string>
#include <locale>
#include <codecvt>

FileSecutiryProcessor::FileSecutiryProcessor(std::string cacheDrive)
{
    ramManager = new RamDiskManager(cacheDrive);
}

NTSTATUS FileSecutiryProcessor::GetSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo, PSECURITY_DESCRIPTOR cacheDescriptor, unsigned long cDescriptorSzie)
{
    std::string tempath;
    if(DokanFileInfo->IsDirectory){
        tempath = ramManager->createdirectory();
    }else{
        tempath = ramManager->createFile();
    }

    std::wstring wtemp(tempath.begin(),tempath.end());

    if(cacheDescriptor!=nullptr){
        SetFileSecurityW(wtemp.c_str(),BACKUP_SECURITY_INFORMATION,cacheDescriptor);
    }

    GetFileSecurityW(wtemp.c_str() , *SecurityInformation,NULL,0,LengthNeeded);
    if(*LengthNeeded>BufferLength){
        ramManager->deleteFile(tempath);
        return STATUS_BUFFER_OVERFLOW;
    }

    if(!GetFileSecurityW(wtemp.c_str() , *SecurityInformation,SecurityDescriptor,BufferLength,LengthNeeded)){
        ramManager->deleteFile(tempath);
        return GetLastError();
    }
    ramManager->deleteFile(tempath);
    return STATUS_SUCCESS;

}

NTSTATUS FileSecutiryProcessor::SetSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo, PSECURITY_DESCRIPTOR fuldescr, PSECURITY_DESCRIPTOR cacheDescriptor)
{
    std::string tempath;
    if(DokanFileInfo->IsDirectory){
        tempath = ramManager->createdirectory();
    }else{
        tempath = ramManager->createFile();
    }

    std::wstring wtemp(tempath.begin(),tempath.end());

    if(cacheDescriptor!=nullptr){
        SetFileSecurityW(wtemp.c_str(),BACKUP_SECURITY_INFORMATION,cacheDescriptor);
    }

    if(!SetFileSecurityW(wtemp.c_str(),*SecurityInformation,SecurityDescriptor)){
        ramManager->deleteFile(tempath);
        return GetLastError();
    }

    unsigned long lneeded;
    GetFileSecurityW(wtemp.c_str() , BACKUP_SECURITY_INFORMATION ,NULL,0,&lneeded);
    PSECURITY_DESCRIPTOR psd=nullptr;
    psd = (PSECURITY_DESCRIPTOR)LocalAlloc(LPTR, lneeded);
    GetFileSecurityW(wtemp.c_str() , BACKUP_SECURITY_INFORMATION ,psd,lneeded,&lneeded);

    fuldescr = psd;
    ramManager->deleteFile(tempath);
    return STATUS_SUCCESS;
}
