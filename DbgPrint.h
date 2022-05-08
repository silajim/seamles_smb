#ifndef DBGPRINT_H
#define DBGPRINT_H

#include <windows.h>
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>

extern BOOL g_UseStdErr;
extern BOOL g_DebugMode;

void DbgPrint(LPCWSTR format, ...);
#endif // DBGPRINT_H
