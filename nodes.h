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

#include <handleapi.h>
#include <processthreadsapi.h>
#include <Psapi.h>
#include <Mq.h>

#include <boost/filesystem.hpp>
#include <boost/serialization/map.hpp>
#include <boost/archive/binary_iarchive.hpp>
#include <boost/archive/binary_oarchive.hpp>
#include <boost/serialization/shared_ptr.hpp>
#include <boost/serialization/unordered_map.hpp>
#include "DbgPrint.h"

#include <ostream>
#include <algorithm>

struct Nodes{
    Nodes(): _filenodes(std::make_shared<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>()) {
//       _filenodes = std::shared_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>>();
    }
    std::shared_ptr<std::unordered_map<std::wstring, std::shared_ptr<filenode>>> _filenodes;
    std::mutex m_mutex;

    template<class Archive>
        void serialize(Archive & ar, const unsigned int version)
        {
            ar &_filenodes;
        }

        std::shared_ptr<filenode> findFileNode(std::wstring fname);

};

inline std::shared_ptr<filenode> Nodes::findFileNode(std::wstring fname)
{
    if(_filenodes->empty())
        return nullptr;

    for(auto it  = _filenodes->begin();it!=_filenodes->end();++it){
        if(it->second->_fileName == fname)
            return it->second;
    }

    return nullptr;
}
#endif // NODES_H
