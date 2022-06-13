#ifndef DBGPRINT_H
#define DBGPRINT_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>

#include "seamless_smb_lib_export.h"
//extern BOOL g_UseStdErr;
//extern BOOL g_DebugMode;

//void DbgPrint(LPCWSTR format, ...);

class SEAMLESS_SMB_LIB_EXPORT DbgPrint{
public:
 DbgPrint(bool UseStdErr , bool DebugMode);
 virtual ~DbgPrint() = default;

 virtual void print(LPCWSTR format, ...);

 bool DebugMode() const;

protected:
 bool m_UseStdErr;
 bool m_DebugMode;
};
#endif // DBGPRINT_H
