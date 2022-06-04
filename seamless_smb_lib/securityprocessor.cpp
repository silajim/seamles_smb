#include "securityprocessor.h"
//#include <Mq.h>
#include <cwchar>
#include <sddl.h>

SecurityProcessor::SecurityProcessor()
{

}

NTSTATUS  SecurityProcessor::getAllData(PSECURITY_DESCRIPTOR pSecurityDescriptor,std::wstring &strindsd)
{

    LPWSTR ssd;
    unsigned long size;
    if(!ConvertSecurityDescriptorToStringSecurityDescriptorW(pSecurityDescriptor,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |  PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION |  UNPROTECTED_SACL_SECURITY_INFORMATION  , &ssd , &size)){
        return GetLastError();
    }
    strindsd = std::wstring(ssd,size);

    return NO_ERROR;

    // Validate the input parameters.
    if (pSecurityDescriptor == NULL)
    {
        return ERROR_INVALID_PARAMETER;
    }
    LPCWSTR wszComputerName = NULL;

    PACL pDacl = NULL;
    ACL_SIZE_INFORMATION aclsizeinfo;
    ACCESS_ALLOWED_ACE * pAce = NULL;
    SID_NAME_USE eSidType;
    DWORD dwErrorCode = 0;
    NTSTATUS hr = NO_ERROR;

    // Create buffers that may be large enough.
    const DWORD INITIAL_SIZE = 256;
    DWORD cchAccName = 0;
    DWORD cchDomainName = 0;
    DWORD dwAccBufferSize = INITIAL_SIZE;
    DWORD dwDomainBufferSize = INITIAL_SIZE;
    DWORD cAce;
    WCHAR * wszAccName = NULL;
    WCHAR * wszDomainName = NULL;

    // Retrieve a pointer to the DACL in the security descriptor.
    BOOL fDaclPresent = FALSE;
    BOOL fDaclDefaulted = TRUE;
    if (GetSecurityDescriptorDacl(pSecurityDescriptor,           &fDaclPresent,          &pDacl,             &fDaclDefaulted ) == FALSE)
    {
        dwErrorCode = GetLastError();
        wprintf(L"GetSecurityDescriptorDacl failed. GetLastError returned: %d\n", dwErrorCode);
        return HRESULT_FROM_WIN32(dwErrorCode);
    }

    // Check whether no DACL or a NULL DACL was retrieved from the security descriptor buffer.
    if (fDaclPresent == FALSE || pDacl == NULL)
    {
        wprintf(L"No DACL was found (all access is denied), or a NULL DACL (unrestricted access) was found.\n");
        return NO_ERROR;
    }

    // Retrieve the ACL_SIZE_INFORMATION structure to find the number of ACEs in the DACL.
    if (GetAclInformation( pDacl, &aclsizeinfo, sizeof(aclsizeinfo), AclSizeInformation) == FALSE)
    {
        dwErrorCode = GetLastError();
        wprintf(L"GetAclInformation failed. GetLastError returned: %d\n", dwErrorCode);
        return HRESULT_FROM_WIN32(dwErrorCode);
    }

    // Create buffers for the account name and the domain name.
    wszAccName = new WCHAR[dwAccBufferSize];
    if (wszAccName == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }
    wszDomainName = new WCHAR[dwDomainBufferSize];
    if (wszDomainName == NULL)
    {
        return ERROR_OUTOFMEMORY;
    }
    memset(wszAccName, 0, dwAccBufferSize*sizeof(WCHAR));
    memset(wszDomainName, 0, dwDomainBufferSize*sizeof(WCHAR));

    // Set the computer name string to NULL for the local computer.
//    if (wcscmp(wszComputerName, L".") == 0)
//    {
//        wszComputerName = L"\0";
//    }

    // Loop through the ACEs and display the information.
    for (cAce = 0; cAce < aclsizeinfo.AceCount && hr == NO_ERROR; cAce++)
    {

        // Get ACE info
        if (GetAce(pDacl,cAce, (LPVOID*)&pAce) == FALSE)
        {
            wprintf(L"GetAce failed. GetLastError returned: %d\n", GetLastError());
            continue;
        }

        // Obtain the account name and domain name for the SID in the ACE.
        for ( ; ; )
        {

            // Set the character-count variables to the buffer sizes.
            cchAccName = dwAccBufferSize;
            cchDomainName = dwDomainBufferSize;
            if (LookupAccountSidW( wszComputerName, &pAce->SidStart,   wszAccName, &cchAccName, wszDomainName, &cchDomainName, &eSidType) == 0)
            {
                break;
            }

            // Check if one of the buffers was too small.
            if ((cchAccName > dwAccBufferSize) || (cchDomainName > dwDomainBufferSize))
            {

                // Reallocate memory for the buffers and try again.
                wprintf(L"The name buffers were too small. They will be reallocated.\n");
                delete [] wszAccName;
                delete [] wszDomainName;
                wszAccName = new WCHAR[cchAccName];
                if (wszAccName == NULL)
                {
                    return ERROR_OUTOFMEMORY;
                }
                wszDomainName = new WCHAR[cchDomainName];
                if (wszDomainName == NULL)
                {
                    return ERROR_OUTOFMEMORY;
                }
                memset(wszAccName, 0, cchAccName*sizeof(WCHAR));
                memset(wszDomainName, 0, cchDomainName*sizeof(WCHAR));
                dwAccBufferSize = cchAccName;
                dwDomainBufferSize = cchDomainName;
                continue;
            }

            // Something went wrong in the call to LookupAccountSid.
            // Check if an unexpected error occurred.
            if (GetLastError() == ERROR_NONE_MAPPED)
            {
                wprintf(L"An unexpected error occurred during the call to LookupAccountSid. A name could not be found for the SID.\n" );
                wszDomainName[0] = L'\0';
                if (dwAccBufferSize > wcslen(L"!Unknown!"))
                {
                    // ************************************
                    // You must copy the string "!Unknown!" into the
                    // wszAccName buffer.
                    // ************************************

                    wszAccName[dwAccBufferSize - 1] = L'\0';
                }
                break;
            }
            else
            {
                dwErrorCode = GetLastError();
                wprintf(L"LookupAccountSid failed. GetLastError returned: %d\n", dwErrorCode);
                delete [] wszAccName;
                delete [] wszDomainName;
                return HRESULT_FROM_WIN32(dwErrorCode);
            }
        }

        switch(pAce->Header.AceType)
        {
        case ACCESS_ALLOWED_ACE_TYPE:
            if (wszDomainName[0] == 0)
            {
                wprintf(L"\nPermissions granted to %s\n", wszAccName);
            }
            else wprintf(L"\nPermissions granted to %s\\%s\n", wszDomainName, wszAccName);
//            DisplayPermissions(pAce->Mask);
            break;

        case ACCESS_DENIED_ACE_TYPE:
            if (wszDomainName[0] == 0)
            {
                wprintf(L"\nPermissions denied to %s\n", wszAccName);
            }
            else wprintf(L"\nPermissions denied to %s\\%s\n", wszDomainName, wszAccName);
//            DisplayPermissions(pAce->Mask);
            break;

        default:
            wprintf(L"Unknown ACE Type");
        }
    }

    // Free memory allocated for buffers.
    delete [] wszAccName;
    delete [] wszDomainName;

    return NO_ERROR;
}

NTSTATUS SecurityProcessor::generateDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, std::wstring &strindsd)
{
    if(!ConvertStringSecurityDescriptorToSecurityDescriptorW(strindsd.c_str(),SDDL_REVISION_1,&pSecurityDescriptor,NULL)){
        return GetLastError();
    }
    return NO_ERROR;
}
