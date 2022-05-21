#ifndef CONSOLEHANDLER_H
#define CONSOLEHANDLER_H

#include "globals.h"
#include <memory>

class ConsoleHandler
{
public:
    ConsoleHandler(std::shared_ptr<Globals> globals);

//    BOOL WINAPI CtrlHandler(DWORD dwCtrlType);

private:
    std::shared_ptr<Globals> m_globals;
};

#endif // CONSOLEHANDLER_H
