//#include <windows.h>
//#include <AclAPI.h>

#include "WinSec.h"

#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))

#include "DbgPrint.h"

NTSTATUS RtlCreateSecurityDescriptorRelative(_In_ PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor, _In_ ULONG Revision)
{

    /* Fail on invalid revisions */
    if (Revision != SECURITY_DESCRIPTOR_REVISION) return ERROR_UNKNOWN_REVISION;

    /* Setup an empty SD */
    RtlZeroMemory(SecurityDescriptor, sizeof(*SecurityDescriptor));
    SecurityDescriptor->Revision = SECURITY_DESCRIPTOR_REVISION;
    SecurityDescriptor->Control = SE_SELF_RELATIVE;

    /* All good */
    return ERROR_SUCCESS;
}


NTSTATUS RtlpSetSecurityObject(_In_opt_ PVOID Object,
                      _In_ SECURITY_INFORMATION SecurityInformation,
                      _In_ PSECURITY_DESCRIPTOR ModificationDescriptor,
                      _Inout_ PSECURITY_DESCRIPTOR *ObjectsSecurityDescriptor,
                      _In_ ULONG AutoInheritFlags,
                      _In_ PGENERIC_MAPPING GenericMapping,
                      _In_ HANDLE Token OPTIONAL)
{
    PISECURITY_DESCRIPTOR_RELATIVE pNewSd = NULL;
    PSID pOwnerSid = NULL;
    PSID pGroupSid = NULL;
    PACL pDacl = NULL;
    PACL pSacl = NULL;
    BOOL Defaulted;
    BOOL Present;
    ULONG ulOwnerSidSize = 0, ulGroupSidSize = 0;
    ULONG ulDaclSize = 0, ulSaclSize = 0;
    ULONG ulNewSdSize;
    SECURITY_DESCRIPTOR_CONTROL Control = SE_SELF_RELATIVE;
    PUCHAR pDest;
    NTSTATUS Status = ERROR_SUCCESS;

     DbgPrint(L"RtlpSetSecurityObject()\n");

    /* Change the Owner SID */
    if (SecurityInformation & OWNER_SECURITY_INFORMATION)
    {
        if(GetSecurityDescriptorOwner(ModificationDescriptor, &pOwnerSid, &Defaulted)==0)
            return GetLastError();
    }
    else
    {
        if(GetSecurityDescriptorOwner(*ObjectsSecurityDescriptor, &pOwnerSid, &Defaulted)==0)
            return GetLastError();
    }

    if (pOwnerSid == NULL || !IsValidSid(pOwnerSid))
        return ERROR_INVALID_OWNER;

    ulOwnerSidSize = GetLengthSid(pOwnerSid);

    /* Change the Group SID */
    if (SecurityInformation & GROUP_SECURITY_INFORMATION)
    {
        if(GetSecurityDescriptorGroup(ModificationDescriptor, &pGroupSid, &Defaulted)==0)
            return GetLastError();
    }
    else
    {
        if(GetSecurityDescriptorGroup(*ObjectsSecurityDescriptor, &pGroupSid, &Defaulted)==0)
            return GetLastError();
    }

    if (pGroupSid == NULL || !IsValidSid(pGroupSid))
        return ERROR_INVALID_PRIMARY_GROUP;

    ulGroupSidSize = GetLengthSid(pGroupSid);

    /* Change the DACL */
    if (SecurityInformation & DACL_SECURITY_INFORMATION)
    {
        if(GetSecurityDescriptorDacl(ModificationDescriptor, &Present, &pDacl, &Defaulted)==0)
            return GetLastError();

        Control |= SE_DACL_PRESENT;
    }
    else
    {
        if(GetSecurityDescriptorDacl(*ObjectsSecurityDescriptor, &Present, &pDacl, &Defaulted)==0)
            return GetLastError();

        if (Present)
            Control |= SE_DACL_PRESENT;

        if (Defaulted)
            Control |= SE_DACL_DEFAULTED;
    }

    if (pDacl != NULL)
        ulDaclSize = pDacl->AclSize;

    /* Change the SACL */
    if (SecurityInformation & SACL_SECURITY_INFORMATION)
    {
        if(GetSecurityDescriptorSacl(ModificationDescriptor, &Present, &pSacl, &Defaulted)==0)
            return GetLastError();


        Control |= SE_SACL_PRESENT;
    }
    else
    {
        if(GetSecurityDescriptorSacl(*ObjectsSecurityDescriptor, &Present, &pSacl, &Defaulted)==0)
            return GetLastError();


        if (Present)
            Control |= SE_SACL_PRESENT;

        if (Defaulted)
            Control |= SE_SACL_DEFAULTED;
    }

    if (pSacl != NULL)
        ulSaclSize = pSacl->AclSize;

    /* Calculate the size of the new security descriptor */
    ulNewSdSize = sizeof(SECURITY_DESCRIPTOR_RELATIVE) +
                  ROUND_UP(ulOwnerSidSize, sizeof(ULONG)) +
                  ROUND_UP(ulGroupSidSize, sizeof(ULONG)) +
                  ROUND_UP(ulDaclSize, sizeof(ULONG)) +
                  ROUND_UP(ulSaclSize, sizeof(ULONG));

    /* Allocate the new security descriptor */
    pNewSd = (PISECURITY_DESCRIPTOR_RELATIVE) HeapAlloc(GetProcessHeap(), 0, ulNewSdSize);
    if (pNewSd == NULL)
    {
        Status = ERROR_OUTOFMEMORY;
        DbgPrint(L"New security descriptor allocation failed (Status 0x%08lx)\n", Status);
        if (pNewSd != NULL)
            HeapFree(GetProcessHeap(), 0, pNewSd);
    }

    /* Initialize the new security descriptor */
    Status = RtlCreateSecurityDescriptorRelative(pNewSd, SECURITY_DESCRIPTOR_REVISION);
    if (Status != ERROR_SUCCESS)    {
        DbgPrint(L"New security descriptor creation failed (Status 0x%08lx)\n", Status);
        if (pNewSd != NULL)
            HeapFree(GetProcessHeap(), 0, pNewSd);
    }

    /* Set the security descriptor control flags */
    pNewSd->Control = Control;

    pDest = (PUCHAR)((ULONG_PTR)pNewSd + sizeof(SECURITY_DESCRIPTOR_RELATIVE));

    /* Copy the SACL */
    if (pSacl != NULL)
    {
        RtlCopyMemory(pDest, pSacl, ulSaclSize);
        pNewSd->Sacl = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
        pDest = pDest + ROUND_UP(ulSaclSize, sizeof(ULONG));
    }

    /* Copy the DACL */
    if (pDacl != NULL)
    {
        RtlCopyMemory(pDest, pDacl, ulDaclSize);
        pNewSd->Dacl = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
        pDest = pDest + ROUND_UP(ulDaclSize, sizeof(ULONG));
    }

    /* Copy the Owner SID */
    RtlCopyMemory(pDest, pOwnerSid, ulOwnerSidSize);
    pNewSd->Owner = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;
    pDest = pDest + ROUND_UP(ulOwnerSidSize, sizeof(ULONG));

    /* Copy the Group SID */
    RtlCopyMemory(pDest, pGroupSid, ulGroupSidSize);
    pNewSd->Group = (ULONG_PTR)pDest - (ULONG_PTR)pNewSd;

    /* Free the old security descriptor */
    HeapFree(GetProcessHeap(), 0, (PVOID)*ObjectsSecurityDescriptor);

    /* Return the new security descriptor */
    *ObjectsSecurityDescriptor = (PSECURITY_DESCRIPTOR)pNewSd;

//done:
    if (Status != ERROR_SUCCESS)
    {
        if (pNewSd != NULL)
            HeapFree(GetProcessHeap(), 0, pNewSd);
    }

    return Status;
}
