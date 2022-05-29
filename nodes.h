#ifndef NODES_H
#define NODES_H

#include "include/dokan/dokan.h"
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
    Nodes(): _filenodes(std::make_shared<std::list<std::shared_ptr<filenode>>>()) {
        //       _filenodes = std::shared_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>();
    }
    std::mutex m_mutex;

    void printAll();
    std::shared_ptr<filenode> findFileNode(std::wstring fname);
    bool addFileNode(std::shared_ptr<filenode> node);

private:
    friend class boost::serialization::access;
    template<class Archive>
    void serialize(Archive & ar, const unsigned int version)
    {
        ar &_filenodes;
    }

    std::shared_ptr<std::list<std::shared_ptr<filenode>>> _filenodes;
};

inline void Nodes::printAll()
{
    for(auto it  = _filenodes->begin();it!=_filenodes->end();++it){
        std::wcout  << it->get()->_fileName << std::endl;
        std::wcout.flush();
    }

}

inline std::shared_ptr<filenode> Nodes::findFileNode(std::wstring fname)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if(_filenodes->empty())
        return nullptr;

    for(auto it  = _filenodes->begin();it!=_filenodes->end();++it){
        if(it->get()->_fileName == fname)
            return *it;
    }

    return nullptr;
}

inline bool Nodes::addFileNode(std::shared_ptr<filenode> node)
{
    std::lock_guard<std::mutex> lg(m_mutex);
    if(!node)
        return false;
    m_mutex.unlock();
    auto f = findFileNode(node->_fileName);
    m_mutex.lock();
    if(!f){
        _filenodes->push_back(node);
        return true;
    }
    return false;
}


#endif // NODES_H
