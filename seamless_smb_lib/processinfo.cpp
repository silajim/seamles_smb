#include "processinfo.h"

//#include <Windows.h>

#include <Winternl.h>

bool GetProcessNameFromPID(const ULONG pid, std::wstring& processName, const std::shared_ptr<DbgPrint> print) {
  ULONG bufferSize = 0;
  NtQuerySystemInformation(SystemProcessInformation, nullptr, 0, &bufferSize);

  if (bufferSize > 0) {
    SYSTEM_PROCESS_INFORMATION* processInfo = (SYSTEM_PROCESS_INFORMATION*)malloc(bufferSize);
    if (processInfo) {
      NTSTATUS err = 0;
      err = NtQuerySystemInformation(SystemProcessInformation, processInfo, bufferSize, nullptr);
      if (err == 0) {
        SYSTEM_PROCESS_INFORMATION* current = processInfo;
        while (current->NextEntryOffset != 0) {
          // Convert the process ID to a DWORD
          ULONG currentPid = (ULONG)current->UniqueProcessId;

          // Check if this is the process you're interested in
          if (currentPid == pid) {
            processName.assign(current->ImageName.Buffer, current->ImageName.Length / sizeof(wchar_t));
            free(processInfo);
            return true;
          }

          current = (SYSTEM_PROCESS_INFORMATION*)((BYTE*)current + current->NextEntryOffset);
        }
      }else{
        print->print(L"NtQuerySystemInformation error: %d \n" , err);
      }
      free(processInfo);
    }
  }
  return false;
}
