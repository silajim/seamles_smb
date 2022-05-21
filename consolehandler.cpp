#include "consolehandler.h"
#include "include/dokan/dokan.h"

#include <functional>

ConsoleHandler::ConsoleHandler(std::shared_ptr<Globals> globals)
{
    m_globals = globals;
}

//BOOL WINAPI ConsoleHandler::CtrlHandler(DWORD dwCtrlType) {
//    switch (dwCtrlType) {
//    case CTRL_C_EVENT:
//    case CTRL_BREAK_EVENT:
//    case CTRL_CLOSE_EVENT:
//    case CTRL_LOGOFF_EVENT:
//    case CTRL_SHUTDOWN_EVENT:

//        std::function<WINAPI BOOL(DWORD)> func1 = std::bind(&ConsoleHandler::CtrlHandler,this,std::placeholders::_1);



//        SetConsoleCtrlHandler(func1, FALSE);
//        DokanRemoveMountPoint(m_globals->MountPoint().c_str());
//        return TRUE;
//    default:
//        return FALSE;
//    }
//}
