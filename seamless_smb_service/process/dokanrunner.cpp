#include "dokanrunner.h"

#include <dokan/dokan.h>
#include "common.h"
#include "filemount.h"
#include "globals.h"
#include "logger.h"

#include <QUuid>
#include <QSettings>
#include <QtCore>

DokanRunner::DokanRunner(int argc, char *argv[])
{
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
        DokanInit();
        filemount->mount();
    }
    connect(&saveSecurityTimer,&QTimer::timeout,this,&DokanRunner::onSaveSecurity);
    saveSecurityTimer.start(60*1000); //1 Minute

}

DokanRunner::~DokanRunner()
{
    if(filemount && filemount->isRunning()){
        filemount->unmount();
    }
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

void DokanRunner::onSaveSecurity()
{
    filemount->saveSecurity();
}
