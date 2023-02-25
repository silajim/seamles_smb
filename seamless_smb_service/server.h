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
    void sendStatusSlot(QUuid id, bool running);
    void start();

signals:
    void reloadMounts();
    void mountAll();
    void unmountAll();
    void mount(QUuid uuid);
    void unmount(QUuid uuid);

    void mountok(QUuid uuid);
    void mounterr(QUuid uuid);


private:
    QLocalServer *lsocket=nullptr;
    QList<QLocalSocket*> sockets;
    QLocalSocket* controller=nullptr;

    std::mutex mutex;
private slots:
    void onnewConnection();
    void onaboutToClose();
    void onReadyRead();

};

#endif // SOCKET_H
