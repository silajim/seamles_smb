#ifndef SOCKET_H
#define SOCKET_H

#include <QObject>
#include <QLocalSocket>

class Socket : public QObject
{
    Q_OBJECT
public:
    explicit Socket(QObject *parent = nullptr);

public slots:
    void sendReload();

signals:

private slots:
    void onSocketError(QLocalSocket::LocalSocketError error);

private:
    QLocalSocket *localsocket=nullptr;

};

Q_DECLARE_METATYPE(QLocalSocket::LocalSocketError)
Q_DECLARE_METATYPE(QLocalSocket::LocalSocketState)

#endif // SOCKET_H
