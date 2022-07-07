#include <QSettings>
#include <QList>
#include <QString>
#include <QCoreApplication>
#include <QStandardPaths>
#include <QDebug>

#include "dokan/dokan.h"
#include "common.h"

#include "QtServiceController"

#include <QThread>
#include <QUuid>

//#include "../seamless_smb_service/daemon.h"

#include "daemon.h"

typedef QList<MountInfo> mlist;

int main(int argc, char *argv[])
{
    QCoreApplication app(argc,argv);

    if(QtServiceController::install("G:\\Dokan Library-1.5.0\\sample\\build-mirror-Desktop_Qt_5_15_2_MSVC2019_64bit-Debug\\bin\\seamless_smb_service.exe"))
        qDebug("service installed");
    else{
        qDebug("Service not installed!");
        return -1;
    }

//    qRegisterMetaTypeStreamOperators<MountInfo>("MountInfo");
//    qRegisterMetaTypeStreamOperators<mlist>("mlist");


    qApp->setOrganizationDomain("Seamless smb");
    qApp->setApplicationName("seamless_smb_service");

    QStringList plist = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    QString path; //= QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    foreach(QString p, plist ){
        if(p.contains("ProgramData")){
            path = p;
            break;
        }
    }

    QSettings settings(path+"/mounts.ini", QSettings::IniFormat);

    qDebug() << path;

    mlist mounts;

    MountInfo info;
    info.uuid = QUuid::createUuid();
    info.MountPoint = "Z:";
    info.RootDirectory = R"(\\192.168.1.4\temp)";
    info.volname = "temp";


    info.options |= DOKAN_OPTION_MOUNT_MANAGER;
    info.debug = true;

    info.enabled = true;
    info.keepAlive = true;

    mounts << info;

    settings.setValue("Mounts",QVariant::fromValue(mounts));

    settings.sync();


//    QThread daemonThread;
//    Daemon *d;
//    d = new Daemon();
////    daemonThread.connect(&daemonThread,&QThread::started,[&](){
////        d = new Daemon();
////        d->moveToThread(&daemonThread);
////    });

//     d->moveToThread(&daemonThread);

//    daemonThread.start();

//    QtServiceController controller("Seamless smb");

//    controller.start();

//    controller.uninstall();

   int ret = app.exec();


//   delete d;
//   daemonThread.exit();

   return ret;


}
