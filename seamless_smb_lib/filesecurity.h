#ifndef FILESECURITY_H
#define FILESECURITY_H

#include <windows.h>
#include <sddl.h>

#include "dokan/dokan.h"
#include "dokan/fileinfo.h"

//NTSTATUS DOKAN_CALLBACK MirrorGetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation,  PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG BufferLength, PULONG LengthNeeded, PDOKAN_FILE_INFO DokanFileInfo);
//NTSTATUS DOKAN_CALLBACK MirrorSetFileSecurity(LPCWSTR FileName, PSECURITY_INFORMATION SecurityInformation, PSECURITY_DESCRIPTOR SecurityDescriptor, ULONG SecurityDescriptorLength, PDOKAN_FILE_INFO DokanFileInfo);

#endif // FILESECURITY_H
