#ifndef DBGPRINT_H
#define DBGPRINT_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>

//extern BOOL g_UseStdErr;
//extern BOOL g_DebugMode;

//void DbgPrint(LPCWSTR format, ...);

class DbgPrint{
public:
 DbgPrint(bool UseStdErr , bool DebugMode);

 void print(LPCWSTR format, ...);

 bool DebugMode() const;

private:
 bool m_UseStdErr;
 bool m_DebugMode;
};
#endif // DBGPRINT_H
