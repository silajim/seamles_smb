#include "mounteditor.h"
#include <QDebug>

MountEditor::MountEditor(const MountInfo &info, QObject *parent)
{
    m_info = info;

}

void MountEditor::setRootDirectory(const QString &newRootDirectory)
{
    qDebug() << "SET root dir";
    m_info.RootDirectory = newRootDirectory;
    emit changed();
}

void MountEditor::setMountPoint(const QString &newMountPoint)
{
    m_info.MountPoint = newMountPoint;
    emit changed();
}

void MountEditor::setUNCName(const QString &newUNCName)
{
    m_info.UNCName = newUNCName;
    emit changed();
}

void MountEditor::setVolname(const QString &newVolname)
{
    m_info.volname = newVolname;
    emit changed();
}

void MountEditor::setCaseSensitive(bool newCaseSensitive)
{
    m_info.CaseSensitive = newCaseSensitive;
    emit changed();
}

void MountEditor::setImpersonateCallerUser(bool newImpersonateCallerUser)
{
    m_info.ImpersonateCallerUser = newImpersonateCallerUser;
    emit changed();
}

void MountEditor::setEnabled(bool newEnabled)
{
    m_info.enabled = newEnabled;
    emit changed();
}

void MountEditor::setKeepAlive(bool newKeepAlive)
{
    m_info.keepAlive = newKeepAlive;
    emit changed();
}

void MountEditor::setDebug(bool newDebug)
{
    m_info.debug = newDebug;
    emit changed();
}

void MountEditor::setSinglethreaded(bool newSinglethreaded)
{
    m_info.singlethreaded = newSinglethreaded;
    emit changed();
}

void MountEditor::setOptions(QString newOptions)
{
    m_info.options = converter.convertStringToOptions(newOptions);
    emit changed();
}

void MountEditor::isaccepted()
{
    emit accepted();
}

void MountEditor::isrejected()
{
    emit rejected();
}

const MountInfo &MountEditor::info() const
{
    return m_info;
}

const QString MountEditor::RootDirectory() const
{
    return m_info.RootDirectory;
}

const QString MountEditor::MountPoint() const
{
    return m_info.MountPoint;
}

const QString MountEditor::UNCName() const
{
    return m_info.UNCName;
}

const QString MountEditor::Volname() const
{
    return m_info.volname;
}

bool MountEditor::CaseSensitive() const
{
    return m_info.CaseSensitive;
}

bool MountEditor::ImpersonateCallerUser() const
{
    return m_info.ImpersonateCallerUser;
}

//bool MountEditor::HasSeSecurityPrivilege() const
//{
//    return m_info.HasSeSecurityPrivilege;
//}

bool MountEditor::Enabled() const
{
    return m_info.enabled;
}

bool MountEditor::KeepAlive() const
{
    return m_info.keepAlive;
}

bool MountEditor::Debug() const
{
    return m_info.debug;
}

bool MountEditor::Singlethreaded() const
{
    return m_info.singlethreaded;
}

QString MountEditor::Options() const
{
    return converter.convertOptionsToString(m_info.options);
}

QUuid MountEditor::uuid()
{
    return m_info.uuid;
}

