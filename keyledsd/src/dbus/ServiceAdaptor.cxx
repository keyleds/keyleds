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
#include <QtGlobal>
#include <algorithm>
#include <string>
#include "keyledsd/Context.h"
#include "keyledsd/PluginManager.h"
#include "dbus/DeviceManagerAdaptor.h"
#include "dbus/ServiceAdaptor.h"

using dbus::ServiceAdaptor;
Q_DECLARE_METATYPE(ServiceContextValues)


ServiceAdaptor::ServiceAdaptor(keyleds::Service *parent)
    : QDBusAbstractAdaptor(parent)
{
    Q_ASSERT(parent != nullptr);
    qDBusRegisterMetaType<ServiceContextValues>();

    setAutoRelaySignals(true);
    QObject::connect(parent, &keyleds::Service::deviceManagerAdded,
                     this, &ServiceAdaptor::onDeviceManagerAdded);
    QObject::connect(parent, &keyleds::Service::deviceManagerRemoved,
                     this, &ServiceAdaptor::onDeviceManagerRemoved);
}

ServiceContextValues ServiceAdaptor::context() const
{
    ServiceContextValues data;
    for (const auto & entry : parent()->context()) {
        data.insert(entry.first.c_str(), entry.second.c_str());
    }
    return data;
}

void ServiceAdaptor::setContext(const ServiceContextValues & data)
{
    auto context = keyleds::Context::value_map();
    for (auto it = data.constBegin(); it != data.constEnd(); ++it) {
        context.emplace(std::make_pair(std::string(it.key().toUtf8()),
                                       std::string(it.value().toUtf8())));
    }
    parent()->setContext(keyleds::Context(context));
}

QList<QDBusObjectPath> ServiceAdaptor::devicePaths() const
{
    QList<QDBusObjectPath> paths;
    std::transform(parent()->devices().cbegin(),
                   parent()->devices().cend(),
                   std::back_inserter(paths),
                   [this](const auto & entry){
                       return QDBusObjectPath(this->managerPath(*entry.second));
                   });
    return paths;
}

QStringList ServiceAdaptor::plugins() const
{
    const auto & manager = keyleds::EffectPluginManager::instance();

    QStringList plugins;
    plugins.reserve(manager.plugins().size());
    std::transform(manager.plugins().cbegin(),
                   manager.plugins().cend(),
                   std::back_inserter(plugins),
                   [](const auto & item) {
                       return item.second->name().c_str();
                   });
    return plugins;
}

void ServiceAdaptor::onDeviceManagerAdded(keyleds::DeviceManager & manager)
{
    auto connection = QDBusConnection::sessionBus();
    auto path = managerPath(manager);
    new dbus::DeviceManagerAdaptor(&manager);    // manager takes ownership
    if (!connection.registerObject(path, &manager)) {
        qCritical("DBus registration of device %s failed", qPrintable(path));
    }
}

void ServiceAdaptor::onDeviceManagerRemoved(keyleds::DeviceManager &)
{
    return;
}

QString ServiceAdaptor::managerPath(const keyleds::DeviceManager & manager) const
{
    std::string path = "/Device/" + manager.serial();
    return QString(path.c_str());
}
