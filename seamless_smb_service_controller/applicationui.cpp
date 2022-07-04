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

    qRegisterMetaTypeStreamOperators<mlist>("mlist");

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
    isRunningTimer->start(1000);

    setIsInstalled(service->isInstalled());
    isRunningTimerTimeout();

    const QUrl url(QStringLiteral("qrc:/main.qml"));
    QObject::connect(engine, &QQmlApplicationEngine::objectCreated,
                     qApp, [url](QObject *obj, const QUrl &objUrl) {
        if (!obj && url == objUrl)
            QCoreApplication::exit(-1);
    }, Qt::QueuedConnection);
    engine->load(url);

//     sock = new Socket(this);
}

ApplicationUI::~ApplicationUI()
{
    delete service;
}

bool ApplicationUI::getIsRunning() const
{
    return isRunning;
}

void ApplicationUI::isRunningTimerTimeout()
{
    if(!isInstalled)
        return;
    qDebug() << "isRunningTimerTimeout";
    bool run = service->isRunning();
    if(run && !sock){
        sock = new Socket(this);
    }if(!run && sock){
        sock->deleteLater();
        sock = nullptr;
    }

    setIsRunning(run);
}

void ApplicationUI::onModelModified()
{
    settings->setValue("Mounts",QVariant::fromValue(model->getModelList()));
    settings->sync();
//    QThread::sleep(1);

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
