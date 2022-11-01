#include "applicationui.h"
#include <QQmlContext>
#include <QStandardPaths>

#include <QThread>

#include "common.h"


typedef QList<MountInfo> mlist;

ApplicationUI::ApplicationUI(QObject *parent) : QObject(parent)
{
    engine = new QQmlApplicationEngine(this);
    engine->rootContext()->setContextProperty("app",this);
    qmlRegisterInterface<MountListModel>("MountListModel",1);
    qmlRegisterInterface<MountEditor>("MountEditor",1);

    qRegisterMetaType<mlist>("mlist");
//    qRegisterMetaTypeStreamOperators<mlist>("mlist");

    qApp->setOrganizationDomain("Seamless smb");
    qApp->setApplicationName("seamless_smb_service");

    QStringList plist = QStandardPaths::standardLocations(QStandardPaths::AppConfigLocation);
    QString path; //= QStandardPaths::writableLocation(QStandardPaths::AppConfigLocation);
    foreach(QString p, plist ){
        if(p.contains("ProgramData")){
            path = p;
            break;
        }
    }

    settings = new QSettings(path+"/mounts.ini", QSettings::IniFormat,this);

    model = new MountListModel(settings->value("Mounts").value<mlist>(),this);
    connect(model,&MountListModel::edited,this,&ApplicationUI::onModelModified);

    service = new QtServiceController("Seamless smb");

    isRunningTimer = new QTimer(this);
    connect(isRunningTimer,&QTimer::timeout,this,&ApplicationUI::isRunningTimerTimeout);


    setIsInstalled(service->isInstalled());
    isRunningTimerTimeout();

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(engine, &QQmlApplicationEngine::objectCreated,
                     qApp, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine->load(url);

    isRunningTimer->start(1000);
}

ApplicationUI::~ApplicationUI()
{
    delete service;
    if(sock){
        sock->close();
        sock->deleteLater();
        sockThread->quit();
        sockThread->exit();
        sock = nullptr;
        sockThread = nullptr;
    }
}

bool ApplicationUI::getIsRunning() const
{
    return isRunning;
}

void ApplicationUI::isRunningTimerTimeout()
{
    if(!isInstalled)
        return;
    bool run = service->isRunning();
    if(run && !sock){
        qDebug() << "Create socket";
        sockThread = new QThread(this);
        sock = new Client();
        sock->moveToThread(sockThread);
        sockThread->start();
        connect(sock,&Client::status,model,&MountListModel::setRunning);
    }if(!run && sock){
        qDebug() << "delete Socket";
        sock->close();
        sock->deleteLater();
        sockThread->exit();
        sock = nullptr;
        sockThread = nullptr;
    }

    setIsRunning(run);
}

void ApplicationUI::onModelModified()
{
    settings->setValue("Mounts",QVariant::fromValue(model->getModelList()));
    settings->sync();

    if(sock){
        sock->sendReload();
    }
}

bool ApplicationUI::getIsInstalled() const
{
    return isInstalled;
}

void ApplicationUI::installService()
{
    setIsInstalled(QtServiceController::install(QCoreApplication::applicationDirPath()+"\\seamless_smb_service.exe"));
}

void ApplicationUI::uninstallService()
{
    if(isInstalled)
        service->uninstall();
}

void ApplicationUI::start()
{
    service->start();
}

void ApplicationUI::stop()
{
    service->stop();
}

void ApplicationUI::setIsInstalled(bool newIsInstalled)
{
    if (isInstalled == newIsInstalled)
        return;
    isInstalled = newIsInstalled;
    emit isInstalledChanged();
}

MountListModel *ApplicationUI::getModel() const
{
    return model;
}

void ApplicationUI::setIsRunning(bool newIsRunning)
{
    if (isRunning == newIsRunning)
        return;
    isRunning = newIsRunning;
    emit isRunningChanged();
}
//QtServiceController
