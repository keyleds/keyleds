#include <iostream>
#include "keyledsd/DeviceManagerAdaptor.h"
#include "keyledsd/ServiceAdaptor.h"
#include "config.h"

using keyleds::ServiceAdaptor;

ServiceAdaptor::ServiceAdaptor(keyleds::Service *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
    QObject::connect(parent, SIGNAL(deviceManagerAdded(keyleds::DeviceManager &)),
                     this, SLOT(onDeviceManagerAdded(keyleds::DeviceManager &)));
    QObject::connect(parent, SIGNAL(deviceManagerRemoved(keyleds::DeviceManager &)),
                     this, SLOT(onDeviceManagerRemoved(keyleds::DeviceManager &)));
}

ServiceAdaptor::~ServiceAdaptor()
{
    // destructor
}

QList<QDBusObjectPath> ServiceAdaptor::devicePaths() const
{
    const auto & devices = parent()->devices();
    QList<QDBusObjectPath> paths;
    for (auto it = devices.cbegin(); it != devices.cend(); ++it) {
        paths.push_back(QDBusObjectPath(managerPath(*it->second)));
    }
    return paths;
}

void ServiceAdaptor::onDeviceManagerAdded(keyleds::DeviceManager & manager)
{
    auto connection = QDBusConnection::systemBus();
    new keyleds::DeviceManagerAdaptor(&manager);
    if (!connection.registerObject(managerPath(manager), &manager)) {
        std::cerr <<"DBus registration of device failed" <<std::endl;
    }
}

void ServiceAdaptor::onDeviceManagerRemoved(keyleds::DeviceManager &)
{
}

QString ServiceAdaptor::managerPath(const keyleds::DeviceManager & manager) const
{
    std::string path = "/Device/" + manager.serial();
    return QString(path.c_str());
}
