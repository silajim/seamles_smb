#include "globals.h"


//WCHAR RootDirectory[DOKAN_MAX_PATH] = L"C:";
//WCHAR MountPoint[DOKAN_MAX_PATH] = L"M:\\";
//WCHAR UNCName[DOKAN_MAX_PATH] = L"";

Globals::Globals()
{

}

Globals::Globals(std::wstring RootDirectory, std::wstring MountPoint, std::wstring UNCName)
{

    m_RootDirectory = RootDirectory;
    m_MountPoint = MountPoint;
    m_UNCName = UNCName;
}

const std::wstring &Globals::RootDirectory() const
{
    return m_RootDirectory;
}

void Globals::setRootDirectory(const std::wstring &newRootDirectory)
{
    m_RootDirectory = newRootDirectory;
}

const std::wstring &Globals::MountPoint() const
{
    return m_MountPoint;
}

void Globals::setMountPoint(const std::wstring &newMountPoint)
{
    m_MountPoint = newMountPoint;
}

const std::wstring &Globals::UNCName() const
{
    return m_UNCName;
}

void Globals::setUNCName(const std::wstring &newUNCName)
{
    m_UNCName = newUNCName;
}

bool Globals::CaseSensitive() const
{
    return m_CaseSensitive;
}

void Globals::setCaseSensitive(bool newCaseSensitive)
{
    m_CaseSensitive = newCaseSensitive;
}

bool Globals::ImpersonateCallerUser() const
{
    return m_ImpersonateCallerUser;
}

void Globals::setImpersonateCallerUser(bool newImpersonateCallerUser)
{
    m_ImpersonateCallerUser = newImpersonateCallerUser;
}

const std::wstring &Globals::volname() const
{
    return m_volname;
}

void Globals::setVolname(const std::wstring &newVolname)
{
    m_volname = newVolname;
}

bool Globals::HasSeSecurityPrivilege() const
{
    return m_HasSeSecurityPrivilege;
}

void Globals::setHasSeSecurityPrivilege(bool newHasSeSecurityPrivilege)
{
    m_HasSeSecurityPrivilege = newHasSeSecurityPrivilege;
}
