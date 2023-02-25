#include "dokanrunner.h"

#include <dokan/dokan.h>
#include "common.h"
#include "filemount.h"
#include "globals.h"
#include "logger.h"
#include <string>

#include <QUuid>
#include <QSettings>
#include <QtCore>
#include "freeDrives.h"



DokanRunner::DokanRunner(int argc, char *argv[]):QObject(nullptr)
{
    qDebug() << Q_FUNC_INFO;
    //    connect(qApp,&QCoreApplication::aboutToQuit,this,[&](){
    //        qDebug() << "About to quit";
    //        myexit(0);
    //    },Qt::DirectConnection);

    connect(this,&DokanRunner::exits,qApp,&QCoreApplication::exit,Qt::QueuedConnection);

    qRegisterMetaType<mlist>("mlist");
    QString mountsPath = QString::fromStdString(argv[1]);
    QUuid id = QUuid::fromString(QString::fromStdString(argv[2]));

    QSettings settings(mountsPath, QSettings::IniFormat);

    settings.sync();
    mlist mountlist;
    mountlist = settings.value("Mounts").value<mlist>();

    for(MountInfo &info:mountlist){
        if(info.uuid == id){
            DOKAN_OPTIONS dokanOptions;
            std::shared_ptr<Globals> globals;

            MountInfoToGlobal(info,globals,dokanOptions);

            QString savePath  = QCoreApplication::applicationDirPath()+"/"+info.RootDirectory.replace("\\","_");
            qDebug() << "SAVE LOGGER" << savePath;

            std::shared_ptr<Logger> log = std::make_shared<Logger>(savePath,info.debug);

            qDebug() << "Log Created";

            filemount = std::make_shared<FileMount>(globals,info.debug,info.cerr,dokanOptions,log);
            break;
        }
    }

    if(filemount){
        qDebug() << "Mount Point" << filemount->getMountPoint();
        auto drives = freeDrives();
        const std::wstring mountPoint = filemount->getMountPoint();
        if(drives.contains(QString::fromWCharArray(mountPoint.c_str(),1))){
            DokanInit();
            if(filemount->mount()!= DOKAN_SUCCESS)
                //                                qApp->exit(2);
               emit exits(2);
            //                myexit(2);
            drives = freeDrives();
            if(drives.contains(QString::fromWCharArray(mountPoint.c_str(),1))){
                //                               qApp->exit(3);
                emit exits(3);
                //                myexit(3);
            }
        }else{
            //                         qApp->exit(3);
            emit exits(3);
            //            myexit(3);
        }
    }
    connect(&saveSecurityTimer,&QTimer::timeout,this,&DokanRunner::onSaveSecurity);
    saveSecurityTimer.start(60*1000); //1 Minute

}

DokanRunner::~DokanRunner()
{
    QFile f(QCoreApplication::applicationDirPath()+"/--DD");
    f.open(QFile::ReadWrite | QFile::Append);
    QTextStream out(&f);
    out << filemount->getMountPoint() << Qt::endl;

    qDebug() << Q_FUNC_INFO;
    if(filemount && filemount->isRunning()){
        filemount->unmount();
    }
    DokanShutdown();
}

void DokanRunner::unmount()
{
    if(filemount && filemount->isRunning()){
        filemount->unmount();
    }
}

void DokanRunner::MountInfoToGlobal(MountInfo info, std::shared_ptr<Globals> &g, DOKAN_OPTIONS &dokanOptions)
{
    g = std::make_shared<Globals>();
    g->setRootDirectory(info.RootDirectory.toStdWString());
    g->setMountPoint(info.MountPoint.toStdWString());
    g->setUNCName(info.UNCName.toStdWString());
    g->setCaseSensitive(info.CaseSensitive);
    g->setImpersonateCallerUser(info.ImpersonateCallerUser);
    g->setVolname(info.volname.toStdWString());
    g->setHasSeSecurityPrivilege(info.HasSeSecurityPrivilege);

    memset(&dokanOptions,0,sizeof (DOKAN_OPTIONS));
    dokanOptions.MountPoint = g->MountPoint().data();
    dokanOptions.Options = info.options;
    dokanOptions.SingleThread = info.singlethreaded;
}

void DokanRunner::myexit(int code)
{
    if(filemount && filemount->isRunning()){
        filemount->unmount();
    }
    DokanShutdown();
    exit(code);
}

void DokanRunner::onSaveSecurity()
{
    filemount->saveSecurity();
}
