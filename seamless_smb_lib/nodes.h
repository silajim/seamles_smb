#ifndef NODES_H
#define NODES_H

#include "dokan/dokan.h"
#include <malloc.h>
#include <stdio.h>
#include <stdlib.h>
#include <winbase.h>
#include <string>
#include <iostream>
#include <sddl.h>
#include <accctrl.h>
#include <aclapi.h>
#include <map>
#include <unordered_map>
#include "filenode.h"
#include <list>

#include <handleapi.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#include <Mq.h>

#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/list.hpp>
#include "DbgPrint.h"

#include <ostream>
#include <algorithm>

struct Nodes{
    Nodes() {
        //       _filenodes = std::shared_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>();
    }
    std::mutex m_mutex;

    void printAll();
    std::shared_ptr<filenode> findFileNode(std::wstring fname);
    bool addFileNode(std::shared_ptr<filenode> node);
    bool rename(std::wstring fname , std::wstring newfname);
    bool move(std::wstring fname , std::wstring newfname);
    bool deleteFileNode(std::wstring fname);
    bool deleteFileNode( std::shared_ptr<filenode> node);

    std::mutex writeCountMutex;
    std::map<HANDLE, std::atomic<uint8_t>> writeCounts;

private:
     std::shared_ptr<filenode> findFileNodeP(std::wstring fname);
//    std::mutex m2;
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar &_filenodes;
    }

    std::list<std::shared_ptr<filenode>> _filenodes;
};

inline void Nodes::printAll()
{
    for(auto it  = _filenodes.begin();it!=_filenodes.end();++it){
        std::wcout  << it->get()->_fileName << std::endl;
        std::wcout.flush();
    }

}

inline std::shared_ptr<filenode> Nodes::findFileNode(std::wstring fname)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    return findFileNodeP(fname);

}

inline bool Nodes::addFileNode(std::shared_ptr<filenode> node)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if(!node)
        return false;
    auto f = findFileNodeP(node->_fileName);
    if(!f){
        _filenodes.push_back(node);
        return true;
    }
    return false;
}

inline bool Nodes::rename(std::wstring fname, std::wstring newfname)
{
     std::lock_guard<std::mutex> lg(m_mutex);
    if(_filenodes.empty())
        return false;
    auto f = findFileNodeP(fname);
    if(f){
        f->_fileName = newfname;
        return true;
    }
    return false;
}

inline bool Nodes::move(std::wstring fname, std::wstring newfname)
{
    return rename(fname,newfname);
}

inline bool Nodes::deleteFileNode(std::wstring fname)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if(_filenodes.empty())
        return false;
    auto f = findFileNodeP(fname);
    if(f){
        _filenodes.remove(f);
        return true;
    }
    return false;
}

inline bool Nodes::deleteFileNode(std::shared_ptr<filenode> node)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if(_filenodes.empty())
        return false;
     _filenodes.remove(node);
     return true;
}

inline std::shared_ptr<filenode> Nodes::findFileNodeP(std::wstring fname)
{
    if(_filenodes.empty())
        return nullptr;

    for(auto it  = _filenodes.begin();it!=_filenodes.end();++it){
        if(it->get()->_fileName == fname)
            return *it;
    }

    return nullptr;
}


#endif // NODES_H
