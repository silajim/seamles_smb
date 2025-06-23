// #include "seamless_smb_service.h"
// #include "daemon.h"

#include <QCoreApplication>
#include "dokanrunner.h"
#include <QString>

#include <csignal>
#include <qsettings.h>
#include <stdio.h>
#include <iostream>
#include <fstream>

QString uncName;

void myMessageHandler(QtMsgType type, const QMessageLogContext &, const QString &msg)
{
    QString txt;
    switch (type)
    {
        case QtDebugMsg:
            txt = QString("Debug: %1").arg(msg);
            break;
        case QtWarningMsg:
            txt = QString("Warning: %1").arg(msg);
            break;
        case QtCriticalMsg:
            txt = QString("Critical: %1").arg(msg);
            break;
        case QtFatalMsg:
            txt = QString("Fatal: %1").arg(msg);
            abort();
        case QtInfoMsg:
            txt = QString("Info: %1").arg(msg);
            break;
    }
    QFile outFile(QCoreApplication::applicationDirPath() + "/log-" + uncName + ".log");
    if (outFile.open(QIODevice::WriteOnly | QIODevice::Append))
    {
        QTextStream ts(&outFile);
        ts << txt << Qt::endl;
    }
}

// void exitfunc(){
//     qDebug() << "exitfunc";
// }

HWND hwnd = NULL;
ATOM _atom = NULL;

static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
    //    std::cout << "WNDPROC " << uMsg << std::endl;
    qDebug() << "WNDPROC" << uMsg;
    if (uMsg == WM_CLOSE)
    {
        qDebug() << "WNDPROC" << uMsg << "VM_CLOSE";
        QFile f(QCoreApplication::applicationDirPath() + "\\" + uncName + "--BB");
        f.open(QFile::WriteOnly);
        //         qApp->quit();
        qApp->exit(0);
        qApp->processEvents();
        //         std::this_thread::sleep_for(std::chrono::seconds(10));
        return DefWindowProc(hwnd, uMsg, wParam, lParam);
        //         return FALSE;
    }
    return TRUE; // DefWindowProc(hwnd, uMsg, wParam, lParam);
}

int main(int argc, char *argv[])
{
    if (argc < 2)
        return -1;

    // uid = argv[2];
    // uid = uid.remove("{");
    // uid = uid.remove("}");
    // uid = uid.mid(0,10);

    qInstallMessageHandler(myMessageHandler);
    QCoreApplication app(argc, argv);
    app.connect(&app, &QCoreApplication::aboutToQuit, []() {
        qDebug() << "About to quit";
    });
    srand((unsigned)time(NULL));

    QString mountsPath = QString::fromStdString(argv[1]);
    QUuid   id = QUuid::fromString(QString::fromStdString(argv[2]));

    // qDebug() << "ID arg" << id;

    qRegisterMetaType<mlist>("mlist");

    QSettings settings(mountsPath, QSettings::IniFormat);

    settings.sync();
    mlist mountlist;
    mountlist = settings.value("Mounts").value<mlist>();

    // foreach (auto m, mountlist)
    // {
    //     qDebug() << m.RootDirectory << " " << m.enabled << m.UNCName << m.MountPoint;
    //     qDebug() << "Uuid" << m.uuid;
    // }

    // MountInfo info;

    for (MountInfo &_info : mountlist)
    {
        if (_info.uuid == id)
        {
            // qDebug() << "Mount loop match found" << id;
            uncName = _info.RootDirectory;
            break;
        }
    }

    uncName = uncName.replace("\\", "_");
    uncName = uncName.replace("/", "_");

    // uncName = info.UNCName;

    const std::wstring wstr = L"smb process window" + std::to_wstring(rand());

    const wchar_t *CLASS_NAME = wstr.c_str();
    WNDCLASSEX     wc = {};
    wc.cbSize = sizeof(WNDCLASSEX);
    wc.lpfnWndProc = &WindowProc;
    wc.hInstance = GetModuleHandle(NULL);
    wc.lpszClassName = CLASS_NAME;

    _atom = RegisterClassEx(&wc);
    if (_atom == NULL)
    {
        std::wcout << "Register class " << GetLastError() << std::endl;
        qDebug() << "Register class" << GetLastError();
    }

    hwnd = CreateWindowEx(0,                   // Optional window styles.
                          MAKEINTATOM(_atom),  // Window class
                          CLASS_NAME,          // Window text
                          WS_OVERLAPPEDWINDOW, // Window style

                          // Size and position
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,
                          CW_USEDEFAULT,

                          NULL,                  // Parent window
                          NULL,                  // Menu
                          GetModuleHandle(NULL), // Instance handle
                          NULL                   // Additional application data
    );

    SetWindowLongPtr(hwnd, GWLP_USERDATA, NULL);

    if (hwnd == NULL)
    {
        qDebug() << "HWND NULL error:" << GetLastError();
    }
    else
    {
        qDebug() << "HWND OK";
    }

    DokanRunner runner(argc, argv);

    int ret = app.exec();

    qDebug() << "Exec done";

    if (_atom != NULL)
    {
        UnregisterClassW(wc.lpszClassName, wc.hInstance);
    }
    if (hwnd != NULL)
    {
        DestroyWindow(hwnd);
    }

    return ret;
}
