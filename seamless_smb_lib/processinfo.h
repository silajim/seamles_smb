#ifndef PROCESSINFO_H
#define PROCESSINFO_H

#include <windows.h>
#include <string>
#include "DbgPrint.h"
#include <memory>

//class ProcessInfo
//{
//public:
//  ProcessInfo()=delete;
//  static
//      bool GetProcessNameFromPID(ULONG pid, std::wstring& processName);
//};

bool GetProcessNameFromPID(const ULONG pid, std::wstring& processName , const std::shared_ptr<DbgPrint> print);

#endif // PROCESSINFO_H
