//#include <windows.h>
//#include <AclAPI.h>

#include "WinSec.h"
#include <sddl.h>
#include <AccCtrl.h>
#include <AclAPI.h>


#define ROUND_DOWN(n, align) (((ULONG)n) & ~((align) - 1l))
#define ROUND_UP(n, align) ROUND_DOWN(((ULONG)n) + (align) - 1, (align))


WinSec::WinSec(std::shared_ptr<DbgPrint> print)
{
    m_print = print;
}

NTSTATUS WinSec::RtlCreateSecurityDescriptorRelative(_In_ PISECURITY_DESCRIPTOR_RELATIVE SecurityDescriptor, _In_ ULONG Revision)
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


NTSTATUS WinSec::RtlpSetSecurityObject(_In_opt_ PVOID Object,
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

    m_print->print(L"RtlpSetSecurityObject()\n");

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
        m_print->print(L"New security descriptor allocation failed (Status 0x%08lx)\n", Status);
        if (pNewSd != NULL)
            HeapFree(GetProcessHeap(), 0, pNewSd);
    }

    /* Initialize the new security descriptor */
    Status = RtlCreateSecurityDescriptorRelative(pNewSd, SECURITY_DESCRIPTOR_REVISION);
    if (Status != ERROR_SUCCESS)    {
        m_print->print(L"New security descriptor creation failed (Status 0x%08lx)\n", Status);
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


ULONG WinSec::BOOL_TO_ERROR(BOOL f)
{
    return f ? 0 : GetLastError();
}

ULONG WinSec::GetNotElevatedDefaultDacl(PACL* DefaultDacl)
{
    HANDLE hToken;
    ULONG err = BOOL_TO_ERROR(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken));

    if (!err)
    {
        ULONG cb;
        union {
            TOKEN_LINKED_TOKEN tlt;
            TOKEN_ELEVATION_TYPE tet;
        };

        err = BOOL_TO_ERROR(GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(tet), &cb));

        if (!err)
        {
            if (tet == TokenElevationTypeFull)
            {
                err = BOOL_TO_ERROR(GetTokenInformation(hToken, TokenLinkedToken, &tlt, sizeof(tlt), &cb));
            }
            else
            {
                err = ERROR_ELEVATION_REQUIRED;
            }
        }
        CloseHandle(hToken);

        if (!err)
        {
            PTOKEN_DEFAULT_DACL p;
            DWORD needsize;

            GetTokenInformation(tlt.LinkedToken, TokenDefaultDacl, NULL, 0, &needsize);

            p = (PTOKEN_DEFAULT_DACL) LocalAlloc(LPTR,needsize);

            if(!GetTokenInformation(tlt.LinkedToken, TokenDefaultDacl, p, needsize, &needsize)){
                m_print->print(L"Get Token information Error %d\n",GetLastError());
                return GetLastError();
            }

            *DefaultDacl = (PACL)LocalAlloc(LPTR,p->DefaultDacl->AclSize);
            memcpy(*DefaultDacl,p->DefaultDacl,p->DefaultDacl->AclSize);

            m_print->print(L"Is DACL valid  %d\n" ,IsValidAcl(*DefaultDacl));

            LocalFree(p);

            CloseHandle(tlt.LinkedToken);
        }
    }

    return err;
}

ULONG WinSec::GetNotElevatedSIDS(PSID *owner, PSID *group)
{
    HANDLE hToken;
    ULONG err = BOOL_TO_ERROR(OpenProcessToken(GetCurrentProcess(), TOKEN_QUERY, &hToken));

    if (!err)
    {
        ULONG cb;
        union {
            TOKEN_LINKED_TOKEN tlt;
            TOKEN_ELEVATION_TYPE tet;
        };

        err = BOOL_TO_ERROR(GetTokenInformation(hToken, TokenElevationType, &tet, sizeof(tet), &cb));

        if (!err)
        {
            if (tet == TokenElevationTypeFull)
            {
                err = BOOL_TO_ERROR(GetTokenInformation(hToken, TokenLinkedToken, &tlt, sizeof(tlt), &cb));
            }
            else
            {
                err = ERROR_ELEVATION_REQUIRED;
            }
        }
        CloseHandle(hToken);

        if (!err)
        {


            DWORD needsize;
            PTOKEN_USER ptowner;
            GetTokenInformation(tlt.LinkedToken, TokenUser , NULL, 0 , &needsize);
            ptowner = (PTOKEN_USER) LocalAlloc(LPTR,needsize);



            if(GetTokenInformation(tlt.LinkedToken, TokenUser , ptowner, needsize , &needsize)==0){
                m_print->print(L"Get Token information Error %d\n",GetLastError());
                return GetLastError();
            }

            DWORD sidl = GetLengthSid(ptowner->User.Sid);
            *owner = LocalAlloc(LPTR,sidl);
            CopySid(sidl,*owner,ptowner->User.Sid);
            LocalFree(ptowner);


            LPWSTR ssid=NULL;
            ConvertSidToStringSidW(*owner,&ssid);

            m_print->print(L"owner sid %s\n", ssid);

            needsize = 0;
            GetTokenInformation(tlt.LinkedToken, TokenPrimaryGroup, NULL, 0 , &needsize);
            PTOKEN_PRIMARY_GROUP tpgroup;

            tpgroup = (PTOKEN_PRIMARY_GROUP)LocalAlloc(LPTR , needsize);

            err = BOOL_TO_ERROR(GetTokenInformation(tlt.LinkedToken, TokenPrimaryGroup, tpgroup, needsize , &needsize));
            LocalFree(ssid);

            sidl = GetLengthSid(tpgroup->PrimaryGroup);
            *group = LocalAlloc(LPTR,sidl);
            CopySid(sidl,*group,tpgroup->PrimaryGroup);
            LocalFree(tpgroup);


            ConvertSidToStringSid(*group,&ssid);

            m_print->print(L"group sid %s\n", ssid);

            LocalFree(ssid);

            CloseHandle(tlt.LinkedToken);
        }
    }

    return err;
}

