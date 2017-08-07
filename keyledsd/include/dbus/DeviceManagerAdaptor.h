/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#ifndef KEYLEDSD_DEVICEMANAGERADAPTOR_H_A3B0E2B4
#define KEYLEDSD_DEVICEMANAGERADAPTOR_H_A3B0E2B4

#include <QtDBus>
#include <QList>
#include <QString>
#include "keyledsd/DeviceManager.h"

struct DBusDeviceKeyInfo
{
    struct Rect { uint x0, y0, x1, y1; };

    ushort      blockId;
    uint8_t     keyId;
    QString     name;
    Rect        position;
};
typedef QList<DBusDeviceKeyInfo> DBusDeviceKeyInfoList;

namespace dbus {

/** Expose keyleds::DeviceManager on DBus
 *
 * This DBus adaptor is attached to a keyleds::DeviceManager instance at creation
 * time. It is typically created by a ServiceAdaptor.
 */
class DeviceManagerAdaptor: public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.DeviceManager")
    Q_PROPERTY(QString serial READ serial)
    Q_PROPERTY(QString devNode READ devNode)
    Q_PROPERTY(QStringList eventDevices READ eventDevices)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString model READ model)
    Q_PROPERTY(QString firmware READ firmware)
    Q_PROPERTY(DBusDeviceKeyInfoList keys READ keys)
    Q_PROPERTY(bool paused READ paused WRITE setPaused)
public:
                DeviceManagerAdaptor(keyleds::DeviceManager *parent);

    inline keyleds::DeviceManager *parent() const
        { return static_cast<keyleds::DeviceManager *>(QObject::parent()); }

public:
    QString     serial() const;
    QString     devNode() const;
    QStringList eventDevices() const;
    QString     name() const;
    QString     model() const;
    QString     firmware() const;
    DBusDeviceKeyInfoList keys() const;
    bool        paused() const { return parent()->paused(); }
    void        setPaused(bool val) const { parent()->setPaused(val); }
};

};

#endif
