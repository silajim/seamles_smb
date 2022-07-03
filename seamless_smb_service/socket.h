#ifndef SOCKET_H
#define SOCKET_H

#include <QObject>

#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>

class Daemon;

class Socket : public QObject
{
    Q_OBJECT
public:
    explicit Socket(QObject *parent = nullptr);
    ~Socket();

signals:
    void reloadMounts();
    void mountAll();
    void unmountAll();
    void mount(QUuid uuid);
    void unmount(QUuid uuid);


private:
    QLocalServer *lsocket=nullptr;
    QList<QLocalSocket*> sockets;

private slots:
    void onnewConnection();
    void onaboutToClose();
    void onReadyRead();

};

#endif // SOCKET_H