NTSTATUS WinSec::CreateDefaultSelfRelativeSD(PSECURITY_DESCRIPTOR *SecurityDescriptor)
{
    PSID owner=NULL, group=NULL;

    PACL dacl;
    ULONG numofaclEntries;

    GetNotElevatedDefaultDacl(&dacl);

    PEXPLICIT_ACCESS_W peacces;

    ULONG err;

    err = GetExplicitEntriesFromAclW(dacl,&numofaclEntries,&peacces);

    if(err!=ERROR_SUCCESS){
        return err;
    }

    err =  GetNotElevatedSIDS(&owner, &group);
    m_print->print(L"Get sids, %d \n",err);

    LPWSTR ssid2 = NULL;
    ConvertSidToStringSidW(owner, &ssid2);
    m_print->print(L"--owner sid %s\n", ssid2);

    BOOL valid = IsValidSid(owner);
    m_print->print(L"Owner sid valid %d \n",valid);

    valid = IsValidSid(group);
    m_print->print(L"Group sid valid %d \n",valid);

    TRUSTEEW towner,tgroup;
    BuildTrusteeWithSidW (&towner,owner);
    BuildTrusteeWithSidW(&tgroup,group);

    //    LocalFree(owner);
    //    LocalFree(group);

    ULONG sdsize;
    PSECURITY_DESCRIPTOR lcsd=nullptr;;
    DWORD error =  BuildSecurityDescriptorW(&towner,&tgroup,numofaclEntries,peacces,0,NULL,NULL,&sdsize,&lcsd);
    if(error != ERROR_SUCCESS){
        m_print->print(L"ERROR BuildSecurityDescriptorW, %d\n",error);
    }

    valid = IsValidSecurityDescriptor(lcsd);
    m_print->print(L"VAlid lcsd, %d \n", valid);

    *SecurityDescriptor = lcsd;

    printSD(lcsd,-1);

    //    HANDLE pHeap = GetProcessHeap();
    //    *SecurityDescriptor = HeapAlloc(GetProcessHeap(), 0, sdsize);
    //    memcpy(*SecurityDescriptor , lcsd , sdsize);

    //    valid = IsValidSecurityDescriptor(*SecurityDescriptor);
    //    m_print->print(L"VAlid heapSecurityDescriptor, %d \n", valid);

    //    LocalFree(lcsd);

    //    valid = IsValidSecurityDescriptor(*SecurityDescriptor);
    //    m_print->print(L"VAlid 2 heapSecurityDescriptor, %d \n", valid);



    //-----------------------------------------------------
    //-----------------------------------------------------

    LPWSTR ssid=NULL;
    PSID owner2 = NULL;
    BOOL ownerDefaulted;

    owner2 = NULL;
    ownerDefaulted = FALSE;
    GetSecurityDescriptorOwner(*SecurityDescriptor,&owner2,&ownerDefaulted);
    if(owner2==NULL){
        m_print->print(L"owner22 NULL");
    }else{
        valid = IsValidSid(owner2);
        m_print->print(L"Owner22 sid valid %d \n",valid);
        ConvertSidToStringSid(owner2,&ssid);
        m_print->print(L"owner22 sid %s\n", ssid);
    }

    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD sdc_version;

    GetSecurityDescriptorControl(*SecurityDescriptor,&sdc,&sdc_version);

    if(sdc & SE_SELF_RELATIVE){
        m_print->print(L"SecurityDescriptor self relative \n");
    }else{
        m_print->print(L"SecurityDescriptor absolute \n");
    }

    //    LocalFree(ssid);
    //    LocalFree(owner2);


    return ERROR_SUCCESS;

}

void WinSec::printSD(PSECURITY_DESCRIPTOR sd, int num)
{
    LPWSTR str;

    BOOL valid = IsValidSecurityDescriptor(sd);
    if(valid){
        ConvertSecurityDescriptorToStringSecurityDescriptorW(sd,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |  PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION |  UNPROTECTED_SACL_SECURITY_INFORMATION   , &str , NULL);
        m_print->print(L"Security Descriptor %d string %s\n",num,str);
    }else{
        m_print->print(L"Security Descriptor %d INVALID\n",num);
    }

    LocalFree(str);
}

bool WinSec::isSelfRelative(PSECURITY_DESCRIPTOR sd)
{
    SECURITY_DESCRIPTOR_CONTROL sdc;
    DWORD sdc_version;
    GetSecurityDescriptorControl(sd,&sdc,&sdc_version);

    return sdc & SE_SELF_RELATIVE;
}


