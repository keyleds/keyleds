#ifndef KEYLEDSD_SERVICEADAPTOR_H
#define KEYLEDSD_SERVICEADAPTOR_H

#include <QtDBus>
#include <QList>
#include <QString>
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"

namespace keyleds {

class ServiceAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.Service")
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool autoQuit READ autoQuit)
    Q_PROPERTY(QList<QDBusObjectPath> devices READ devicePaths)
public:
            ServiceAdaptor(Service *parent);
    virtual ~ServiceAdaptor();

    inline Service *parent() const
        { return static_cast<Service *>(QObject::parent()); }

public:
    bool    active() const { return parent()->active(); }
    void    setActive(bool value) { parent()->setActive(value); }
    bool    autoQuit() const { return parent()->configuration().autoQuit(); }
    QList<QDBusObjectPath> devicePaths() const;

private slots:
    void    onDeviceManagerAdded(keyleds::DeviceManager &);
    void    onDeviceManagerRemoved(keyleds::DeviceManager &);

private:
    QString managerPath(const keyleds::DeviceManager &) const;
};

};

#endif
