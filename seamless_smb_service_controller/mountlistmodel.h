#ifndef MOUNTLISTMODEL_H
#define MOUNTLISTMODEL_H

#include <QObject>
#include <QAbstractListModel>

#include "common.h"
#include "mounteditor.h"

typedef QList<MountInfo> mlist;

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
        id
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

    const mlist &getModelList() const;

public slots:
    void remove(QUuid id);
    void stop(QUuid id);
    void start(QUuid id);
    MountEditor* edit(QUuid id);
    MountEditor* newMount();

private:
    mlist modelList;
    MountOptionsConverter converter;
    MountEditor *editor=nullptr;

private slots:
    void editaccepted();
    void editrejected();

};


#endif // MOUNTLISTMODEL_H
