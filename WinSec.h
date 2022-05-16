#ifndef WINSEC_H
#define WINSEC_H

#include <windows.h>
//#include <winbase.h>
#include <sddl.h>
//#include <Psapi.h>

NTSTATUS RtlpSetSecurityObject(_In_opt_ PVOID Object,
                      _In_ SECURITY_INFORMATION SecurityInformation,
                      _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
                      _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                      _In_ ULONG AutoInheritFlags,
                      _In_ PGENERIC_MAPPING GenericMapping,
                      _In_ HANDLE Token OPTIONAL);

ULONG GetNotElevatedSIDS(PSID *owner, PSID *group);

NTSTATUS CreateDefaultSelfRelativeSD(PSECURITY_DESCRIPTOR *SecurityDescriptor);

void printSD(PSECURITY_DESCRIPTOR sd, int num=0);

bool isSelfRelative(PSECURITY_DESCRIPTOR sd);

#endif // WINSEC_H
