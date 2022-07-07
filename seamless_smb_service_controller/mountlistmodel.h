#ifndef MOUNTLISTMODEL_H
#define MOUNTLISTMODEL_H

#include <QObject>
#include <QAbstractListModel>

#include "common.h"
#include "mounteditor.h"

#include <QMutex>
#include <QMutexLocker>

typedef QList<MountInfo> mlist;


struct MountInfoExt:public MountInfo{
    MountInfoExt() = default;
    MountInfoExt(MountInfo &info):MountInfo(info){

    };
     MountInfoExt(const MountInfo &info):MountInfo(info){

     };

    bool operator==(const MountInfoExt &other){
        return MountInfo::operator==(other);
    }
    bool operator!=(const MountInfoExt &other){
        return MountInfo::operator!=(other);
    }
    bool running = false;
};

typedef QList<MountInfoExt> mlistExt;

class MountListModel : public QAbstractListModel
{
    Q_OBJECT
    Q_ENUMS(MyRoles)
public:
    explicit MountListModel(mlist infolist,QObject *parent = nullptr);



    enum MyRoles {
        rootdirectory = Qt::UserRole + 1,
        MountPoint,
        UNCName,
        volname,
        CaseSensitive,
        ImpersonateCallerUser,
        enabled,
        keepAlive,
        debug,
        singlethreaded,
        options,
        id,
        running
    };


signals:
    void edited();

    // QAbstractItemModel interface
public:
    int rowCount(const QModelIndex &parent) const override;
    QVariant data(const QModelIndex &index, int role) const override;
    bool insertRows(int row, int count, const QModelIndex &parent) override;
    bool removeRows(int row, int count, const QModelIndex &parent) override;
    QHash<int, QByteArray> roleNames() const override;

//    bool setData(const QModelIndex &index, const QVariant &value, int role) override;

    const mlist getModelList() const;

public slots:
    void remove(QUuid id);
    void stop(QUuid id);
    void start(QUuid id);
    MountEditor* edit(QUuid id);
    MountEditor* newMount();

    void setRunning(QUuid id, bool running);

private:
    mlistExt modelList;
    MountOptionsConverter converter;
    MountEditor *editor=nullptr;

    QMutex mutex;

private slots:
    void editaccepted();
    void editrejected();

};


#endif // MOUNTLISTMODEL_H
