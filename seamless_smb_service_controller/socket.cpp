#include "socket.h"

Socket::Socket(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    localsocket = new QLocalSocket(this);
//    localsocket->setReadBufferSize(16000);
    connect(localsocket,&QLocalSocket::errorOccurred,this,&Socket::onSocketError);
    connect(localsocket,&QLocalSocket::stateChanged,[](QLocalSocket::LocalSocketState state){
        qDebug() << "SOCKET STATE" << state;
    });
    localsocket->connectToServer("seamless_smb_service");

    connect(localsocket,&QLocalSocket::readyRead,this,&Socket::onReadyRead);
}

void Socket::close()
{
     QMutexLocker lk(&mutex);
    localsocket->close();
    localsocket->waitForDisconnected(3000);
}

void Socket::sendReload()
{
     QMutexLocker lk(&mutex);
    if(localsocket->state() == QLocalSocket::LocalSocketState::ConnectedState){
        qDebug() << "Sending Reload";
        localsocket->write("reload\n");
        localsocket->flush();
    }
}

void Socket::onSocketError(QLocalSocket::LocalSocketError error)
{
    qDebug() << Q_FUNC_INFO << error;
}

void Socket::onReadyRead(){

    QMutexLocker lk(&mutex);
//    bool looped = false;
    while(localsocket->canReadLine()){
//        looped = true;
         QByteArray arr;
        try{
             arr =  localsocket->readLine();
        }catch(...){
            continue;
        }

        qDebug() << "Read Line" << arr;
        QString command = arr;

        if(command.startsWith("STATUS")){
            QStringList st = command.split(" ");
            emit status(st[1],st[2].toInt());
        }
        qint64 bytes = localsocket->bytesAvailable();
        if(bytes>0 && !localsocket->canReadLine()){
            try{
            localsocket->readAll();
            }catch(...){
                continue;
            }
        }
    }

}


