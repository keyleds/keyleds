#ifndef KEYLEDSD_SERVICEADAPTOR_H
#define KEYLEDSD_SERVICEADAPTOR_H

#include <QtDBus>
#include <QList>
#include <QMap>
#include <QString>
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"

typedef QMap<QString, QString> ServiceContextValues;

namespace dbus {

class ServiceAdaptor: public QDBusAbstractAdaptor
{
public:
private:
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.Service")
    Q_PROPERTY(ServiceContextValues context READ context WRITE setContext)
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool autoQuit READ autoQuit WRITE setAutoQuit)
    Q_PROPERTY(QList<QDBusObjectPath> devices READ devicePaths)
public:
            ServiceAdaptor(keyleds::Service *parent);

    inline keyleds::Service *parent() const
        { return static_cast<keyleds::Service *>(QObject::parent()); }

public:
    ServiceContextValues context() const;
    void    setContext(ServiceContextValues);
    bool    active() const { return parent()->active(); }
    void    setActive(bool value) { parent()->setActive(value); }
    bool    autoQuit() const { return parent()->configuration().autoQuit(); }
    void    setAutoQuit(bool value) { parent()->configuration().setAutoQuit(value); }
    QList<QDBusObjectPath> devicePaths() const;

private slots:
    void    onDeviceManagerAdded(keyleds::DeviceManager &);
    void    onDeviceManagerRemoved(keyleds::DeviceManager &);

private:
    QString managerPath(const keyleds::DeviceManager &) const;
};

};

#endif
