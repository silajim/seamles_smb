#include "server.h"
#include <QUuid>
#include <QMetaObject>

#include <thread>

Server::Server(QObject *parent) : QObject(parent)
{
    lsocket = new QLocalServer(this);
    connect(lsocket,&QLocalServer::newConnection,this,&Server::onnewConnection);
    lsocket->listen("seamless_smb_service");

}

Server::~Server()
{
//    foreach(auto sock , sockets){
//        sock->close();
//        sock->deleteLater();
//    }
    sockets->deleteLater();
}

void Server::sendStatus(QUuid id, bool running)
{
    QMetaObject::invokeMethod(this,"sendStatusSlot",Qt::QueuedConnection,Q_ARG(QUuid,id),Q_ARG(bool,running));
}

void Server::sendStatusSlot(QUuid id, bool running)
{
    std::lock_guard<std::mutex> lg(mutex);
    if(!sockets){
//        qDebug() << "No socket";
        return;
    }

    QString msg("STATUS %1 %2\n");
    msg = msg.arg(id.toString()).arg(running);
    try{
    qint64 bytes = sockets->write(msg.toLatin1());
    }catch(...){
        qDebug() << "Socket exception";
    }

//    foreach(QLocalSocket* sock , sockets){
//        sock->write(msg.toUtf8());
//    }
//    qDebug() << "Status wrote" << msg << "bytes" << bytes;
}

void Server::onnewConnection()
{
    if(lsocket->hasPendingConnections()){
        QLocalSocket *sock = lsocket->nextPendingConnection();
        connect(sock,&QLocalSocket::aboutToClose,this,&Server::onaboutToClose);
        connect(sock,&QLocalSocket::readyRead,this,&Server::onReadyRead);
        sockets = sock;
    }
}

void Server::onaboutToClose()
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

void Server::onReadyRead()
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
        emit unmount(QUuid(suid));
    }else if(command.startsWith("start/")){
        QString suid = command.remove("start/");
        emit mount(QUuid(suid));
    }
}
