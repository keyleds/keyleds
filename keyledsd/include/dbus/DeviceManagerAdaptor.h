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
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Layout.h"

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
    Q_PROPERTY(QStringList eventDevices READ eventDevices)
    Q_PROPERTY(QString name READ name)
    Q_PROPERTY(QString model READ model)
    Q_PROPERTY(QString firmware READ firmware)
    Q_PROPERTY(QString layoutName READ layoutName)
    Q_PROPERTY(bool paused READ paused WRITE setPaused)
public:
                DeviceManagerAdaptor(keyleds::DeviceManager *parent);

    inline keyleds::DeviceManager *parent() const
        { return static_cast<keyleds::DeviceManager *>(QObject::parent()); }

public:
    QString     serial() const { return parent()->serial().c_str(); }
    QStringList eventDevices() const {
        QStringList result;
        result.reserve(parent()->eventDevices().size());
        std::transform(parent()->eventDevices().cbegin(),
                       parent()->eventDevices().cend(),
                       std::back_inserter(result),
                       [](const auto & path){ return path.c_str(); });
        return result;
    }
    QString     name() const { return parent()->device().name().c_str(); }
    QString     model() const { return parent()->device().model().c_str(); }
    QString     firmware() const { return parent()->device().firmware().c_str(); }
    QString     layoutName() const { return parent()->hasLayout() ?
                                            parent()->layout().name().c_str() : ""; }
    bool        paused() const { return parent()->paused(); }
    void        setPaused(bool val) const { parent()->setPaused(val); }
};

};

#endif
