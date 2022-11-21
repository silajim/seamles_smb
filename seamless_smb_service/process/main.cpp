//#include "seamless_smb_service.h"
//#include "daemon.h"

#include <QCoreApplication>
#include "dokanrunner.h"
int main(int argc, char *argv[])
{
    if(argc<2)
        return -1;

    QCoreApplication app(argc,argv);

    DokanRunner runner(argc,argv);

    QObject::connect(&app,&QCoreApplication::aboutToQuit,[&](){
        runner.unmount();
        QCoreApplication::exit(0);
    });
    return app.exec();
}
