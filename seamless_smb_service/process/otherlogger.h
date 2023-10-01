#ifndef OTHERLOGGER_H
#define OTHERLOGGER_H

#include "DbgPrint.h"
#include <QFile>

//class OtherLogger
//{
//public:
//  OtherLogger();
//};

class OtherLogger:public DbgPrint
{
public:
  OtherLogger(QString logpath,bool debug);
  ~OtherLogger();

  // DbgPrint interface
public:
  void print(LPCWSTR format,...) override;

private:
  QFile* f = nullptr;
  QString m_logpath = "C:/debuglog.txt";
  unsigned int lines=0;
};

#endif // OTHERLOGGER_H
