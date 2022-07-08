#ifndef DAEMON_H
#define DAEMON_H

#include <QObject>
#include "common.h"
#include "filemount.h"
#include "globals.h"
#include "server.h"

#include <QList>
#include <QMap>
#include <QPair>
#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>
#include <QThread>
#include <QTimer>


typedef QList<MountInfo> mlist;

class Daemon: public QObject
{
    Q_OBJECT
public:
    Daemon(QObject *parent =nullptr);
    ~Daemon();


public slots:
    void reloadMounts();
    void mountAll();
    void unmountAll();
    void mount(QUuid uuid);
    void unmount(QUuid uuid);

private:

    void toaddModify(mlist newlist, mlist &toadd,mlist &tomodify);
    mlist toRemove(mlist newlist);
    void remove(mlist rm);
    void remove(const MountInfo &info);
    void add(mlist _add);
    void add(MountInfo info);
    void modify(mlist mod);
    void modify(const MountInfo &info);


    void MountInfoToGlobal(MountInfo info, std::shared_ptr<Globals> &g, DOKAN_OPTIONS &dokanOptions);



    QList<QPair<std::shared_ptr<FileMount>,MountInfo>> mounts;
    std::shared_ptr<QSettings> settings;
//    QLocalServer *lsocket=nullptr;

//    QList<QLocalSocket*> sockets;

    unsigned int statTimer=0;
    QTimer stats;

    Server *sock=nullptr;

    QThread *sockThread=nullptr;

private slots:
    void checkStatus();

};

#endif // DAEMON_H
