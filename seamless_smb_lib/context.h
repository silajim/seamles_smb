#ifndef CONTEXT_H
#define CONTEXT_H

#include <memory>
#include "DbgPrint.h"
#include "WinSec.h"
#include "globals.h"
#include "nodes.h"
#include "cache.h"

struct Context{
    std::shared_ptr<DbgPrint> print;
    std::shared_ptr<Globals> globals;
    std::shared_ptr<Nodes> nodes;
    std::shared_ptr<WinSec> winsec;
    std::shared_ptr<Cache> cache;
};

#endif // CONTEXT_H
