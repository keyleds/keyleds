#include <fstream>
#include <iomanip>
#include <sstream>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Layout.h"
#include "keyledsd/PluginManager.h"
#include "tools/Paths.h"
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

DeviceManager::DeviceManager(const device::Description & description, Device && device,
                             const Configuration & conf, const Context & context, QObject *parent)
    : QObject(parent),
      m_configuration(conf),
      m_serial(getSerial(description)),
      m_eventDevices(findEventDevices(description)),
      m_device(std::move(device)),
      m_layout(loadLayout(conf, m_device)),
      m_renderLoop(m_device, loadRenderers(context), 16)
{
    m_renderLoop.setPaused(false);
}

DeviceManager::~DeviceManager()
{
    m_renderLoop.stop();
}

keyleds::Device::key_indices DeviceManager::resolveKeyName(const std::string & name) const
{
    if (m_layout == nullptr) {
        return m_device.resolveKey(name);
    }
    auto kit = std::find_if(m_layout->keys().begin(), m_layout->keys().end(),
                            [&name](const auto & key) { return key.name == name; });
    if (kit == m_layout->keys().end()) { return Device::key_npos; }
    return m_device.resolveKey(kit->block, kit->code);
}

void DeviceManager::setContext(const Context & context)
{
    m_renderLoop.setRenderers(loadRenderers(context));
}

void DeviceManager::setPaused(bool val)
{
    m_renderLoop.setPaused(val);
}

void DeviceManager::reloadConfiguration()
{
    DEBUG("Device(", this, ") clearing renderers cache");
    m_renderLoop.setRenderers({});
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

std::unique_ptr<Layout> DeviceManager::loadLayout(const Configuration & conf, const Device & device)
{
    if (!device.hasLayout()) { return nullptr; }

    auto paths = conf.layoutPaths();
    for (const auto & path : paths::getPaths(paths::XDG::Data, true)) {
        paths.push_back(path + "/" KEYLEDSD_DATA_PREFIX "/layouts");
    }
    const auto & fileName = layoutName(device);

    for (const auto & path : paths) {
        std::string fullName = path + '/' + fileName;
        auto file = std::ifstream(fullName);
        if (!file) { continue; }
        try {
            auto result = std::make_unique<Layout>(Layout::parse(file));
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

    // Load groups
    keyleds::IRendererPlugin::group_map groups;
    for (const auto & group : conf.groups()) {
        auto keys = decltype(groups)::mapped_type();
        for (const auto & name : group.second) {
            auto indices = resolveKeyName(name);
            if (indices != Device::key_npos) {
                keys.push_back(indices);
            } else {
                WARNING("unknown key ", name, " for device ", m_serial, " in profile ", conf.name());
            }
        }
        groups.emplace(group.first, std::move(keys));
    }
    for (const auto & group : m_configuration.groups()) {
        if (groups.find(group.first) != groups.end()) { continue; }
        auto keys = decltype(groups)::mapped_type();
        for (const auto & name : group.second) {
            auto indices = resolveKeyName(name);
            if (indices != Device::key_npos) {
                keys.push_back(indices);
            } else {
                DEBUG("ignoring global key ", name, ": not found on device ", m_serial);
            }
        }
        groups.emplace(group.first, std::move(keys));
    }

    // Load renderers
    auto & manager = RendererPluginManager::instance();
    LoadedProfile::renderer_list renderers;
    for (const auto & pluginConf : conf.plugins()) {
        auto plugin = manager.get(pluginConf.name());
        if (!plugin) {
            ERROR("plugin ", pluginConf.name(), " not found");
            continue;
        }
        DEBUG("loaded plugin ", pluginConf.name());
        renderers.push_back(plugin->createRenderer(*this, pluginConf, groups));
    }

    auto it = m_profiles.emplace(conf.id(), LoadedProfile(std::move(renderers))).first;
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
