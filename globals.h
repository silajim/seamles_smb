#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>
#include <string>

// Enable Long Paths on Windows 10 version 1607 and later by changing
// the OS configuration (see Microsoft own documentation for the steps)
// and rebuild the mirror with the following line uncommented.
#define WIN10_ENABLE_LONG_PATH
#ifdef WIN10_ENABLE_LONG_PATH
//dirty but should be enough
#define DOKAN_MAX_PATH 32768
#else
#define DOKAN_MAX_PATH MAX_PATH
#endif // DEBUG

//extern  WCHAR RootDirectory[DOKAN_MAX_PATH];
//extern  WCHAR MountPoint[DOKAN_MAX_PATH];
//extern  WCHAR UNCName[DOKAN_MAX_PATH];



class Globals{
    public:
    explicit Globals();
    explicit Globals(std::wstring RootDirectory, std::wstring MountPoint , std::wstring UNCName);


    const std::wstring &RootDirectory() const;
    void setRootDirectory(const std::wstring &newRootDirectory);

    const std::wstring &MountPoint() const;
    void setMountPoint(const std::wstring &newMountPoint);

    const std::wstring &UNCName() const;
    void setUNCName(const std::wstring &newUNCName);


    bool CaseSensitive() const;
    void setCaseSensitive(bool newCaseSensitive);

    bool ImpersonateCallerUser() const;
    void setImpersonateCallerUser(bool newImpersonateCallerUser);

    const std::wstring &volname() const;
    void setVolname(const std::wstring &newVolname);

private:
    std::wstring m_RootDirectory;
    std::wstring m_MountPoint ;
    std::wstring m_UNCName;
    std::wstring m_volname;
    bool m_CaseSensitive = false;
    bool m_ImpersonateCallerUser = false;
    bool m_HasSeSecurityPrivilege = false;
};

#endif // GLOBALS_H
