#include <QCoreApplication>
#include <iostream>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/Service.h"
#include "config.h"

using keyleds::Service;


Service::Service(Configuration & configuration, QObject * parent)
    : QObject(parent),
      m_configuration(configuration),
      m_deviceWatcher(nullptr, this),
      m_sessionWatcher(this)
{
    m_active = false;
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceAdded(const device::DeviceDescription &)),
                     this, SLOT(onDeviceAdded(const device::DeviceDescription &)));
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceRemoved(const device::DeviceDescription &)),
                     this, SLOT(onDeviceRemoved(const device::DeviceDescription &)));
}

void Service::init()
{
    setActive(true);
}

void Service::setActive(bool active)
{
    m_deviceWatcher.setActive(active);
    m_active = active;
}

void Service::onDeviceAdded(const device::DeviceDescription & description)
{
    try {
        auto device = Device(description.devNode());
        auto manager = std::make_unique<DeviceManager>(
            std::move(device), device::DeviceDescription(description), m_configuration
        );
        emit deviceManagerAdded(*manager);

        std::cout <<"Opened device " <<description.devNode()
                  <<": serial " <<manager->serial()
                  <<", model " <<manager->device().model()
                  <<" firmware " <<manager->device().firmware()
                  <<", <" <<manager->device().name() <<">" <<std::endl;

        m_devices[description.devPath()] = std::move(manager);

    } catch (Device::error & error) {
        std::cerr <<"Not opening device " <<description.devNode() <<": " <<error.what() <<std::endl;
    }
}

void Service::onDeviceRemoved(const device::DeviceDescription & description)
{
    auto it = m_devices.find(description.devPath());
    if (it != m_devices.end()) {
        auto usbDevDescription = description.parentWithType("usb", "usb_device");
        std::cout <<"Removing device " <<usbDevDescription.attributes().at("serial") <<std::endl;

        auto manager = std::move(it->second);
        m_devices.erase(it);

        emit deviceManagerRemoved(*manager);

        if (m_devices.empty() && m_configuration.autoQuit()) {
            QCoreApplication::quit();
        }
    }
}
