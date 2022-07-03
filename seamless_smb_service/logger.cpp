#include "logger.h"
#include <QMutex>
#include <QMutexLocker>
#include <QtGlobal>
#include <QByteArray>
#include <QTime>
#include <QIODevice>
#include <QFile>

#include <comdef.h>

Logger::Logger(bool debug):DbgPrint(debug,false)
{

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
        f = new QFile("C:/debuglog.txt");
        if (!f->open(QIODevice::WriteOnly | QIODevice::Append)) {
            delete f;
            f = 0;
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
    s.append(c,length);

    free(c);
    _freea(buffer);

    s += '\n';

    f->write(s);
    f->flush();
}
