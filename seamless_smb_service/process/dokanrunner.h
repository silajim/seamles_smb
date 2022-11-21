#ifndef DOKANRUNNER_H
#define DOKANRUNNER_H

#include <QObject>
#include "common.h"
#include "filemount.h"
#include "globals.h"
#include "logger.h"
#include <QTimer>

typedef QList<MountInfo> mlist;

class DokanRunner : public QObject
{
    Q_OBJECT
public:
    DokanRunner(int argc, char *argv[]);
    ~DokanRunner();
    void unmount();
private:
    void MountInfoToGlobal(MountInfo info, std::shared_ptr<Globals> &g, DOKAN_OPTIONS &dokanOptions);
    std::shared_ptr<FileMount> filemount = nullptr;
    QTimer saveSecurityTimer;

private slots:
    void onSaveSecurity();
};

#endif // DOKANRUNNER_H
