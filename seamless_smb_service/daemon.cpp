#include "daemon.h"
#include "logger.h"
#include "qabstracteventdispatcher.h"
#include <QEventLoop>
#include <QString>
#include <QStandardPaths>
#include <QUrl>
#include <QByteArray>
#include <QCoreApplication>
#include <QDir>

#include <QtConcurrent/QtConcurrentRun>

Daemon::Daemon(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<mlist>("mlist");
    QStringList plist = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    QString     path; //= QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    foreach (QString p, plist)
    {
        if (p.contains("ProgramData"))
        {
            path = p;
            break;
        }
    }

    if (path.isEmpty())
    {
        qFatal("Cannot determine settings storage location");
        return;
    }

    //    qRegisterMetaTypeStreamOperators<mlist>("mlist");
    settings = std::make_shared<QSettings>(path + "/mounts.ini", QSettings::IniFormat);
    qDebug() << "Daemon path" << path;
}

void Daemon::start()
{
    stats = new QTimer(this);
    sockThread = new QThread(this);
    sock = new Server();
    sock->moveToThread(sockThread);
    connect(sockThread, &QThread::started, sock, &Server::start);
    sockThread->start();
    connect(sock, &Server::destroyed, sockThread, &QThread::quit, Qt::DirectConnection);

    //    connect(sock,&Server::destroyed,this,[this](){
    //        qDebug() << "Server Destroyed";
    ////        sockThread->exit(0);
    //    },Qt::DirectConnection);

    connect(sock, &Server::reloadMounts, this, &Daemon::reloadMounts);
    connect(sock, &Server::mount, this, &Daemon::mount);
    connect(sock, &Server::mountAll, this, &Daemon::mountAll);

    statTimer = settings->value("statTimer", 5000).toUInt();
    connect(stats, &QTimer::timeout, this, &Daemon::checkStatus);

    //    DokanInit();
    reloadMounts();

    stats->start(statTimer);
}

Daemon::~Daemon()
{
    qDebug() << Q_FUNC_INFO;
    stats->stop();
    stats->deleteLater();
    unmountAll();
    for (auto &mount : mounts)
    {
        if (mount.first->state() == QProcess::ProcessState::Running)
        {
            if (!mount.first->waitForFinished(2000))
            {
                qDebug() << "Killing process";
                mount.first->kill();
            }
            bool running = mount.first->state() == QProcess::Running;
            sock->sendStatusSlot(mount.second.uuid, running);
            //            sockThread->eventDispatcher()->processEvents(QEventLoop::ProcessEventsFlag::AllEvents);
        }
    }
    QCoreApplication::processEvents();

    //    delete sock;

    //    delete sock;
    sock->deleteLater();
    QCoreApplication::processEvents();

    //    sockThread->exit();

    sockThread->wait(1000);
    QCoreApplication::processEvents();
    delete sockThread;

    qDebug() << Q_FUNC_INFO << "END";
}

void Daemon::reloadMounts()
{
    qDebug() << Q_FUNC_INFO;
    settings->sync();
    mlist mountlist;
    mountlist = settings->value("Mounts").value<mlist>();

    mlist addl, modifyl, rm;
    toaddModify(mountlist, addl, modifyl);

    // qDebug() << "Add list";
    // foreach (auto m, addl)
    // {
    //     qDebug() << m.RootDirectory << " " << m.enabled << m.UNCName << m.MountPoint;
    // }
    // qDebug() << "Modify list";
    // foreach (auto m, modifyl)
    // {
    //     qDebug() << m.RootDirectory << " " << m.enabled << m.UNCName << m.MountPoint;
    // }

    rm = toRemove(mountlist);
    remove(rm);
    add(addl);
    modify(modifyl);
    //    qDebug() << "Mount all";
    mountAll();
}

void Daemon::toaddModify(mlist newlist, mlist &toadd, mlist &tomodify)
{
    foreach (auto info, newlist)
    {
        bool found = false;
        foreach (auto mount, mounts)
        {
            if (info.uuid == mount.second.uuid)
            {
                found = true;
                if (info != mount.second)
                {
                    tomodify << info;
                }
                break;
            }
        }
        if (!found)
            toadd << info;
    }
}

mlist Daemon::toRemove(mlist newlist)
{
    mlist rm;
    foreach (auto mount, mounts)
    {
        bool found = false;
        foreach (auto info, newlist)
        {
            if (info.uuid == mount.second.uuid)
            {
                found = true;
                break;
            }
        }
        if (!found)
        {
            rm << mount.second;
        }
    }
    return rm;
}

void Daemon::remove(mlist rm)
{
    foreach (auto info, rm)
    {
        remove(info);
    }
}

