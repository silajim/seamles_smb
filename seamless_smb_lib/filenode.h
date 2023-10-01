/*
  Dokan : user-mode file system library for Windows

  Copyright (C) 2019 Adrien J. <liryna.stark@gmail.com>
  Copyright (C) 2020 Google, Inc.

  http://dokan-dev.github.io

Permission is hereby granted, free of charge, to any person obtaining a copy
of this software and associated documentation files (the "Software"), to deal
in the Software without restriction, including without limitation the rights
to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
copies of the Software, and to permit persons to whom the Software is
furnished to do so, subject to the following conditions:

The above copyright notice and this permission notice shall be included in
all copies or substantial portions of the Software.

THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN
THE SOFTWARE.
*/

#ifndef FILENODE_H_
#define FILENODE_H_

#include <dokan/dokan.h>
//#include <dokan/fileinfo.h>

//#include "memfs_helper.h"

#include <WinBase.h>
#include <atomic>
#include <filesystem>
#include <mutex>
#include <sstream>
#include <string>
#include <unordered_map>

#include <boost/serialization/unordered_map.hpp>
#include <boost/serialization/serialization.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>

#include "WinSec.h"

#include "securityprocessor.h"

struct security_informations {
    std::wstring strdescriptor;
    std::unique_ptr<byte[]> descriptor = nullptr;
    mutable std::mutex m_mutex;

    //  std::unique_ptr<PACL> dacl = nullptr;

    unsigned long descriptor_size = 0;

    security_informations() = default;

    void SetDescriptor(std::shared_ptr<WinSec> winsec, std::shared_ptr<DbgPrint> print, PSECURITY_DESCRIPTOR securitydescriptor);
    void GetDescriptor(std::shared_ptr<WinSec> winsec, std::shared_ptr<DbgPrint> print,PSECURITY_DESCRIPTOR *securitydescriptor);

private:
    friend class boost::serialization::access;
    template <class Archive>
    void save(Archive& ar, unsigned int version) const {
        std::lock_guard<std::mutex> lk(m_mutex);
        ar &strdescriptor;
        ar &descriptor_size;
        for(unsigned int i=0;i<descriptor_size;i++){
            ar &descriptor[i];
        }
    }

    template <class Archive>
    void load(Archive& ar, unsigned int version) {
        std::lock_guard<std::mutex> lk(m_mutex);
        ar &strdescriptor;
        ar &descriptor_size;
        descriptor = std::make_unique<byte[]>(descriptor_size);

        for(unsigned int i=0;i<descriptor_size;i++){
            ar &descriptor[i];
        }
    }
    BOOST_SERIALIZATION_SPLIT_MEMBER()
};

//struct filetimes {
//  void reset() {
//    lastaccess = lastwrite = creation = get_currenttime();
//  }

//  static bool empty(const FILETIME* filetime) {
//    return filetime->dwHighDateTime == 0 && filetime->dwLowDateTime == 0;
//  }

//  static LONGLONG get_currenttime() {
//    FILETIME t;
//    GetSystemTimeAsFileTime(&t);
//    return memfs_helper::DDwLowHighToLlong(t.dwLowDateTime, t.dwHighDateTime);
//  }

//  std::atomic<LONGLONG> creation;
//  std::atomic<LONGLONG> lastaccess;
//  std::atomic<LONGLONG> lastwrite;


// Memfs file context
// Each file/directory on the memfs has his own filenode instance
// Alternated streams are also filenode where the main stream \myfile::$DATA
// has all the alternated streams (e.g. \myfile:foo:$DATA) attached to him
// and the alternated has main_stream assigned to the main stream filenode.
class filenode {
public:
    filenode(const std::wstring &filename, bool is_directory, DWORD file_attr, const PDOKAN_IO_SECURITY_CONTEXT security_context);

    //  filenode(const filenode& f);

    //  DWORD read(LPVOID buffer, DWORD bufferlength, LONGLONG offset);
    //  DWORD write(LPCVOID buffer, DWORD number_of_bytes_to_write, LONGLONG offset);

    //  const LONGLONG get_filesize();
    //  void set_endoffile(const LONGLONG& byte_offset);

    //  // Filename can during a move so we need to protect it behind a lock
    //  const std::wstring get_filename();
    //  void set_filename(const std::wstring& filename);

    //  // Alternated streams
    //  void add_stream(const std::shared_ptr<filenode>& stream);
    //  void remove_stream(const std::shared_ptr<filenode>& stream);
    //  std::unordered_map<std::wstring, std::shared_ptr<filenode> > get_streams();

    // No lock needed above
    //  std::atomic<bool> is_directory = false;
    //  std::atomic<DWORD> attributes = 0;
    bool is_directory = false;
    DWORD attributes = 0;
    std::wstring _fileName;
    //  LONGLONG fileindex = 0;
    //  std::shared_ptr<filenode> main_stream;

    //  filetimes times;
    security_informations security;
    std::mutex _data_mutex;

    //  void SetDescriptor(PSECURITY_DESCRIPTOR securitydescriptor);
    //  void GetDescriptor(PSECURITY_DESCRIPTOR *securitydescriptor);


private:

    filenode() = default;

    //  std::shared_ptr<WinSec> m_winsec;
    //  std::shared_ptr<DbgPrint> m_print;

    friend class boost::serialization::access;
    template <class Archive>
    void serialize(Archive& ar, const unsigned int version)
    {
        ar &is_directory;
        ar &attributes;
        //       ar &_data_mutex;
        ar &_fileName;
        ar &security;
    }

    // _data_mutex need to be aquired
    //  std::vector<uint8_t> _data;
    //  std::unordered_map<std::wstring, std::shared_ptr<filenode> > _streams;

    //  std::mutex _fileName_mutex;
    // _fileName_mutex need to be aquired

};

#endif  // FILENODE_H_
