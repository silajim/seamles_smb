#include "mountlistmodel.h"

#include <QRegularExpression>
#include <QDebug>


MountListModel::MountListModel(mlist infolist,QObject *parent) : QAbstractListModel(parent)
{
    foreach(auto info , infolist){
        modelList << info;
    }

}

int MountListModel::rowCount(const QModelIndex &parent) const
{
//    QMutexLocker lk(&mutex); QMutexLocker lk(&mutex);
    return modelList.size();
}

QVariant MountListModel::data(const QModelIndex &index, int role) const
{
//    QMutexLocker lk(&mutex);
    if(index.row() > modelList.size())
        return QVariant();

    MountInfoExt info = modelList[index.row()];
    switch(role){
    case rootdirectory:
        return info.RootDirectory;
        break;
    case MountPoint:
        return info.MountPoint;
        break;
    case UNCName:
        return info.UNCName;
        break;
    case volname:
        return info.volname;
        break;
    case CaseSensitive:
        return info.CaseSensitive;
        break;
    case ImpersonateCallerUser :
        return info.ImpersonateCallerUser;
        break;
    case enabled:
        return info.enabled;
        break;
    case keepAlive:
        return info.keepAlive;
        break;
    case debug:
        return info.debug;
        break;
    case singlethreaded:
        return info.singlethreaded;
        break;
    case options:
        return converter.convertOptionsToString(info.options);
        break;
    case id:
        return info.uuid;
        break;
    case running:
        return info.running;
        break;
    default:
        return QVariant();
        break;

    }
}

bool MountListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent,modelList.size(),modelList.size());
    modelList << MountInfoExt();
    endInsertRows();
    return true;
}

bool MountListModel::removeRows(int row, int count, const QModelIndex &parent)
{
    if(row>=modelList.size())
        return false;

    beginRemoveRows(parent,row,1);
    modelList.removeAt(row);
    endRemoveRows();

    return true;
}

QHash<int, QByteArray> MountListModel::roleNames() const
{
    QHash<int, QByteArray> names;
    names[rootdirectory] = "Rootdirectory";
    names[MountPoint] = "MountPoint";
    names[UNCName] = "UNCName";
    names[volname] = "volname";
    names[CaseSensitive] = "CaseSensitive";
    names[ImpersonateCallerUser] = "ImpersonateCallerUser";
    names[enabled] = "enabled";
    names[keepAlive] = "keepAlive";
    names[debug] = "debug";
    names[singlethreaded] = "singlethreaded";
    names[options] = "options";
    names[id] = "id";
    names[running] = "running";

    return names;
}

//bool MountListModel::setData(const QModelIndex &index, const QVariant &value, int role)
//{
//    if(index.row() > modelList.size())
//        return false;

//    if(value.isValid())
//        return false;

//    MountInfo info = modelList[index.row()];
//    switch(role){
//    case MountPoint:
//        info.MountPoint = value.toString();
//        break;
//    case UNCName:
//        info.UNCName = value.toString();
//        break;
//    case volname:
//        info.volname = value.toString();
//        break;
//    case CaseSensitive:
//        info.CaseSensitive = value.toBool();
//        break;
//    case ImpersonateCallerUser :
//        info.ImpersonateCallerUser = value.toBool();
//        break;
//    case enabled:
//        info.enabled = value.toBool();
//        break;
//    case keepAlive:
//        info.keepAlive = value.toBool();
//        break;
//    case debug:
//        info.debug = value.toBool();
//        break;
//    case singlethreaded:
//        info.singlethreaded = value.toBool();
//        break;
//    case options:
//        info.options = converter.convertStringToOptions(value.toString());
//        break;
//    default:
//        return false;
//        break;

//    }

//    return true;
//}

MountEditor *MountListModel::edit(QUuid id)
{
     QMutexLocker lk(&mutex);
    if(editor!=nullptr){
        return nullptr;
    }

    foreach(MountInfo info,modelList){
        if(info.uuid == id){
            editor = new MountEditor(info,this);
            connect(editor,&MountEditor::accepted,this,&MountListModel::editaccepted);
            connect(editor,&MountEditor::rejected,this,&MountListModel::editrejected);
            return editor;
        }
    }
    return nullptr;
}

MountEditor *MountListModel::newMount()
{
     QMutexLocker lk(&mutex);
    if(editor!=nullptr){
        return nullptr;
    }

    editor = new MountEditor(MountInfo(),this);
    connect(editor,&MountEditor::accepted,this,&MountListModel::editaccepted);
    connect(editor,&MountEditor::rejected,this,&MountListModel::editrejected);
    return editor;

}

const mlist MountListModel::getModelList() const
{
    mlist list;
    foreach(auto info , modelList){
        list << info;
    }

    return list;
}

void MountListModel::setRunning(QUuid id, bool running)
{
    QMutexLocker lk(&mutex);
//    qDebug() << Q_FUNC_INFO << id.toString() << running;
    for(int i=0;i<modelList.size();i++){
        if(modelList[i].uuid == id){
            if(modelList[i].running != running){
                beginResetModel();
                modelList[i].running = running;
                endResetModel();
            }
        }
    }
}

void MountListModel::remove(QUuid id)
{
//     QMutexLocker lk(&mutex);
    for(int i=0;i<modelList.size();i++){
        if(modelList[i].uuid == id){
            QModelIndex model = QModelIndex();
            beginRemoveRows(model,i,1);
            modelList.removeAt(i);
            endRemoveRows();
            emit edited();
            break;
        }
    }
}

void MountListModel::stop(QUuid id)
{
     QMutexLocker lk(&mutex);
    beginResetModel();
    for(int i=0;i<modelList.size();i++){
        if(modelList[i].uuid == id){
            modelList[i].enabled = false;
            emit edited();
            break;
        }

    }
    endResetModel();

}

void MountListModel::start(QUuid id)
{
     QMutexLocker lk(&mutex);
    beginResetModel();
    for(int i=0;i<modelList.size();i++){
        if(modelList[i].uuid == id){
            modelList[i].enabled = true;
            emit edited();
            break;
        }
    }
    endResetModel();

}

void MountListModel::editaccepted()
{
     QMutexLocker lk(&mutex);
    if(!editor)
        return;

    beginResetModel();
    bool changed=false;
    for(int i=0;i<modelList.size();i++){
        if(modelList[i].uuid == editor->uuid()){
            modelList[i] = editor->info();
            changed = true;
            emit edited();
            break;
        }
    }
    if(!changed){
        modelList << editor->info();
        emit edited();
    }
    endResetModel();
    editor->deleteLater();
    editor = nullptr;
}

void MountListModel::editrejected()
{
     QMutexLocker lk(&mutex);
    if(!editor)
        return;
    editor->deleteLater();
    editor = nullptr;
}


