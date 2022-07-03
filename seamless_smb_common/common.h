#ifndef COMMON_H
#define COMMON_H

#include <QString>
#include <QMetaType>
#include <QDataStream>
#include <QUuid>
#include <QMap>
//#include "dokan/dokan.h"

//class Common
//{
//public:
//    Common();
//};

struct MountInfo{

    MountInfo();

    QString RootDirectory;
    QString MountPoint ;
    QString UNCName;
    QString volname;
    bool CaseSensitive = false;
    bool ImpersonateCallerUser = false;
    bool HasSeSecurityPrivilege = false;

    QUuid uuid;
    bool enabled = false;
    bool keepAlive = false;

    bool debug=false;
    bool cerr = false;
    bool singlethreaded=false;

    unsigned long options=0;

    //Compares everything expect the UUID
    bool operator==(const MountInfo &other);
    bool operator!=(const MountInfo &other);

//    friend QDataStream& operator<<(QDataStream& out, const MountInfo& v);

    //    friend QDataStream& operator>>(QDataStream& in, MountInfo& v);


};

QDataStream& operator<<(QDataStream& out, const MountInfo& v);

QDataStream& operator>>(QDataStream& in, MountInfo& v);

class MountOptionsConverter{
public:
    MountOptionsConverter();
    unsigned long convertStringToOptions(QString str);
    QString convertOptionsToString(unsigned long options) const;
    void checkFlag(QString &str,unsigned long &options,unsigned long opt) const ;

private:
    QMap<QString, unsigned long> stringmappings;

};


Q_DECLARE_METATYPE(MountInfo);
#endif // COMMON_H
