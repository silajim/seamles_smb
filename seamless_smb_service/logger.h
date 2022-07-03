#ifndef LOGGER_H
#define LOGGER_H

#include "DbgPrint.h"
#include <QFile>

class Logger:public DbgPrint
{
public:
    Logger(bool debug);
    ~Logger();

    // DbgPrint interface
public:
    void print(LPCWSTR format,...) override;

private:
    QFile* f = nullptr;
};

#endif // LOGGER_H
