#ifndef DBGPRINT_H
#define DBGPRINT_H
#include <stdarg.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>

extern BOOL g_UseStdErr;
extern BOOL g_DebugMode;

static void DbgPrint(LPCWSTR format, ...) {
    if (g_DebugMode) {
        const WCHAR *outputString;
        WCHAR *buffer = NULL;
        size_t length;
        va_list argp;

        va_start(argp, format);
        length = _vscwprintf(format, argp) + 1;
        buffer = (WCHAR*)_malloca(length * sizeof(WCHAR));
        if (buffer) {
            vswprintf_s(buffer, length, format, argp);
            outputString = buffer;
        } else {
            outputString = format;
        }
        if (g_UseStdErr)
            fputws(outputString, stderr);
        else
            OutputDebugStringW(outputString);
        if (buffer)
            _freea(buffer);
        va_end(argp);
        if (g_UseStdErr)
            fflush(stderr);
    }
}
#endif // DBGPRINT_H
