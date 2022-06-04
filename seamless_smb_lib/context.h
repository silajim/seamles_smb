#ifndef CONTEXT_H
#define CONTEXT_H

#include <memory>
#include "DbgPrint.h"
#include "WinSec.h"
#include "globals.h"
#include "nodes.h"

struct Context{
    std::shared_ptr<DbgPrint> print;
    std::shared_ptr<Globals> globals;
    std::shared_ptr<Nodes> nodes;
    std::shared_ptr<WinSec> winsec;
};

#endif // CONTEXT_H
