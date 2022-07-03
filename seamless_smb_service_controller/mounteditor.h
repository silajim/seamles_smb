#ifndef MOUNTEDITOR_H
#define MOUNTEDITOR_H

#include <QObject>
#include "common.h"

class MountEditor : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString RootDirectory READ RootDirectory WRITE setRootDirectory NOTIFY changed)
    Q_PROPERTY(QString MountPoint READ MountPoint WRITE setMountPoint NOTIFY changed)
    Q_PROPERTY(QString UNCName READ UNCName WRITE setUNCName NOTIFY changed)
    Q_PROPERTY(QString Volname READ Volname WRITE setVolname NOTIFY changed)
    Q_PROPERTY(bool CaseSenitive READ CaseSensitive WRITE setCaseSensitive NOTIFY changed)
    Q_PROPERTY(bool ImpersonateCallerUser READ ImpersonateCallerUser WRITE setImpersonateCallerUser NOTIFY changed)
    Q_PROPERTY(bool Enabled READ Enabled WRITE setEnabled NOTIFY changed)
    Q_PROPERTY(bool KeepAlive READ KeepAlive WRITE setKeepAlive NOTIFY changed)
    Q_PROPERTY(bool Debug READ Debug WRITE setDebug NOTIFY changed)
    Q_PROPERTY(bool Singlethreaded READ Singlethreaded WRITE setSinglethreaded NOTIFY changed)

    Q_PROPERTY(QString Options READ Options WRITE setOptions NOTIFY changed)


public:    
    explicit MountEditor(const MountInfo &info, QObject *parent = nullptr);

    const MountInfo &info() const;

    const QString RootDirectory() const;
    const QString MountPoint() const;
    const QString UNCName() const;
    const QString Volname() const;
    bool CaseSensitive() const;
    bool ImpersonateCallerUser() const;
    bool Enabled() const;
    bool KeepAlive() const;
    bool Debug() const;
    bool Singlethreaded() const;
    QString Options() const;

    QUuid uuid();

    void setRootDirectory(const QString &newRootDirectory);
    void setMountPoint(const QString &newMountPoint);
    void setUNCName(const QString &newUNCName);
    void setVolname(const QString &newVolname);
    void setCaseSensitive(bool newCaseSensitive);
    void setImpersonateCallerUser(bool newImpersonateCallerUser);
//    void setHasSeSecurityPrivilege(bool newHasSeSecurityPrivilege);
    void setEnabled(bool newEnabled);
    void setKeepAlive(bool newKeepAlive);
    void setDebug(bool newDebug);
    void setSinglethreaded(bool newSinglethreaded);
    void setOptions(QString newOptions);

signals:
    void changed();
    void accepted();
    void rejected();

public slots:

    void isaccepted();
    void isrejected();

private:
    MountInfo m_info;
    MountOptionsConverter converter;
};

#endif // MOUNTEDITOR_H
