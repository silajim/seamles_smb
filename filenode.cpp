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

#include "filenode.h"

//#include <spdlog/spdlog.h>
#include "DbgPrint.h"
#include "WinSec.h"

filenode::filenode(const std::wstring& filename, bool is_directory, DWORD file_attr, const PDOKAN_IO_SECURITY_CONTEXT security_context)
    : is_directory(is_directory), attributes(file_attr), _fileName(filename) {
    std::wcout << L"FILENODE name " << filename << std::endl;
    std::wcout.flush();
    // No lock need, FileNode is still not in a directory
    //  times.reset();

//    m_print = print;
//    m_winsec = winsec;

//    if (security_context && security_context->AccessState.SecurityDescriptor) {
//        security.SetDescriptor(m_winsec,m_print,security_context->AccessState.SecurityDescriptor);
//    }
}

//void filenode::SetDescriptor(PSECURITY_DESCRIPTOR securitydescriptor)
//{
//    security.SetDescriptor(m_winsec,m_print,securitydescriptor);
//}

//void filenode::GetDescriptor(PSECURITY_DESCRIPTOR *securitydescriptor)
//{
//    security.GetDescriptor(m_winsec,m_print,securitydescriptor);
//}

void security_informations::SetDescriptor(std::shared_ptr<WinSec> winsec , std::shared_ptr<DbgPrint> print ,PSECURITY_DESCRIPTOR securitydescriptor) {    
    if (!securitydescriptor) return;

    PSECURITY_DESCRIPTOR internalsd = securitydescriptor;
    DWORD internalsize =  GetSecurityDescriptorLength(securitydescriptor);

    if(!winsec->isSelfRelative(securitydescriptor)){
        print->print(L"SetDescriptor Absolute converted to Self-Relative\n");
        MakeSelfRelativeSD(securitydescriptor,internalsd,&internalsize);
    }

    descriptor_size = internalsize;
    descriptor = std::make_unique<byte[]>(internalsize);
    memcpy(descriptor.get(), internalsd, static_cast<size_t>(internalsize));

    LPWSTR ssd;
    unsigned long size;
    ConvertSecurityDescriptorToStringSecurityDescriptorW(internalsd,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |  PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION |  UNPROTECTED_SACL_SECURITY_INFORMATION  , &ssd , &size);
//    if(!ConvertSecurityDescriptorToStringSecurityDescriptorW(internalsd,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |  PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION |  UNPROTECTED_SACL_SECURITY_INFORMATION  , &ssd , &size)){
//        return GetLastError();
//    }
    strdescriptor = std::wstring(ssd,size);

    if(internalsd!=securitydescriptor){
        LocalFree(internalsd);
    }
    //    SecurityProcessor sp;
    //    sp.getAllData(securitydescriptor,strdescriptor);
}

void security_informations::GetDescriptor(std::shared_ptr<WinSec> winsec, std::shared_ptr<DbgPrint> print,PSECURITY_DESCRIPTOR *securitydescriptor)
{
    if(descriptor_size!=0){
        *securitydescriptor = LocalAlloc(LPTR, static_cast<size_t>(descriptor_size));
        memcpy(*securitydescriptor,descriptor.get(),static_cast<size_t>(descriptor_size));

        LPWSTR ssd;
        unsigned long size;
        if(ConvertSecurityDescriptorToStringSecurityDescriptorW(*securitydescriptor,SDDL_REVISION_1,OWNER_SECURITY_INFORMATION | ATTRIBUTE_SECURITY_INFORMATION | DACL_SECURITY_INFORMATION | GROUP_SECURITY_INFORMATION | LABEL_SECURITY_INFORMATION | SACL_SECURITY_INFORMATION | PROTECTED_DACL_SECURITY_INFORMATION |  PROTECTED_SACL_SECURITY_INFORMATION | UNPROTECTED_DACL_SECURITY_INFORMATION |  UNPROTECTED_SACL_SECURITY_INFORMATION  , &ssd , &size)==0){
            DbgPrint(L"ConvertSecurityDescriptorToStringSecurityDescriptorW Error %d\n",GetLastError());
            return;
        }
        std::wstring strds = std::wstring(ssd,size);

        if(strds == strdescriptor){
            print->print(L"Strings Equal\n");
        }else{
            print->print(L"Strings NOT Equal\n");
        }


    }else{
        winsec->CreateDefaultSelfRelativeSD(securitydescriptor);
    }
}

//DWORD filenode::read(LPVOID buffer, DWORD bufferlength, LONGLONG offset) {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    if (static_cast<size_t>(offset + bufferlength) > _data.size())
//        bufferlength = (_data.size() > static_cast<size_t>(offset))
//                ? static_cast<DWORD>(_data.size() - offset)
//                : 0;
//    if (bufferlength)
//        memcpy(buffer, &_data[static_cast<size_t>(offset)], bufferlength);
////    spdlog::info(L"Read {} : BufferLength {} Offset {}", get_filename(), bufferlength, offset);
//    DbgPrint(L"Read %s : BufferLength %s Offset %s", get_filename().c_str(),bufferlength,offset);
//    return bufferlength;
//}

//DWORD filenode::write(LPCVOID buffer, DWORD number_of_bytes_to_write, LONGLONG offset) {
//    if (!number_of_bytes_to_write) return 0;

//    std::lock_guard<std::mutex> lock(_data_mutex);
//    if (static_cast<size_t>(offset + number_of_bytes_to_write) > _data.size())
//        _data.resize(static_cast<size_t>(offset + number_of_bytes_to_write));

//    spdlog::info(L"Write {} : NumberOfBytesToWrite {} Offset {}", get_filename(), number_of_bytes_to_write, offset);
//    memcpy(&_data[static_cast<size_t>(offset)], buffer, number_of_bytes_to_write);
//    return number_of_bytes_to_write;
//}

//const LONGLONG filenode::get_filesize() {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    return static_cast<LONGLONG>(_data.size());
//}

//void filenode::set_endoffile(const LONGLONG& byte_offset) {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    _data.resize(static_cast<size_t>(byte_offset));
//}

//const std::wstring filenode::get_filename() {
//    std::lock_guard<std::mutex> lock(_fileName_mutex);
//    return _fileName;
//}

//void filenode::set_filename(const std::wstring& f) {
//    std::lock_guard<std::mutex> lock(_fileName_mutex);
//    _fileName = f;
//}

//void filenode::add_stream(const std::shared_ptr<filenode>& stream) {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    _streams[stream->get_filename()] = stream;
//}

//void filenode::remove_stream(const std::shared_ptr<filenode>& stream) {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    _streams.erase(stream->get_filename());
//}

//std::unordered_map<std::wstring, std::shared_ptr<filenode> >
//filenode::get_streams() {
//    std::lock_guard<std::mutex> lock(_data_mutex);
//    return _streams;
//}


