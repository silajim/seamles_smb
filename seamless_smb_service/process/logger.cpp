#include "logger.h"
#include "qfileinfo.h"
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>
#include <QByteArray>
#include <QTime>
#include <QIODevice>
#include <QFile>

#include <QDir>
#include <comdef.h>

Logger::Logger(QString logpath,bool debug):DbgPrint(false,debug), m_logpath(logpath)
{
    qDebug() << Q_FUNC_INFO;

}

Logger::~Logger()
{
    if (!f)
        return;
    f->write(QTime::currentTime().toString("HH:mm:ss.zzz").toLatin1());
    f->write(" --- DEBUG LOG CLOSED ---\n\n");
    f->flush();
    f->close();
    delete f;
    f = nullptr;
}

void Logger::print(LPCWSTR format,...)
{

    if(!m_DebugMode)
        return;
    static QMutex mutex;
    QMutexLocker locker(&mutex);
#if defined(Q_OS_WIN32)
    const qulonglong processId = GetCurrentProcessId();
#else
    const qulonglong processId = getpid();
#endif
    QByteArray s(QTime::currentTime().toString("HH:mm:ss.zzz").toLatin1());
    s += " [";
    s += QByteArray::number(processId);
    s += "] ";

    if (!f) {
        QString name = m_logpath+"_"+QDateTime::currentDateTime().toString("dd.MM-hh.mm.ss")+"_Start.log";
        f = new QFile(name);
        if (!f->open(QIODevice::WriteOnly | QIODevice::Append)) {
            delete f;
            f = nullptr;
            return;
        }
        QByteArray ps('\n' + s + "--- DEBUG LOG OPENED ---\n");
        f->write(ps);
    }

    const WCHAR *outputString;
    WCHAR *buffer = NULL;
    size_t length;
    va_list argp;

    va_start(argp, format);
    length = _vscwprintf(format, argp) + 1;
    buffer = (WCHAR*)_malloca(length * sizeof(WCHAR));
    if (buffer) {
        vswprintf_s(buffer, length, format, argp);
        outputString = buffer;
    } else {
        outputString = format;
    }

//    _bstr_t b(outputString);
//    const char* c = b;

    char * c = (char*)malloc(length*sizeof (char));
    wcstombs_s(NULL,c,length*sizeof (char),outputString,length * sizeof(WCHAR));
    s.append(c,length-1);

    free(c);
    _freea(buffer);

//    if(c[length-1] != '\n')
//        s += '\n';

    f->write(s);
    f->flush();

    ++lines;

    if(lines>=10000){
        QFileInfo inf(m_logpath);
        QDir dir = inf.absoluteDir();
        dir.setNameFilters({inf.fileName()+"*.log"});
        dir.setSorting(QDir::SortFlag::Time);
        auto fileList = dir.entryInfoList();
        f->close();
        delete f;
        if(fileList.size()>30){
//            QString toRemove =
            QFile::remove(fileList.last().absoluteFilePath());
        }

        f = new QFile(m_logpath+"_"+QDateTime::currentDateTime().toString("dd.MM-hh.mm.ss")+".log");
        if (!f->open(QIODevice::WriteOnly | QIODevice::Append)) {
            delete f;
            f = nullptr;
            return;
        }
        lines=0;
    }
}
