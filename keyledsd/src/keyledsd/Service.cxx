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

void Service::onDeviceAdded(const device::DeviceDescription & description)
{
    try {
        auto usbDevDescription = description.parentWithType("usb", "usb_device");

        // Open device
        auto & device = m_devices.emplace(std::make_pair(
            description.devPath(), Device(description.devNode())
        )).first->second;

        // Load layout description
        std::unique_ptr<keyleds::Layout> layout;
        if (device.hasLayout()) {
            try {
                auto layoutFile = openLayout(device);
                try {
                    layout = std::make_unique<keyleds::Layout>(layoutFile.second);
                } catch (keyleds::Layout::ParseError & error) {
                    std::cerr <<"Error in layout " <<layoutFile.first
                            <<" line " <<error.line()
                            <<": " <<error.what() <<std::endl;
                } catch (std::exception & error) {
                    std::cerr <<"Error in layout " <<layoutFile.first
                            <<": " <<error.what() <<std::endl;
                }
            } catch (std::exception & error) {
                std::cerr <<"Error opening layout for " <<description.devNode() <<": "
                          <<error.what() <<std::endl;
            }
        }

        // Write to the logs
        std::cout <<"Added device " <<description.devNode()
                  <<": serial " <<usbDevDescription.attributes().at("serial")
                  <<", model " <<device.model()
                  <<" firmware " <<device.firmware()
                  <<", <" <<device.name() <<">" <<std::endl;

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
        m_devices.erase(it);
    }
}
