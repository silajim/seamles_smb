#ifndef APPLICATIONUI_H
#define APPLICATIONUI_H

#include <QObject>
#include <QQmlApplicationEngine>
#include <QGuiApplication>
#include <QTimer>
#include <QSettings>

#include "QtService"

#include "mountlistmodel.h"
#include "mounteditor.h"

#include "socket.h"

class ApplicationUI : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool isRunning READ getIsRunning NOTIFY isRunningChanged)
    Q_PROPERTY(bool isInstalled READ getIsInstalled NOTIFY isInstalledChanged)
    Q_PROPERTY(MountListModel* model READ getModel)
public:
    explicit ApplicationUI(QObject *parent = nullptr);
    ~ApplicationUI();

    bool getIsRunning() const;
    bool getIsInstalled() const;


public slots:
    MountListModel *getModel() const;
    void installService();
    void uninstallService();
    void start();
    void stop();

signals:

    void isRunningChanged();

    void isInstalledChanged();

private slots:
    void isRunningTimerTimeout();
    void onModelModified();

private:
    QQmlApplicationEngine *engine=nullptr;
    bool isRunning = false;
    bool isInstalled = false;

    void setIsRunning(bool newIsRunning);
    void setIsInstalled(bool newIsInstalled);

    QtServiceController *service=nullptr;
    QTimer *isRunningTimer=nullptr;

    QSettings *settings=nullptr;

    MountListModel *model = nullptr;

    Socket *sock=nullptr;
    QThread *sockThread = nullptr;


};

#endif // APPLICATIONUI_H
