#include "cache.h"

const wchar_t* fileNotifyChangeFlagToString(const DWORD flag) {
  switch (flag) {
  case FILE_NOTIFY_CHANGE_FILE_NAME:
    return L"FILE_NOTIFY_CHANGE_FILE_NAME";
  case FILE_NOTIFY_CHANGE_DIR_NAME:
    return L"FILE_NOTIFY_CHANGE_DIR_NAME";
  case FILE_NOTIFY_CHANGE_SIZE:
    return L"FILE_NOTIFY_CHANGE_SIZE";
  case FILE_NOTIFY_CHANGE_LAST_WRITE:
    return L"FILE_NOTIFY_CHANGE_LAST_WRITE";
  case FILE_NOTIFY_CHANGE_CREATION:
    return L"FILE_NOTIFY_CHANGE_CREATION";
  case FILE_NOTIFY_CHANGE_LAST_ACCESS:
    return L"FILE_NOTIFY_CHANGE_LAST_ACCESS";
  case FILE_NOTIFY_CHANGE_ATTRIBUTES:
    return L"FILE_NOTIFY_CHANGE_ATTRIBUTES";
  default:
    return L"Unknown";
  }
}

Cache::Cache(std::wstring rootDir , std::shared_ptr<DbgPrint> print): m_rootDir(rootDir) , m_print(print)
{
  t = std::make_unique<std::thread>(&Cache::MonitorDirectoryChanges , this);
  m_print->print(L"Cache started %s\n" , bzvolfile.c_str());
}

Cache::~Cache()
{
   SetEvent(exitEvent);
  t->join();

  m_print->print(L"Cache Destructed\n");
}

void Cache::MonitorDirectoryChanges() {
  using namespace std;
   m_print->print(L"Thread started");

  // Create a handle to the directory to monitor
   exitEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
  HANDLE hDirectory = CreateFileW(
      m_rootDir.c_str(),
      FILE_LIST_DIRECTORY,
      FILE_SHARE_READ | FILE_SHARE_WRITE | FILE_SHARE_DELETE,
      NULL,
      OPEN_EXISTING,
      FILE_FLAG_BACKUP_SEMANTICS | FILE_FLAG_OVERLAPPED,
      NULL
      );

  if (hDirectory == INVALID_HANDLE_VALUE) {
    std::cout << "Invalid handle"  << GetLastError() << endl;
    return;
  }

  // Set up an OVERLAPPED structure for asynchronous I/O
  OVERLAPPED overlapped;
  ZeroMemory(&overlapped, sizeof(OVERLAPPED));

  // Buffer to hold the change notifications
  BYTE buffer[4096]; // Adjust the buffer size as needed

  // Monitor the directory for changes
  while (true) {
    DWORD bytesRead;
    if (!ReadDirectoryChangesW(
            hDirectory,
            buffer,
            sizeof(buffer),
            TRUE,
            FILE_NOTIFY_CHANGE_FILE_NAME | FILE_NOTIFY_CHANGE_DIR_NAME | FILE_NOTIFY_CHANGE_SIZE | FILE_NOTIFY_CHANGE_LAST_WRITE | FILE_NOTIFY_CHANGE_CREATION | FILE_NOTIFY_CHANGE_LAST_ACCESS | FILE_NOTIFY_CHANGE_ATTRIBUTES,
            &bytesRead,
            &overlapped,
            NULL)) {
      // Handle error here
      break;
    }

    const HANDLE handles[] = {hDirectory, exitEvent };
    // Wait for the directory change event
    DWORD waitResult = WaitForMultipleObjects(2,handles, FALSE ,INFINITE);

    if (waitResult == WAIT_OBJECT_0) {
      // Handle directory change
      // The buffer contains information about the changes
      // Parse and process the change notifications here
      FILE_NOTIFY_INFORMATION *event = (FILE_NOTIFY_INFORMATION*)buffer;

      for (;;) {
        DWORD name_len = event->FileNameLength / sizeof(wchar_t);

        m_print->print(L"Change %.*s action %s\n",name_len, event->FileName, fileNotifyChangeFlagToString(event->Action));

        switch (event->Action) {
        case FILE_ACTION_ADDED: {
          //          wprintf(L"       Added: %.*s\n", name_len, event->FileName);
        } break;

        case FILE_ACTION_REMOVED: {
          //          wprintf(L"     Removed: %.*s\n", name_len, event->FileName);
        } break;

        case FILE_ACTION_MODIFIED: {
          //          wprintf(L"    Modified: %.*s\n", name_len, event->FileName);
          if(std::wstring(&event->FileName[0],(size_t)name_len) == bzvolfile){
            bzvolInfoSet = false;
            memset(&bzvolInfo,0,sizeof(bzvolInfo));
            free(bzvolData);
            bzvolData = nullptr;
            bzvolDataLength = 0;
            bzvolDataOffset = 0;
          }
        } break;

        case FILE_ACTION_RENAMED_OLD_NAME: {
          //          wprintf(L"Renamed from: %.*s\n", name_len, event->FileName);
        } break;

        case FILE_ACTION_RENAMED_NEW_NAME: {
          //          wprintf(L"          to: %.*s\n", name_len, event->FileName);
        } break;

        default: {
          printf("Unknown action!\n");
        }
        break;
        }

        // Are there more events to handle?
        if (event->NextEntryOffset) {
          *((uint8_t**)&event) += event->NextEntryOffset;
        } else {
          break;
        }
      }

      // Reissue the asynchronous read
      ResetEvent(hDirectory);
    } else {
      // Handle error here
      break;
    }
  }

  // Close the directory handle when done
  CloseHandle(hDirectory);

  m_print->print(L"Thread ended\n");
}

void Cache::recursiveFileFinder(WCHAR *root)
{
  HANDLE hFind;
  WIN32_FIND_DATAW findData;
  DWORD error;

  hFind = FindFirstFile(root, &findData);

  if (hFind == INVALID_HANDLE_VALUE) {
    error = GetLastError();
    m_print->print(L"\tinvalid file handle. Error is %u\n", error);
    return; //error;
  }

  // Root folder does not have . and .. folder - we remove them
  BOOLEAN rootFolder = (wcscmp(root, L"\\") == 0);
  do {
    if (!rootFolder || (wcscmp(findData.cFileName, L".") != 0 && wcscmp(findData.cFileName, L"..") != 0)){
      //      FillFindData(&findData, DokanFileInfo);
      //    ++count;
    }
  } while (FindNextFile(hFind, &findData) != 0);

  error = GetLastError();
  FindClose(hFind);

  if (error != ERROR_NO_MORE_FILES) {
    m_print->print(L"\tFindNextFile error. Error is %u\n", error);
    return; //DokanNtStatusFromWin32(error);
  }

}
