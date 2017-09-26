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
#ifndef KEYLEDSD_SERVICEADAPTOR_H_A616A05A
#define KEYLEDSD_SERVICEADAPTOR_H_A616A05A

#include <QDBusAbstractAdaptor>
#include <QDBusObjectPath>
#include <QList>
#include <QMap>
#include <QObject>
#include <QString>

// must be in global namespace for QtDBus to find it
typedef QMap<QString, QString> ServiceContextValues;

namespace keyleds {
    class DeviceManager;
    class Service;
}

namespace dbus {

/** Expose keyleds::Service on DBus
 *
 * This DBus adaptor is attached to a keyleds::Service instance at creation
 * time. It automatically connects relevant signals and responds to device
 * creation / removal by creating and destroying dbus::DeviceManagerAdaptor
 * instances.
 */
class ServiceAdaptor final : public QDBusAbstractAdaptor
{
private:
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.Service")
    Q_PROPERTY(ServiceContextValues context READ context)
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool autoQuit READ autoQuit WRITE setAutoQuit)
    Q_PROPERTY(QList<QDBusObjectPath> devices READ devicePaths)
    Q_PROPERTY(QStringList plugins READ plugins)
public:
            ServiceAdaptor(keyleds::Service *parent);

public:
    ServiceContextValues context() const;
    bool    active() const;
    void    setActive(bool value);
    bool    autoQuit() const;
    void    setAutoQuit(bool value);
    QList<QDBusObjectPath> devicePaths() const;
    QStringList plugins() const;

private slots:
    void    onDeviceManagerAdded(keyleds::DeviceManager &);
    void    onDeviceManagerRemoved(keyleds::DeviceManager &);

private:
    keyleds::Service * parent() const;
    QString managerPath(const keyleds::DeviceManager &) const;
};

};

#endif
