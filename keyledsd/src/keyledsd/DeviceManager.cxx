#include <fstream>
#include <iomanip>
#include <iostream>
#include <sstream>
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
      m_serial(getSerial(description)),
      m_description(std::move(description)),
      m_eventDevices(findEventDevices(m_description)),
      m_device(std::move(device)),
      m_configuration(conf)
{
    m_layout = loadLayout(m_device);
    m_renderLoop = std::make_unique<RenderLoop>(
        m_device,
        loadRenderers(m_configuration.stackFor(m_serial)),
        16
    );
    QObject::connect(m_renderLoop.get(), SIGNAL(finished()), this, SLOT(renderLoopFinished()));
    m_renderLoop->start();
}

DeviceManager::~DeviceManager()
{
    m_renderLoop->stop();
    m_renderLoop.reset();
}

std::string DeviceManager::getSerial(const device::Description & description)
{
    const auto usbDevDescription = description.parentWithType("usb", "usb_device");
    return usbDevDescription.attributes().at("serial");
}

DeviceManager::dev_list DeviceManager::findEventDevices(const device::Description & description)
{
    dev_list result;
    const auto usbdev = description.parentWithType("usb", "usb_device");
    const auto candidates = usbdev.descendantsWithType("input");
    for (const auto & candidate : candidates) {
        const auto devNode = candidate.devNode();
        if (!devNode.empty()) {
            result.push_back(devNode);
        }
    }
    return result;
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
    paths.push_back(KEYLEDSD_DATA_PREFIX "/layouts");
    const auto fileName = layoutName(device);

    for (const auto & path : paths) {
        std::string fullName = path + '/' + fileName;
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

keyleds::RenderLoop::renderer_list DeviceManager::loadRenderers(const Configuration::Stack & stack)
{
    auto & manager = RendererPluginManager::instance();

    RenderLoop::renderer_list renderers;
    for (const auto & pluginConf : stack.plugins()) {
        auto plugin = manager.get(pluginConf.name());
        if (!plugin) {
            std::cerr <<"Plugin " <<pluginConf.name() <<" not found" <<std::endl;
            continue;
        }
        renderers.push_back(plugin->createRenderer(*this, pluginConf));
    }
    return renderers;
}

void DeviceManager::renderLoopFinished()
{
    emit stopped();
}
