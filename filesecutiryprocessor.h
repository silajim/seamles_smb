#ifndef FILESECUTIRYPROCESSOR_H
#define FILESECUTIRYPROCESSOR_H

#include <windows.h>
#include <handleapi.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#include <libloaderapi.h>
#include <dokan/dokan.h>
#include "ramdiskmanager.h"

class FileSecutiryProcessor
{
public:
    FileSecutiryProcessor(std::string cacheDrive);
    NTSTATUS GetSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation,  PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo , PSECURITY_DESCRIPTOR cacheDescriptor =nullptr, unsigned long cDescriptorSzie=0);
    NTSTATUS SetSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo , PSECURITY_DESCRIPTOR fuldescr , PSECURITY_DESCRIPTOR cacheDescriptor =nullptr);

private:
    RamDiskManager *ramManager=nullptr;
};

#endif // FILESECUTIRYPROCESSOR_H
