#ifndef SOCKET_H
#define SOCKET_H

#include <QObject>

#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>

#include "mutex"

class Daemon;

class Socket : public QObject
{
    Q_OBJECT
public:
    explicit Socket(QObject *parent = nullptr);
    ~Socket();

public slots:
    void sendStatus(QUuid id, bool running);

signals:
    void reloadMounts();
    void mountAll();
    void unmountAll();
    void mount(QUuid uuid);
    void unmount(QUuid uuid);


private:
    QLocalServer *lsocket=nullptr;
    QLocalSocket* sockets=nullptr;

    std::mutex mutex;
private slots:
    void onnewConnection();
    void onaboutToClose();
    void onReadyRead();
    void sendStatusSlot(QUuid id, bool running);

};

#endif // SOCKET_H
