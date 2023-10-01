#ifndef CACHE_H
#define CACHE_H

#include <string>
#include <Windows.h>

#include <thread>
#include <iostream>
#include <map>

#include "DbgPrint.h"
#include "globals.h"


class Cache{
public:

  Cache(std::wstring rootDir, std::shared_ptr<DbgPrint>);
  ~Cache();

  bool GetVolumeInformationNotAvailable = false;

  std::wstring bzvolfile = L"\\.bzvol\\bzvol_id.xml";
  BY_HANDLE_FILE_INFORMATION bzvolInfo = {};
  bool bzvolInfoSet = false;
  char* bzvolData = nullptr;
  DWORD bzvolDataLength = 0;
  LONGLONG bzvolDataOffset = 0;

  std::map<std::wstring,WIN32_FIND_DATAW> files;

private:
  std::unique_ptr<std::thread> t;
  const std::wstring m_rootDir;
  void MonitorDirectoryChanges();
  void recursiveFileFinder(WCHAR *root);

  std::shared_ptr<DbgPrint> m_print;
  HANDLE exitEvent=0;

};



#endif // CACHE_H
