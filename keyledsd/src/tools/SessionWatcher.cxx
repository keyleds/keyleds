#include <QtDBus>
#include <exception>
#include "tools/SessionWatcher.h"

const QString c_loginService("org.freedesktop.login1");
const QString c_loginPath("/org/freedesktop/login1");
const QString c_loginManagerIface("org.freedesktop.login1.Manager");
const QString c_sessionIface("org.freedesktop.login1.Session");
const QString c_propertiesIface("org.freedesktop.DBus.Properties");


SessionWatcher::SessionWatcher(QObject * parent)
    : QObject(parent),
      m_loginManager(c_loginService, c_loginPath, c_loginManagerIface,
                     QDBusConnection::systemBus())
{
    qDBusRegisterMetaType<QMap<QString, QVariant>>();

    auto reply = QDBusReply<QDBusObjectPath>{m_loginManager.call("GetSession", QString())};
    if (!reply.isValid()) {
        throw std::runtime_error(reply.error().message().toUtf8());
    }
    m_sessionPath = reply.value().path();

    reply = m_loginManager.call("GetSeat", QString());
    if (!reply.isValid()) {
        throw std::runtime_error(reply.error().message().toUtf8());
    }
    m_seatPath = reply.value().path();

    auto bus = QDBusConnection::systemBus();
    bus.connect(c_loginService, m_sessionPath, c_propertiesIface, "PropertiesChanged",
                this, SLOT(sessionPropertiesChanged(const QString&,
                                                    const QMap<QString, QVariant>&)));
    bus.connect(c_loginService, m_sessionPath, c_sessionIface, "Lock",
                this, SLOT(sessionLocked()));
    bus.connect(c_loginService, m_sessionPath, c_sessionIface, "Unlock",
                this, SLOT(sessionUnlocked()));
}

void SessionWatcher::sessionPropertiesChanged(const QString &,
                                              const QMap<QString, QVariant> & changed)
{
    if (changed.contains("Active")) {
        emit activeChanged(changed["Active"].value<bool>());
    }
    if (changed.contains("IdleHint")) {
        emit idleChanged(changed["IdleHint"].value<bool>());
    }
}

void SessionWatcher::sessionLocked()
{
    emit lockChanged(true);
}

void SessionWatcher::sessionUnlocked()
{
    emit lockChanged(false);
}
