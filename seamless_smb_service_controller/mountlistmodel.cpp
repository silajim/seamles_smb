#include "mountlistmodel.h"

#include <QRegularExpression>


MountListModel::MountListModel(mlist infolist,QObject *parent) : QAbstractListModel(parent)
{
    modelList = infolist;
}

int MountListModel::rowCount(const QModelIndex &parent) const
{
    return modelList.size();
}

QVariant MountListModel::data(const QModelIndex &index, int role) const
{
    if(index.row() > modelList.size())
        return QVariant();

    MountInfo info = modelList[index.row()];
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
    default:
        return QVariant();
        break;

    }
}

bool MountListModel::insertRows(int row, int count, const QModelIndex &parent)
{
    beginInsertRows(parent,modelList.size(),modelList.size());
    modelList << MountInfo();
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
    if(editor!=nullptr){
        return nullptr;
    }

    editor = new MountEditor(MountInfo(),this);
    connect(editor,&MountEditor::accepted,this,&MountListModel::editaccepted);
    connect(editor,&MountEditor::rejected,this,&MountListModel::editrejected);
    return editor;

}

const mlist &MountListModel::getModelList() const
{
    return modelList;
}

void MountListModel::remove(QUuid id)
{
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

void MountListModel::editaccepted()
{
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
    if(!editor)
        return;
    editor->deleteLater();
    editor = nullptr;
}


