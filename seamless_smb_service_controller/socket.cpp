#include "socket.h"

Socket::Socket(QObject *parent) : QObject(parent)
{
    qRegisterMetaType<QLocalSocket::LocalSocketError>("QLocalSocket::LocalSocketError");
    qRegisterMetaType<QLocalSocket::LocalSocketState>("QLocalSocket::LocalSocketState");
    localsocket = new QLocalSocket(this);
    connect(localsocket,&QLocalSocket::errorOccurred,this,&Socket::onSocketError);
    connect(localsocket,&QLocalSocket::stateChanged,[](QLocalSocket::LocalSocketState state){
       qDebug() << "SOCKET STATE" << state;
    });
    localsocket->connectToServer("seamless_smb_service");
}

void Socket::sendReload()
{
    if(localsocket->state() == QLocalSocket::LocalSocketState::ConnectedState){
        qDebug() << "Sending Reload";
        localsocket->write("reload\n");
    }
}

void Socket::onSocketError(QLocalSocket::LocalSocketError error)
{
    qDebug() << Q_FUNC_INFO << error;
}
