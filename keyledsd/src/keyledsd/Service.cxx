#include <iostream>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/Service.h"

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

void Service::onDeviceAdded(const device::DeviceDescription & device)
{
    try {
        m_devices.emplace(std::make_pair(device.devPath(), Device(device.devNode())));
        std::cout <<"Device added: " <<device.devPath() <<std::endl;
    } catch (Device::error & error) {
        std::cerr <<"Not opening device: " <<error.what() <<std::endl;
    }
}

void Service::onDeviceRemoved(const device::DeviceDescription & device)
{
    auto it = m_devices.find(device.devPath());
    if (it != m_devices.end()) {
        m_devices.erase(it);
        std::cout <<"Device removed: " <<device.devPath() <<std::endl;
    }
}
