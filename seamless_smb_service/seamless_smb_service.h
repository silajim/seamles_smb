#ifndef SEAMLESS_SMB_SERVICE_H
#define SEAMLESS_SMB_SERVICE_H

#include "QtService.h"
#include "daemon.h"

#include <QObject>
#include <QThread>

class seamless_smb_service:public QObject , public QtService<QCoreApplication>
{
    Q_OBJECT
public:
    seamless_smb_service(int argc, char **argv);


    // QtServiceBase interface
public:
    void start() override;
    void stop() override;
    void processCommand(int code) override;

private:
    QThread *dthread=nullptr;
    Daemon *daemon=nullptr;
};

#endif // SEAMLESS_SMB_SERVICE_H
