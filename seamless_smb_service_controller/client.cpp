#include "client.h"

Client::Client(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    localsocket = new QLocalSocket(this);
//    localsocket->setReadBufferSize(16000);
    connect(localsocket,&QLocalSocket::errorOccurred,this,&Client::onSocketError);
    connect(localsocket,&QLocalSocket::stateChanged,[](QLocalSocket::LocalSocketState state){
        qDebug() << "SOCKET STATE" << state;
    });
    connect(&connectTimer,&QTimer::timeout,this,&Client::onConnectTimer);
    connectTimer.setSingleShot(true);
    localsocket->connectToServer("seamless_smb_service");
    if(!localsocket->waitForConnected()){
        connectTimer.start(30*1000);
    }

    connect(localsocket,&QLocalSocket::readyRead,this,&Client::onReadyRead);
}

void Client::close()
{
     QMutexLocker lk(&mutex);
    localsocket->close();
    localsocket->waitForDisconnected(3000);
}

void Client::sendReload()
{
     QMutexLocker lk(&mutex);
    if(localsocket->state() == QLocalSocket::LocalSocketState::ConnectedState){
        qDebug() << "Sending Reload";
        localsocket->write("reload\n");
        localsocket->flush();
    }
}

void Client::onConnectTimer()
{
    localsocket->connectToServer("seamless_smb_service");
    if(!localsocket->waitForConnected()){
        connectTimer.start(30*1000);
    }
}

void Client::onSocketError(QLocalSocket::LocalSocketError error)
{
    qDebug() << Q_FUNC_INFO << error;
}

void Client::onReadyRead(){

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
            emit status(QUuid(st[1]),st[2].toInt());
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


