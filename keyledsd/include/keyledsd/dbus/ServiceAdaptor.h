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
using ServiceContextValues = QMap<QString, QString>;

namespace keyleds {
    class DeviceManager;
    class Service;
}

namespace keyleds { namespace dbus {

/****************************************************************************/

/** Expose keyleds::Service on DBus
 *
 * This DBus adaptor is attached to a keyleds::Service instance at creation
 * time. It automatically connects relevant signals and responds to device
 * creation / removal by creating and destroying dbus::DeviceManagerAdaptor
 * instances.
 */
class ServiceAdaptor final : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "org.etherdream.keyleds.Service")
    Q_PROPERTY(QString configurationPath READ configurationPath)
    Q_PROPERTY(ServiceContextValues context READ context)
    Q_PROPERTY(bool active READ active WRITE setActive)
    Q_PROPERTY(bool autoQuit READ autoQuit WRITE setAutoQuit)
    Q_PROPERTY(QList<QDBusObjectPath> devices READ devicePaths)
    Q_PROPERTY(QStringList plugins READ plugins)
public:
            ServiceAdaptor(Service *parent);

public:         // Simple pass-through methods acessing the Service
    QString configurationPath() const;
    ServiceContextValues context() const;
    bool    active() const;
    void    setActive(bool value);
    bool    autoQuit() const;
    void    setAutoQuit(bool value);
    QList<QDBusObjectPath> devicePaths() const;
    QStringList plugins() const;

public slots:   // Simple pass-through methods acessing the Service
    Q_NOREPLY void    setContextValues(ServiceContextValues);
    Q_NOREPLY void    setContextValue(QString, QString);
    Q_NOREPLY void    sendGenericEvent(ServiceContextValues);
    Q_NOREPLY void    sendKeyEvent(QString serial, int key);  ///< serial is that of target device

private:
    void        onDeviceManagerAdded(DeviceManager &);
    void        onDeviceManagerRemoved(DeviceManager &);

private:
    Service *   parent() const;    ///< instance this adapter is attached to
    QString     managerPath(const DeviceManager &) const;
};

/****************************************************************************/

} } // namespace keyleds::dbus

#endif
