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
#include <QDBusArgument>
#include <QDBusMetaType>
#include "dbus/DeviceManagerAdaptor.h"
#include "keyledsd/DeviceManager.h"

using dbus::DeviceManagerAdaptor;
Q_DECLARE_METATYPE(DBusDeviceKeyInfo)
Q_DECLARE_METATYPE(DBusDeviceKeyInfoList)

/****************************************************************************/

/// DBusDeviceKeyInfo serializer for Qt
QDBusArgument & operator<<(QDBusArgument & arg, const DBusDeviceKeyInfo & key)
{
    arg.beginStructure();
        arg <<key.keyCode;
        arg <<key.name;
        arg.beginStructure();
            arg <<key.position.x0 <<key.position.y0
                <<key.position.x1 <<key.position.y1;
        arg.endStructure();
    arg.endStructure();
    return arg;
}

/// DBusDeviceKeyInfo deserializer for Qt
const QDBusArgument & operator>>(const QDBusArgument & arg, DBusDeviceKeyInfo & key)
{
    arg.beginStructure();
        arg >>key.keyCode;
        arg >>key.name;
        arg.beginStructure();
            arg >>key.position.x0 >>key.position.y0
                >>key.position.x1 >>key.position.y1;
        arg.endStructure();
    arg.endStructure();
    return arg;
}

/****************************************************************************/

DeviceManagerAdaptor::DeviceManagerAdaptor(keyleds::DeviceManager *parent)
    : QDBusAbstractAdaptor(parent)
{
    Q_ASSERT(parent != nullptr);
    qDBusRegisterMetaType<DBusDeviceKeyInfo>();
    qDBusRegisterMetaType<DBusDeviceKeyInfoList>();

    setAutoRelaySignals(true);
}

inline keyleds::DeviceManager * DeviceManagerAdaptor::parent() const
{
    return static_cast<keyleds::DeviceManager *>(QObject::parent());
}

QString DeviceManagerAdaptor::sysPath() const
{
    return parent()->sysPath().c_str();
}

QString DeviceManagerAdaptor::serial() const
{
    return parent()->serial().c_str();
}

QString DeviceManagerAdaptor::devNode() const
{
    return parent()->device().path().c_str();
}

QStringList DeviceManagerAdaptor::eventDevices() const
{
    QStringList result;
    result.reserve(parent()->eventDevices().size());
    std::transform(parent()->eventDevices().cbegin(),
                    parent()->eventDevices().cend(),
                    std::back_inserter(result),
                    [](const auto & path){ return path.c_str(); });
    return result;
}

QString DeviceManagerAdaptor::name() const
{
    return parent()->device().name().c_str();
}

QString DeviceManagerAdaptor::model() const
{
    return parent()->device().model().c_str();
}

QString DeviceManagerAdaptor::firmware() const
{
    return parent()->device().firmware().c_str();
}

DBusDeviceKeyInfoList DeviceManagerAdaptor::keys() const
{
    DBusDeviceKeyInfoList result;
    result.reserve(parent()->keyDB().size());
    std::transform(parent()->keyDB().begin(),
                    parent()->keyDB().end(),
                    std::back_inserter(result),
                    [](const auto & key) { return DBusDeviceKeyInfo{
                        key.keyCode,
                        key.name.c_str(),
                        { key.position.x0, key.position.y0,
                          key.position.x1, key.position.y1 }
                    }; });
    return result;
}

bool DeviceManagerAdaptor::paused() const
{
    return parent()->paused();
}

void DeviceManagerAdaptor::setPaused(bool val)
{
    parent()->setPaused(val);
}
