#ifndef WINSEC_H
#define WINSEC_H

#include <windows.h>
//#include <winbase.h>
#include <sddl.h>
//#include <Psapi.h>
#include "DbgPrint.h"

class WinSec{
public:

    WinSec(std::shared_ptr<DbgPrint> print);

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

private:
    std::shared_ptr<DbgPrint> m_print;
    NTSTATUS RtlCreateSecurityDescriptorRelative(PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor, ULONG Revision);
    ULONG BOOL_TO_ERROR(BOOL f);
    ULONG GetNotElevatedDefaultDacl(PACL *DefaultDacl);
};

#endif // WINSEC_H
