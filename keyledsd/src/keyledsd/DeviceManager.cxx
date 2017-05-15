#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <assert.h>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Layout.h"
#include "keyledsd/PluginManager.h"
#include "config.h"

using keyleds::Configuration;
using keyleds::DeviceManager;
using keyleds::Layout;
using keyleds::RenderLoop;

DeviceManager::DeviceManager(device::Description && description, Device && device,
                             const Configuration & conf, QObject *parent)
    : QObject(parent),
      m_serial(loadSerial(description)),
      m_description(std::move(description)),
      m_device(std::move(device)),
      m_configuration(conf)
{
    m_layout = loadLayout(m_device);
    m_renderLoop = std::make_unique<RenderLoop>(
        m_device,
        loadRenderers(m_configuration.stackFor(m_serial)),
        20
    );
    m_renderLoop->start();
}

DeviceManager::~DeviceManager()
{
    m_renderLoop->stop();
}

std::string DeviceManager::loadSerial(device::Description & description)
{
    auto usbDevDescription = description.parentWithType("usb", "usb_device");
    return usbDevDescription.attributes().at("serial");
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

keyleds::RenderLoop::renderer_list DeviceManager::loadRenderers(const Configuration::Stack & conf)
{
    const auto & plugins = conf.plugins();
    auto & manager = RendererPluginManager::instance();

    RenderLoop::renderer_list renderers;
    for (auto it = plugins.begin(); it != plugins.end(); ++it) {
        auto plugin = manager.get(it->name());
        if (!plugin) {
            std::cerr <<"Plugin " <<it->name() <<" not found" <<std::endl;
            continue;
        }
        renderers.push_back(plugin->createRenderer(*this, *it));
    }
    return renderers;
}
