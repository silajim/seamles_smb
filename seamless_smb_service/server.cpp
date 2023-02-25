#include "server.h"
#include <QUuid>
#include <QMetaObject>

#include <thread>
#include <QCoreApplication>
Server::Server(QObject *parent) : QObject(parent)
{


}

void Server::start()
{
    lsocket = new QLocalServer(this);
    connect(lsocket,&QLocalServer::newConnection,this,&Server::onnewConnection,Qt::QueuedConnection);
    qDebug() << "Server Socket started, listen:" << lsocket->listen("seamless_smb_service");
}


Server::~Server()
{
    qDebug() << Q_FUNC_INFO;
//    QCoreApplication::processEvents();
    foreach(QLocalSocket* sock , sockets){
        sock->flush();
        sock->close();
//        delete sock;
        sock->deleteLater();
    }
    //    sockets->deleteLater();

      qDebug() << Q_FUNC_INFO << "END";
}

void Server::sendStatus(QUuid id, bool running)
{
//    qDebug() << Q_FUNC_INFO;
    QMetaObject::invokeMethod(this,"sendStatusSlot",Qt::QueuedConnection,Q_ARG(QUuid,id),Q_ARG(bool,running));
}


void Server::sendStatusSlot(QUuid id, bool running)
{
    std::lock_guard<std::mutex> lg(mutex);
    if(!controller){
//        qDebug() << "No socket";
        return;
    }

    QString msg("STATUS %1 %2\n");
    msg = msg.arg(id.toString()).arg(running);
    try{
    qint64 bytes = controller->write(msg.toLatin1());
    }catch(...){
        qDebug() << "Socket exception";
    }

//    foreach(QLocalSocket* sock , sockets){
//        sock->write(msg.toUtf8());
//    }
//    qDebug() << "Status wrote" << msg;
}

void Server::onnewConnection()
{
    if(lsocket->hasPendingConnections()){
        QLocalSocket *sock = lsocket->nextPendingConnection();
        connect(sock,&QLocalSocket::aboutToClose,this,&Server::onaboutToClose);
        connect(sock,&QLocalSocket::readyRead,this,&Server::onReadyRead);
        qDebug() << "New Connection Bytes available" << sock->bytesAvailable() << "Can read line" << sock->canReadLine();
        if(sock->bytesAvailable()>0){
            emit sock->readyRead();
        }
        sockets << sock;
    }
}

void Server::onaboutToClose()
{
    QLocalSocket *sock = qobject_cast<QLocalSocket*>(sender());
    if(!sock)
        return;
    for(int i=0;i<sockets.size();i++){
        if(sockets[i]==sock){
            QLocalSocket* s = sockets[i];
            sockets.removeAt(i);
            if(s==controller)
                controller = nullptr;
            s->deleteLater();
            break;
        }
    }
//    sockets->deleteLater();
//    sockets = nullptr;

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
    }else if(command == "CONTROLLER"){
        controller = sock;
    }else if(command.startsWith("mountok/")){
        QString suid = command.remove("mountok/");
        emit mountok(QUuid(suid));
    }else if(command.startsWith("mounterr/")){
        QString suid = command.remove("mounterr/");
        emit mounterr(QUuid(suid));
    }
}
