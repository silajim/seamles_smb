#ifndef SERVER_H
#define SERVER_H

#include <QObject>

#include <QSettings>
#include <QLocalServer>
#include <QLocalSocket>

#include <mutex>

class Daemon;

class Server : public QObject
{
    Q_OBJECT
public:
    explicit Server(QObject *parent = nullptr);
    ~Server();

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
