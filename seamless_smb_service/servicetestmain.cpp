#include "seamless_smb_service.h"
#include "daemon.h"
int main(int argc, char *argv[])
{

    QCoreApplication src(argc,argv);
    qApp->setOrganizationDomain("Seamless smb");
    qApp->setApplicationName("seamless_smb_service");
    Daemon d;
    src.exec();
    return 0;
}
