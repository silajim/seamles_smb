#ifndef SECURITYPROCESSOR_H
#define SECURITYPROCESSOR_H

#include <Windows.h>
#include <string>

class SecurityProcessor
{
public:
    SecurityProcessor();
    HRESULT getAllData(PSECURITY_DESCRIPTOR pSecurityDescriptor, std::wstring &strindsd);
    NTSTATUS generateDescriptor(PSECURITY_DESCRIPTOR pSecurityDescriptor, std::wstring &strindsd);
};

#endif // SECURITYPROCESSOR_H
