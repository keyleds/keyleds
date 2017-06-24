#include <fstream>
#include <iomanip>
#include <sstream>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Layout.h"
#include "keyledsd/PluginManager.h"
#include "config.h"
#include "logging.h"

LOGGING("dev-manager");

using keyleds::Configuration;
using keyleds::Context;
using keyleds::DeviceManager;
using keyleds::Layout;
using keyleds::RenderLoop;

/****************************************************************************/

static std::vector<const Configuration::Profile *> profilesForContext(
    const Configuration & config, const std::string & serial, const Context & context)
{
    auto it = config.devices().find(serial);
    const auto & devLabel = (it == config.devices().end()) ? serial : it->second;

    auto result = std::vector<const Configuration::Profile *>();
    bool hasNonDefault = false;

    for (const auto & profileEntry : config.profiles()) {
        const auto & profile = profileEntry.second;
        const auto & devices = profile.devices();

        if ((devices.empty() ||
                std::find(devices.begin(), devices.end(), devLabel) != devices.end()) &&
            (profile.isDefault() ||
                profile.lookup().match(context))) {
            result.push_back(&profile);
            if (!profile.isDefault()) { hasNonDefault = true; }
        }
    }

    if (hasNonDefault) {
        auto itend = std::remove_if(result.begin(), result.end(),
                                    [](const auto & profile) { return profile->isDefault(); });
        result.erase(itend, result.end());
    }
    return result;
}

/****************************************************************************/

DeviceManager::DeviceManager(device::Description && description, Device && device,
                             const Configuration & conf, const Context & context, QObject *parent)
    : QObject(parent),
      m_configuration(conf),
      m_serial(getSerial(description)),
      m_description(std::move(description)),
      m_eventDevices(findEventDevices(m_description)),
      m_device(std::move(device)),
      m_layout(loadLayout(m_device)),
      m_renderLoop(m_device, loadRenderers(context), 16)
{
    DEBUG("Device(", m_serial, ") enabled ", m_renderLoop.renderers().size(), " renderers");
    m_renderLoop.setPaused(false);
}

DeviceManager::~DeviceManager()
{
    m_renderLoop.stop();
}

void DeviceManager::setContext(const Context & context)
{
    auto lock = m_renderLoop.renderersLock();
    m_renderLoop.renderers() = loadRenderers(context);
    DEBUG("Device(", m_serial, ") enabled ", m_renderLoop.renderers().size(), " renderers");
}

void DeviceManager::setPaused(bool val)
{
    m_renderLoop.setPaused(val);
}

void DeviceManager::reloadConfiguration()
{
    DEBUG("Device(", this, ") clearing renderers cache");
    auto lock = m_renderLoop.renderersLock();
    m_renderLoop.renderers().clear();
    m_profiles.clear();
}

std::string DeviceManager::getSerial(const device::Description & description)
{
    const auto & usbDevDescription = description.parentWithType("usb", "usb_device");
    return usbDevDescription.attributes().at("serial");
}

DeviceManager::dev_list DeviceManager::findEventDevices(const device::Description & description)
{
    dev_list result;
    const auto & usbdev = description.parentWithType("usb", "usb_device");
    const auto & candidates = usbdev.descendantsWithType("input");
    for (const auto & candidate : candidates) {
        const auto & devNode = candidate.devNode();
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
    const auto & fileName = layoutName(device);

    for (const auto & path : paths) {
        std::string fullName = path + '/' + fileName;
        auto file = std::ifstream(fullName);
        if (!file) { continue; }
        try {
            auto result = std::make_unique<Layout>(file);
            INFO("loaded layout ", fullName);
            return result;
        } catch (Layout::ParseError & error) {
            ERROR("layout ", fullName, " line ", error.line(), ": ", error.what());
        } catch (std::exception & error) {
            ERROR("layout ", fullName, ": ", error.what());
        }
    }
    throw std::runtime_error("layout " + fileName + " not found");
}

keyleds::RenderLoop::renderer_list DeviceManager::loadRenderers(const Context & context)
{
    const auto & profiles = profilesForContext(m_configuration, m_serial, context);

    RenderLoop::renderer_list renderers;
    for (const auto & profile : profiles) {
        const auto & loadedProfile = getProfile(*profile);
        const auto & profileRenderers = loadedProfile.renderers();
        std::copy(profileRenderers.begin(),
                  profileRenderers.end(),
                  std::back_inserter(renderers));
    }
    return renderers;
}

DeviceManager::LoadedProfile & DeviceManager::getProfile(const Configuration::Profile & conf)
{
    {
        auto it = m_profiles.find(conf.id());
        if (it != m_profiles.end()) { return it->second; }
    }

    auto & manager = RendererPluginManager::instance();

    LoadedProfile::renderer_list renderers;
    for (const auto & pluginConf : conf.plugins()) {
        auto plugin = manager.get(pluginConf.name());
        if (!plugin) {
            ERROR("plugin ", pluginConf.name(), " not found");
            continue;
        }
        DEBUG("loaded plugin ", pluginConf.name());
        renderers.push_back(plugin->createRenderer(*this, pluginConf));
    }

    auto it = m_profiles.emplace(std::make_pair(conf.id(), LoadedProfile(std::move(renderers)))).first;
    return it->second;
}

keyleds::RenderLoop::renderer_list DeviceManager::LoadedProfile::renderers() const
{
    keyleds::RenderLoop::renderer_list result;
    result.reserve(m_renderers.size());
    std::transform(m_renderers.begin(),
                   m_renderers.end(),
                   std::back_inserter(result),
                   [](auto & ptr) { return ptr.get(); });
    return result;
}
