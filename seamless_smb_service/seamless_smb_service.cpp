#include "seamless_smb_service.h"
#include <QFile>
#include <QTextStream>


seamless_smb_service::seamless_smb_service(int argc, char **argv): QtService<QCoreApplication>(argc,argv,"Seamless smb")
{
    qApp->setOrganizationDomain("Seamless smb");
    qApp->setApplicationName("seamless_smb_service");
    setServiceDescription("A service to run multiple instances of seamless smb lib");
    setServiceFlags(QtServiceBase::NeedsStopOnShutdown);
}

void seamless_smb_service::start()
{
     qDebug() << "Start Service";
    if(!daemon && !dthread){
        daemon = new Daemon();
        dthread = new QThread(this);
//        connect(dthread,&QThread::finished,dthread,&QThread::deleteLater);
        connect(dthread,&QThread::started,daemon,&Daemon::start);
        connect(daemon,&Daemon::destroyed,dthread,&QThread::quit,Qt::DirectConnection);
        daemon->moveToThread(dthread);
        dthread->start();
    }
}

void seamless_smb_service::stop()
{
    qDebug() << "Stop Service";
    if(daemon && dthread){
        QCoreApplication::processEvents();
        daemon->deleteLater();
        QCoreApplication::processEvents();
//        dthread->exit();
        dthread->wait(3*1000);
        delete dthread;
        daemon = nullptr;
        dthread = nullptr;
         qDebug() << "Stop Service END";
    }
}

void seamless_smb_service::processCommand(int code)
{
    qDebug() << "Command" << code;
}
