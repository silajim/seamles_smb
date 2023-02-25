#ifndef CLIENT_H
#define CLIENT_H

#include <QObject>
#include <QLocalSocket>
#include <QUuid>
#include <QMutex>
#include <QMutexLocker>
#include <QTimer>

class Client : public QObject
{
    Q_OBJECT
public:
    explicit Client(QObject *parent = nullptr);
    void close();

public slots:
    void sendReload();
    void start();

signals:
    void status(QUuid id,bool running);

private slots:
    void onConnectTimer();
    void onSocketError(QLocalSocket::LocalSocketError error);
    void onReadyRead();
    void closeSlot();

private:
    QLocalSocket *localsocket=nullptr;
    QMutex mutex;
    QTimer *connectTimer;

};

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)
Q_DECLARE_METATYPE(QLocalSocket::LocalSocketState)

#endif // SOCKET_H
