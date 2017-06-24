#include <QtGlobal>
#include <algorithm>
#include "dbus/DeviceManagerAdaptor.h"
#include "dbus/ServiceAdaptor.h"
#include "config.h"

using dbus::ServiceAdaptor;
Q_DECLARE_METATYPE(ServiceContextValues)


ServiceAdaptor::ServiceAdaptor(keyleds::Service *parent)
    : QDBusAbstractAdaptor(parent)
{
    Q_ASSERT(parent != nullptr);
    qDBusRegisterMetaType<ServiceContextValues>();

    setAutoRelaySignals(true);
    QObject::connect(parent, SIGNAL(deviceManagerAdded(keyleds::DeviceManager &)),
                     this, SLOT(onDeviceManagerAdded(keyleds::DeviceManager &)));
    QObject::connect(parent, SIGNAL(deviceManagerRemoved(keyleds::DeviceManager &)),
                     this, SLOT(onDeviceManagerRemoved(keyleds::DeviceManager &)));
}

ServiceContextValues ServiceAdaptor::context() const
{
    ServiceContextValues data;
    std::for_each(parent()->context().begin(),
                  parent()->context().end(),
                  [&data](const auto & entry){ data.insert(entry.first.c_str(), entry.second.c_str()); });
    return data;
}

void ServiceAdaptor::setContext(ServiceContextValues data)
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
