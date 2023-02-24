#ifndef LOGGER_H
#define LOGGER_H

#include "DbgPrint.h"
#include <QFile>

class Logger:public DbgPrint
{
public:
    Logger(QString logpath,bool debug);
    ~Logger();

    // DbgPrint interface
public:
    void print(LPCWSTR format,...) override;

private:
    QFile* f = nullptr;
    QString m_logpath = "C:/debuglog.txt";
    unsigned int lines=0;
};

#endif // LOGGER_H
