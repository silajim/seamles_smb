#include "socket.h"
#include <QUuid>

Socket::Socket(QObject *parent) : QObject(parent)
{
    lsocket = new QLocalServer(this);
    connect(lsocket,&QLocalServer::newConnection,this,&Socket::onnewConnection);
    lsocket->listen("seamless_smb_service");

}

Socket::~Socket()
{
    foreach(auto sock , sockets){
        sock->close();
        sock->deleteLater();
    }
}

void Socket::onnewConnection()
{
    if(lsocket->hasPendingConnections()){
        QLocalSocket *sock = lsocket->nextPendingConnection();
        connect(sock,&QLocalSocket::aboutToClose,this,&Socket::onaboutToClose);
        connect(sock,&QLocalSocket::readyRead,this,&Socket::onReadyRead);
        sockets << sock;
    }
}

void Socket::onaboutToClose()
{
    QLocalSocket *sock = qobject_cast<QLocalSocket*>(sender());
    if(!sock)
        return;
    for(int i=0;i<sockets.size();i++){
        if(sockets[i]==sock){
            sockets.removeAt(i);
            break;
        }
    }
    sock->deleteLater();
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
