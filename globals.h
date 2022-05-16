#ifndef GLOBALS_H
#define GLOBALS_H

#include <windows.h>

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

extern  WCHAR RootDirectory[DOKAN_MAX_PATH];
extern  WCHAR MountPoint[DOKAN_MAX_PATH];
extern  WCHAR UNCName[DOKAN_MAX_PATH];

#endif // GLOBALS_H
