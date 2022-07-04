#include "seamless_smb_service.h"
#include <QFile>
#include <QTextStream>

void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString & msg)
{
    QString txt;
    switch (type) {
    case QtDebugMsg:
        txt = QString("Debug: %1").arg(msg);
        break;
    case QtWarningMsg:
        txt = QString("Warning: %1").arg(msg);
        break;
    case QtCriticalMsg:
        txt = QString("Critical: %1").arg(msg);
        break;
    case QtFatalMsg:
        txt = QString("Fatal: %1").arg(msg);
        abort();
    }
    QFile outFile("Servicelog.txt");
    outFile.open(QIODevice::WriteOnly | QIODevice::Append);
    QTextStream ts(&outFile);
    ts << txt << Qt::endl;
}

seamless_smb_service::seamless_smb_service(int argc, char **argv): QtService<QCoreApplication>(argc,argv,"Seamless smb")
{
    qApp->setOrganizationDomain("Seamless smb");
    qApp->setApplicationName("seamless_smb_service");
    setServiceDescription("A service to run multiple instances of seamless smb lib");
    setServiceFlags(QtServiceBase::NeedsStopOnShutdown);
}

void seamless_smb_service::start()
{
    if(!daemon && !dthread){
//        qInstallMessageHandler(myMessageHandler);
        daemon = new Daemon(this);
        dthread = new QThread(this);
        connect(dthread,&QThread::finished,dthread,&QThread::deleteLater);
        daemon->moveToThread(dthread);
    }
}

void seamless_smb_service::stop()
{
    if(daemon && dthread){
        connect(daemon,&QObject::destroyed,dthread,&QThread::quit);
        daemon->deleteLater();
        daemon = nullptr;
        dthread = nullptr;
    }
}

void seamless_smb_service::processCommand(int code)
{

}
