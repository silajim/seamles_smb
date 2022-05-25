#include "DbgPrint.h"



DbgPrint::DbgPrint(bool UseStdErr, bool DebugMode):m_UseStdErr(UseStdErr),m_DebugMode(DebugMode)
{

}

void DbgPrint::print(LPCWSTR format, ...) {
    if (m_DebugMode) {
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
        if (m_UseStdErr)
            fputws(outputString, stderr);
        else
            OutputDebugStringW(outputString);
        if (buffer)
            _freea(buffer);
        va_end(argp);
        if (m_UseStdErr)
            fflush(stderr);
    }
}

bool DbgPrint::DebugMode() const
{
    return m_DebugMode;
}
