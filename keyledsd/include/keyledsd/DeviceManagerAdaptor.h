#ifndef KEYLEDSD_DEVICEMANAGERADAPTOR_H
#define KEYLEDSD_DEVICEMANAGERADAPTOR_H

#include <QtDBus>
#include "keyledsd/DeviceManager.h"

namespace keyleds {

class DeviceManagerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.DeviceManager")
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString model READ model)
    Q_PROPERTY(QString firmware READ firmware)
    Q_PROPERTY(QString serial READ serial)
public:
                DeviceManagerAdaptor(DeviceManager *parent);
    virtual     ~DeviceManagerAdaptor();

    inline DeviceManager *parent() const
        { return static_cast<DeviceManager *>(QObject::parent()); }

public:
    QString     name() const { return parent()->device().name().c_str(); }
    QString     model() const { return parent()->device().model().c_str(); }
    QString     firmware() const { return parent()->device().firmware().c_str(); }
    QString     serial() const;
};

};

#endif