void Daemon::remove(const MountInfo &info)
{
    for (int i = 0; i < mounts.size(); i++)
    {
        auto mount = mounts[i];
        if (info.uuid == mount.second.uuid)
        {
            mount.first->terminate();
            mounts.removeAt(i);
            break;
        }
    }
}

void Daemon::add(mlist _add)
{
    foreach (auto inf, _add)
    {
        add(inf);
        QThread::sleep(1);
    }
}

void Daemon::add(MountInfo info)
{
    qDebug() << Q_FUNC_INFO;

    auto p = std::make_shared<QProcess>(this);

    p->setProgram(QCoreApplication::applicationDirPath() + "/" + "seamless_smb_service_process.exe");
    p->setArguments({settings->fileName(), info.uuid.toString()});
    qDebug() << info.RootDirectory << "ARGS" << p->arguments();
    auto pair = qMakePair(p, info);
    mounts << pair;
}

void Daemon::modify(mlist mod)
{
    foreach (auto inf, mod)
    {
        modify(inf);
    }
}

void Daemon::modify(const MountInfo &info)
{
    for (int i = 0; i < mounts.size(); i++)
    {
        auto mount = mounts[i];
        if (info.uuid == mount.second.uuid)
        {
            mount.first->terminate();

            auto p = std::make_shared<QProcess>(this);
            p->setProgram(QCoreApplication::applicationDirPath() + "/" + "seamless_smb_service_process.exe");
            p->setArguments({settings->fileName(), info.uuid.toString()});

            auto pair = qMakePair(p, info);

            mounts.replace(i, pair);
            break;
        }
    }
}

void Daemon::mountAll()
{
    QtConcurrent::run([this]() {
        mountAllrunning = true;
        foreach (auto mount, mounts)
        {
            QDir dir(mount.second.RootDirectory);
            if (dir.exists() && mount.second.enabled)
            {
                QMetaObject::invokeMethod(this, "mount", Qt::BlockingQueuedConnection, Q_ARG(QUuid, mount.second.uuid));
                QThread::msleep(500);
            }
        }
        mountAllrunning = false;
    });
}

void Daemon::unmountAll()
{
    foreach (auto mount, mounts)
    {
        mount.first->terminate();
    }
}

void Daemon::mount(QUuid uuid)
{
    foreach (auto mount, mounts)
    {
        if (mount.second.uuid == uuid)
        {
            mount.first->start();
            mount.first->waitForStarted(500);
            bool running = mount.first->state() == QProcess::Running;
            sock->sendStatus(mount.second.uuid, running);

            if (running)
            {
                auto   pid = mount.first->processId();
                HANDLE Handle = OpenProcess(PROCESS_SET_INFORMATION, FALSE, pid);
                if (Handle)
                {
                    if (!SetPriorityClass(Handle, HIGH_PRIORITY_CLASS))
                    {
                        qDebug() << mount.second.RootDirectory << "Unable to set process priority:" << GetLastError();
                    }
                    else
                    {
                        qDebug() << mount.second.RootDirectory << "Process priority set to High";
                    }
                    CloseHandle(Handle);
                }
                else
                {
                    qDebug() << mount.second.RootDirectory << "Unable to OPEN process to set priority:" << GetLastError();
                }
            }

            break;
        }
    }
}

// void Daemon::mountSlot(QUuid uuid)
//{
//     mount(uuid);
// }

void Daemon::unmount(QUuid uuid)
{
    foreach (auto mount, mounts)
    {
        if (mount.second.uuid == uuid)
        {
            mount.first->terminate();
            bool running = mount.first->state() == QProcess::Running;
            sock->sendStatus(mount.second.uuid, running);
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

    memset(&dokanOptions, 0, sizeof(DOKAN_OPTIONS));
    dokanOptions.MountPoint = g->MountPoint().data();
    dokanOptions.Options = info.options;
    dokanOptions.SingleThread = info.singlethreaded;
}

void Daemon::checkStatus()
{
    //    qDebug() << "CheckStatus";
    QFuture<void> f = QtConcurrent::run([this]() {
        foreach (auto amount, mounts)
        {
            //            qDebug() << amount.second.RootDirectory << " " << amount.second.enabled;

            bool running = amount.first->state() == QProcess::Running;
            sock->sendStatus(amount.second.uuid, running);

            if (!mountAllrunning && amount.second.enabled && amount.second.keepAlive && !running)
            {
                qDebug() << "ReMounting" << amount.second.RootDirectory;
                QMetaObject::invokeMethod(this, "mount", Qt::BlockingQueuedConnection, Q_ARG(QUuid, amount.second.uuid));
                QThread::msleep(500);
                qDebug() << "Done ReMounting";
            }
        }
    });
}
