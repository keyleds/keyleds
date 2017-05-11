#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Layout.h"
#include "keyledsd/Service.h"
#include "config.h"

using keyleds::Configuration;
using keyleds::DeviceManager;
using keyleds::Layout;


DeviceManager::DeviceManager(Device && device, device::Description && description,
                             const Configuration & conf, QObject *parent)
    : QObject(parent),
      m_description(std::move(description)),
      m_device(std::move(device)),
      m_configuration(conf),
      m_loop(20, this)
{
    auto usbDevDescription = m_description.parentWithType("usb", "usb_device");
    m_serial = usbDevDescription.attributes().at("serial");
    m_layout = loadLayout(m_device);
}

DeviceManager::~DeviceManager()
{
}

std::string DeviceManager::layoutName(const Device & device)
{
    std::ostringstream fileNameBuf;
    fileNameBuf.fill('0');
    fileNameBuf <<device.model() <<'_' <<std::hex <<std::setw(4) <<device.layout() <<".xml";
    return fileNameBuf.str();
}

std::unique_ptr<Layout> DeviceManager::loadLayout(const Device & device)
{
    if (!device.hasLayout()) { return nullptr; }

    auto paths = m_configuration.layoutPaths();
    paths.push_back(KEYLEDSD_DATA_PREFIX);
    const auto fileName = layoutName(device);

    for (auto it = paths.begin(); it != paths.end(); ++it) {
        std::string fullName = *it + '/' + fileName;
        std::ifstream file(fullName);
        if (!file) { continue; }
        try {
            return std::make_unique<Layout>(file);
        } catch (Layout::ParseError & error) {
            std::cerr <<"Error in layout " <<fullName
                      <<" line " <<error.line()
                      <<": " <<error.what() <<std::endl;
        } catch (std::exception & error) {
            std::cerr <<"Error in layout " <<fullName
                      <<": " <<error.what() <<std::endl;
        }
    }
    throw std::runtime_error("layout " + fileName + " not found");
}
