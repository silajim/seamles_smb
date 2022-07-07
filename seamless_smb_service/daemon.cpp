#include "daemon.h"
#include "logger.h"
#include <QString>
#include <QStandardPaths>
#include <QUrl>
#include <QByteArray>

#include <QtConcurrent/QtConcurrentRun>

Daemon::Daemon(QObject *parent):QObject(parent)
{
    QStringList plist = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    QString path; //= QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    foreach(QString p, plist ){
        if(p.contains("ProgramData")){
            path = p;
            break;
        }
    }

    if (path.isEmpty()){
        qFatal("Cannot determine settings storage location");
        return;
    }


    qRegisterMetaTypeStreamOperators<mlist>("mlist");
    settings = std::make_shared<QSettings>(path+"/mounts.ini", QSettings::IniFormat);
    qDebug() << "Daemon path" << path;

    sock = new Socket();
    sockThread = new QThread(this);
    sock->moveToThread(sockThread);
    sockThread->start();
    connect(sock,&Socket::reloadMounts,this,&Daemon::reloadMounts);
    connect(sock,&Socket::mount,this,&Daemon::mount);
    connect(sock,&Socket::mountAll,this,&Daemon::mountAll);


    statTimer = settings->value("statTimer",5000).toUInt();
    connect(&stats,&QTimer::timeout,this,&Daemon::checkStatus);


    DokanInit();
    reloadMounts();

    stats.start(statTimer);

}

Daemon::~Daemon()
{
    disconnect();
    sock->deleteLater();
    sockThread->exit();
    unmountAll();
    DokanShutdown();
}

void Daemon::reloadMounts()
{
    qDebug() << Q_FUNC_INFO;
    settings->sync();
    mlist mountlist;
    mountlist = settings->value("Mounts").value<mlist>();

    mlist addl , modifyl , rm;
    toaddModify(mountlist,addl,modifyl);

    qDebug() << "Add list";
    foreach(auto m , addl){
        qDebug() << m.RootDirectory << " " << m.enabled;
    }
    qDebug() << "Modify list";
    foreach(auto m , modifyl){
        qDebug() << m.RootDirectory << " " << m.enabled;
    }

    rm = toRemove(mountlist);
    remove(rm);
    add(addl);
    modify(modifyl);
    qDebug() << "Mount all";
    mountAll();
}

void Daemon::toaddModify(mlist newlist,mlist &toadd,mlist &tomodify)
{
    foreach(auto info,newlist){
        bool found = false;
        foreach(auto mount, mounts){
            if(info.uuid == mount.second.uuid){
                found = true;
                if(info != mount.second){
                    tomodify << info;
                }
                break;
            }
        }
        if(!found)
            toadd << info;
    }
}

mlist Daemon::toRemove(mlist newlist)
{
    mlist rm;
    foreach(auto mount , mounts){
        bool found=false;
        foreach(auto info , newlist){
            if(info.uuid == mount.second.uuid){
                found = true;
                break;
            }
        }
        if(!found){
            rm << mount.second;
        }
    }
    return rm;
}

void Daemon::remove(mlist rm)
{
    foreach(auto info,rm){
        remove(info);
    }
}

void Daemon::remove(const MountInfo &info)
{
    for(int i=0;i<mounts.size();i++){
        auto mount = mounts[i];
        if(info.uuid == mount.second.uuid){
            mount.first->unmount();
            mounts.removeAt(i);
            break;
        }
    }
}

void Daemon::add(mlist _add)
{
    foreach(auto inf , _add){
        add(inf);
    }
}

void Daemon::add(MountInfo info)
{
    DOKAN_OPTIONS dokanOptions;
    std::shared_ptr<Globals> globals;

    MountInfoToGlobal(info,globals,dokanOptions);

    std::shared_ptr<Logger> log = std::make_shared<Logger>(info.debug);

    std::shared_ptr<FileMount> filemount = std::make_shared<FileMount>(globals,info.debug,info.cerr,dokanOptions,log);

    auto pair = qMakePair<std::shared_ptr<FileMount>,MountInfo>(filemount,info);
    mounts << pair;
}

void Daemon::modify(mlist mod)
{
    foreach(auto inf,mod){
        modify(inf);
    }
}

void Daemon::modify(const MountInfo &info)
{
    for(int i=0;i<mounts.size();i++){
        auto mount = mounts[i];
        if(info.uuid == mount.second.uuid){
            mount.first->unmount();

            DOKAN_OPTIONS dokanOptions;
            std::shared_ptr<Globals> globals;

            MountInfoToGlobal(info,globals,dokanOptions);

            std::shared_ptr<FileMount> filemount = std::make_shared<FileMount>(globals,info.debug,info.cerr,dokanOptions);

            auto pair = qMakePair<std::shared_ptr<FileMount>,MountInfo>(filemount,info);

            mounts.replace(i,pair);
            break;
        }
    }
}

void Daemon::mountAll()
{
     foreach(auto mount, mounts){
         if(mount.second.enabled)
             mount.first->mount();
     }
}

void Daemon::unmountAll()
{
    foreach(auto mount, mounts){
        mount.first->unmount();
    }
}

void Daemon::mount(QUuid uuid)
{
    foreach(auto mount, mounts){
        if(mount.second.uuid == uuid){
            mount.first->mount();
            break;
        }
    }
}

void Daemon::unmount(QUuid uuid)
{
    foreach(auto mount, mounts){
        if(mount.second.uuid == uuid){
            mount.first->unmount();
            break;
        }
    }
}

void Daemon::MountInfoToGlobal(MountInfo info, std::shared_ptr<Globals> &g, DOKAN_OPTIONS &dokanOptions)
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

void Daemon::checkStatus()
{
    qDebug() << "CheckStatus";
    QtConcurrent::run([this](){
        foreach(auto amount, mounts){
            qDebug() << amount.second.RootDirectory << " " << amount.second.enabled;

            bool running = amount.first->isRunning();
            sock->sendStatus(amount.second.uuid,running);

            if(amount.second.enabled && amount.second.keepAlive && !running){
                mount(amount.second.uuid);
            }
        }
    });
}
