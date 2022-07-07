#include "socket.h"
#include <QUuid>
#include <QMetaObject>

#include <thread>

Socket::Socket(QObject *parent) : QObject(parent)
{
    lsocket = new QLocalServer(this);
    connect(lsocket,&QLocalServer::newConnection,this,&Socket::onnewConnection);
    lsocket->listen("seamless_smb_service");

}

Socket::~Socket()
{
//    foreach(auto sock , sockets){
//        sock->close();
//        sock->deleteLater();
//    }
    sockets->deleteLater();
}

void Socket::sendStatus(QUuid id, bool running)
{
    QMetaObject::invokeMethod(this,"sendStatusSlot",Qt::QueuedConnection,Q_ARG(QUuid,id),Q_ARG(bool,running));
}

void Socket::sendStatusSlot(QUuid id, bool running)
{
    std::lock_guard<std::mutex> lg(mutex);
    if(!sockets){
        qDebug() << "No socket";
        return;
    }

    QString msg("STATUS %1 %2\n");
    msg = msg.arg(id.toString()).arg(running);
    qint64 bytes = sockets->write(msg.toLatin1());

//    foreach(QLocalSocket* sock , sockets){
//        sock->write(msg.toUtf8());
//    }
    qDebug() << "Status wrote" << msg << "bytes" << bytes;
}

void Socket::onnewConnection()
{
    if(lsocket->hasPendingConnections()){
        QLocalSocket *sock = lsocket->nextPendingConnection();
        connect(sock,&QLocalSocket::aboutToClose,this,&Socket::onaboutToClose);
        connect(sock,&QLocalSocket::readyRead,this,&Socket::onReadyRead);
        sockets = sock;
    }
}

void Socket::onaboutToClose()
{
    QLocalSocket *sock = qobject_cast<QLocalSocket*>(sender());
    if(!sock)
        return;
//    for(int i=0;i<sockets.size();i++){
//        if(sockets[i]==sock){
//            sockets.removeAt(i);
//            break;
//        }
//    }
    sockets->deleteLater();
    sockets = nullptr;

}

void Socket::onReadyRead()
{
    QLocalSocket *sock = qobject_cast<QLocalSocket*>(sender());
    if(!sock)
        return;

    QByteArray arr =  sock->readLine();
    QString command = arr;
    qDebug() << "Read Command" << arr;
    if(command=="reload\n"){
        //        QtConcurrent::run(this,&Daemon::reloadMounts);
        emit reloadMounts();
    }else if(command.startsWith("stop/")){
        QString suid = command.remove("stop/");
        emit unmount(suid);
    }else if(command.startsWith("start/")){
        QString suid = command.remove("start/");
        emit mount(suid);
    }
}
