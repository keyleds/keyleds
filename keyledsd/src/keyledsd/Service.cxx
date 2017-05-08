#include <iomanip>
#include <iostream>
#include <memory>
#include <fstream>
#include <sstream>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/Layout.h"
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

std::pair<std::string, std::ifstream> Service::openLayout(const Device & device) const
{
    auto paths = m_configuration.layoutPaths();
    paths.push_back(KEYLEDSD_DATA_PREFIX);

    std::ostringstream fileNameBuf;
    fileNameBuf.fill('0');
    fileNameBuf <<device.model() <<'_' <<std::hex <<std::setw(4) <<device.layout() <<".xml";
    auto fileName = fileNameBuf.str();

    for (auto it = paths.begin(); it != paths.end(); ++it) {
        std::string fullName = *it + '/' + fileName;
        std::ifstream file(fullName);
        if (file) {
            return std::make_pair(fullName, std::move(file));
        }
    }
    throw std::runtime_error("layout " + fileName + " not found");
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
