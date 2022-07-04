#include "common.h"
#include "dokan/dokan.h"
#include <QRegularExpression>

//Common::Common()
//{

//}

QDataStream& operator<<(QDataStream &out, const MountInfo &v) {
    out << v.RootDirectory << v.MountPoint << v.UNCName << v.volname << v.CaseSensitive << v.ImpersonateCallerUser << v.HasSeSecurityPrivilege << v.uuid << v.debug << v.cerr << v.singlethreaded << v.enabled << v.keepAlive;

    if(sizeof(unsigned long) == sizeof(quint32)){
        out << (quint32) v.options;
    }else if(sizeof(unsigned long) == sizeof(quint64)){
        out << (quint64) v.options;
    }

    return out;
}

QDataStream& operator>>(QDataStream &in, MountInfo &v) {
    in >> v.RootDirectory >> v.MountPoint >> v.UNCName >> v.volname >> v.CaseSensitive >> v.ImpersonateCallerUser >> v.HasSeSecurityPrivilege >> v.uuid >> v.debug >> v.cerr >> v.singlethreaded >> v.enabled >> v.keepAlive;

    if(sizeof(unsigned long) == sizeof(quint32)){
        quint32 opt;
        in >> opt;
        v.options = opt;
    }else if(sizeof(unsigned long) == sizeof(quint64)){
        quint64 opt;
        in >> opt;
        v.options = opt;
    }


    return in;
}

//QDataStream& MountInfo::operator<<(QDataStream &out, const MountInfo &v) {
//    out << v.RootDirectory << v.MountPoint << v.UNCName << v.volname << v.CaseSensitive << v.ImpersonateCallerUser << v.HasSeSecurityPrivilege << v.uuid << v.debug << v.cerr << v.singlethreaded;

//    if(sizeof(unsigned long) == sizeof(quint32)){
//        out << (quint32) v.options;
//    }else if(sizeof(unsigned long) == sizeof(quint64)){
//        out << (quint32) v.options;
//    }

//    return out;
//}

//QDataStream& MountInfo::operator>>(QDataStream &in, MountInfo &v) {
//    in >> v.RootDirectory >> v.MountPoint >> v.UNCName >> v.volname >> v.CaseSensitive >> v.ImpersonateCallerUser >> v.HasSeSecurityPrivilege >> v.uuid >> v.debug >> v.cerr >> v.singlethreaded;

//    if(sizeof(unsigned long) == sizeof(quint32)){
//        quint32 opt;
//        //        in >>  v.options;
//        in >> opt;
//        opt = v.options;
//    }else if(sizeof(unsigned long) == sizeof(quint64)){
//        quint64 opt;

//        in >> opt;
//        opt = v.options;
//        //        in >> (quint32) v.options;
//    }


//    return in;
//}

//Compares everything expect the UUID

MountInfo::MountInfo()
{
    uuid = QUuid::createUuid();
}

bool MountInfo::operator==(const MountInfo &other)
{
    return RootDirectory==other.RootDirectory && MountPoint == other.MountPoint && UNCName == other.UNCName && volname == other.volname && CaseSensitive == other.CaseSensitive && \
            ImpersonateCallerUser==other.ImpersonateCallerUser && HasSeSecurityPrivilege== other.HasSeSecurityPrivilege  && enabled == other.enabled && \
            keepAlive == other.keepAlive && options==other.options;
}

bool MountInfo::operator!=(const MountInfo &other)
{
    return !(operator==(other));
}


MountOptionsConverter::MountOptionsConverter()
{
    stringmappings["DOKAN_OPTION_DEBUG"] = DOKAN_OPTION_DEBUG;
    stringmappings["DOKAN_OPTION_STDERR"] = DOKAN_OPTION_STDERR;
    stringmappings["DOKAN_OPTION_ALT_STREAM"] = DOKAN_OPTION_ALT_STREAM;
    stringmappings["DOKAN_OPTION_WRITE_PROTECT"] = DOKAN_OPTION_WRITE_PROTECT;
    stringmappings["DOKAN_OPTION_NETWORK"] = DOKAN_OPTION_NETWORK;
    stringmappings["DOKAN_OPTION_REMOVABLE"] = DOKAN_OPTION_REMOVABLE;
    stringmappings["DOKAN_OPTION_MOUNT_MANAGER"] = DOKAN_OPTION_MOUNT_MANAGER;
    stringmappings["DOKAN_OPTION_CURRENT_SESSION"] = DOKAN_OPTION_CURRENT_SESSION;
    stringmappings["DOKAN_OPTION_FILELOCK_USER_MODE"] = DOKAN_OPTION_FILELOCK_USER_MODE;
    stringmappings["DOKAN_OPTION_CASE_SENSITIVE"] = DOKAN_OPTION_CASE_SENSITIVE;
    stringmappings["DOKAN_OPTION_ENABLE_UNMOUNT_NETWORK_DRIVE"] = DOKAN_OPTION_ENABLE_UNMOUNT_NETWORK_DRIVE;
    stringmappings["DOKAN_OPTION_DISPATCH_DRIVER_LOGS"] = DOKAN_OPTION_DISPATCH_DRIVER_LOGS;
}

unsigned long MountOptionsConverter::convertStringToOptions(QString str)
{
    unsigned long opt=0;
    QRegularExpression  reg("[ ,;:\\/|]");
    QStringList strlist =  str.split(reg,Qt::SkipEmptyParts);
    foreach(QString _str,strlist){
        if(stringmappings.contains(_str)){
            opt|=stringmappings.value(_str);
        }
    }
    return opt;
}

QString MountOptionsConverter::convertOptionsToString(unsigned long options) const
{
    QString str;
    foreach(unsigned long opt,stringmappings.values()){
        checkFlag(str,options,opt);
    }

    return str;
}

void MountOptionsConverter::checkFlag(QString &str, unsigned long &options, unsigned long opt) const
{
    QString tmpstr;
    if(options & opt){
        tmpstr = stringmappings.key(opt,"");
        if(!tmpstr.isEmpty()){
            str += tmpstr + " ";
        }
    }
}
