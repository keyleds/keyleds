/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include "keyledsd/device/DeviceManager.h"

#include <unistd.h>
#include <algorithm>
#include <cassert>
#include <fstream>
#include <iomanip>
#include <sstream>
#include "keyledsd/device/LayoutDescription.h"
#include "keyledsd/PluginManager.h"
#include "tools/Paths.h"
#include "config.h"
#include "logging.h"

LOGGING("dev-manager");

using keyleds::Configuration;
using keyleds::DeviceManager;
using keyleds::KeyDatabase;
using keyleds::LayoutDescription;
using keyleds::RenderLoop;

static constexpr char defaultProfileName[] = "__default__";
static constexpr char overlayProfileName[] = "__overlay__";

/****************************************************************************/

DeviceManager::LoadedEffect::LoadedEffect(std::string name, plugin_list && plugins)
 : m_name(std::move(name)),
   m_plugins(std::move(plugins))
{}

/****************************************************************************/

DeviceManager::DeviceManager(FileWatcher & fileWatcher,
                             const device::Description & description, Device && device,
                             const Configuration * conf, QObject *parent)
    : QObject(parent),
      m_configuration(nullptr),
      m_sysPath(description.sysPath()),
      m_serial(getSerial(description)),
      m_eventDevices(findEventDevices(description)),
      m_device(std::move(device)),
      m_fileWatcherSub(fileWatcher.subscribe(description.devNode(), FileWatcher::event::Attrib,
                                             std::bind(&DeviceManager::handleFileEvent, this,
                                                       std::placeholders::_1, std::placeholders::_2,
                                                       std::placeholders::_3))),
      m_keyDB(buildKeyDB(*conf, m_device)),
      m_renderLoop(m_device, KEYLEDSD_RENDER_FPS)
{
    setConfiguration(conf);
    m_renderLoop.start();
}

DeviceManager::~DeviceManager()
{
    m_renderLoop.stop();            // destroying the loop is UB if the thread is still running
}

void DeviceManager::setConfiguration(const Configuration * conf)
{
    assert(conf != nullptr);
    auto lock = m_renderLoop.lock();

    m_renderLoop.effects().clear();
    m_effects.clear();

    m_configuration = conf;
    m_name = getName(*conf, m_serial);
}


void DeviceManager::setContext(const string_map & context)
{
    auto effects = loadEffects(context);
    DEBUG("enabling ", effects.size(), " effects for loop ", &m_renderLoop);

    // Notify newly-active plugins of context change
    auto lock = m_renderLoop.lock();
    for (auto * effect : effects) { effect->handleContextChange(context); }
    m_renderLoop.effects() = std::move(effects);
}

void DeviceManager::handleFileEvent(FileWatcher::event, uint32_t, std::string)
{
    int result = access(m_device.path().c_str(), R_OK | W_OK);
    setPaused(result != 0);
}

void DeviceManager::handleGenericEvent(const string_map & context)
{
    auto lock = m_renderLoop.lock();
    for (auto * effect : m_renderLoop.effects()) { effect->handleGenericEvent(context); }
}

void DeviceManager::handleKeyEvent(int keyCode, bool press)
{
    // Convert raw key code into a reference to its database entry
    auto it = m_keyDB.findKeyCode(keyCode);
    if (it == m_keyDB.end()) {
        DEBUG("unknown key ", keyCode, " on device ", m_serial);
        return;
    }

    // Pass event to active effect plugins
    auto lock = m_renderLoop.lock();
    for (const auto & plugin : m_renderLoop.effects()) {
        plugin->handleKeyEvent(*it, press);
    }
    DEBUG("key ", it->name, " ", press ? "pressed" : "released", " on device ", m_serial);
}

void DeviceManager::setPaused(bool val)
{
    m_renderLoop.setPaused(val);
}

std::string DeviceManager::getSerial(const device::Description & description)
{
    // Serial is stored on master USB device, so we walk up the hierarchy
    const auto & usbDevDescription = description.parentWithType("usb", "usb_device");
    auto it = std::find_if(
        usbDevDescription.attributes().begin(), usbDevDescription.attributes().end(),
        [](const auto & attr) { return attr.first == "serial"; }
    );
    if (it == usbDevDescription.attributes().end()) {
        throw std::runtime_error("Device " + description.sysPath() + " has no serial");
    }
    return it->second;
}

std::string DeviceManager::getName(const Configuration & config, const std::string & serial)
{
    auto dit = std::find_if(config.devices().begin(), config.devices().end(),
                            [&serial](auto & item) { return item.second == serial; });
    return dit != config.devices().end() ? dit->first : serial;
}

DeviceManager::dev_list DeviceManager::findEventDevices(const device::Description & description)
{
    // Event devices are any device detected as input devices and attached
    // to same USB device as ours
    dev_list result;
    const auto & usbdev = description.parentWithType("usb", "usb_device");
    const auto & candidates = usbdev.descendantsWithType("input");
    for (const auto & candidate : candidates) {
        const auto & devNode = candidate.devNode();
        if (!devNode.empty()) {
            result.emplace_back(devNode);
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

LayoutDescription DeviceManager::loadLayoutDescription(const Configuration & conf, const Device & device)
{
    using tools::paths::XDG;
    if (!device.hasLayout()) { return LayoutDescription(std::string(), {}); }

    auto paths = conf.layoutPaths();
    for (const auto & path : tools::paths::getPaths(XDG::Data, true)) {
        paths.emplace_back(path + "/" KEYLEDSD_DATA_PREFIX "/layouts");
    }
    const auto & fileName = layoutName(device);

    for (const auto & path : paths) {
        std::string fullName = path + '/' + fileName;
        std::ifstream file(fullName);
        if (!file) { continue; }
        try {
            auto result = LayoutDescription::parse(file);
            INFO("loaded layout ", fullName);
            return result;
        } catch (LayoutDescription::ParseError & error) {
            ERROR("layout ", fullName, " line ", error.line(), ": ", error.what());
        } catch (std::exception & error) {
            ERROR("layout ", fullName, ": ", error.what());
        }
    }
    return LayoutDescription(std::string(), {});
}

KeyDatabase DeviceManager::buildKeyDB(const Configuration & conf, const Device & device)
{
    auto layout = loadLayoutDescription(conf, device);

    KeyDatabase::key_list db;
    RenderTarget::size_type keyIndex = 0;

    for (const auto & block : device.blocks()) {
        for (Device::key_list::size_type kidx = 0; kidx < block.keys().size(); ++kidx) {
            const auto keyId = block.keys()[kidx];
            std::string name;
            auto position = KeyDatabase::Key::Rect{0, 0, 0, 0};

            for (const auto & key : layout.keys()) {
                if (key.block == block.id() && key.code == keyId) {
                    name = key.name;
                    position = {key.position.x0, key.position.y0,
                                key.position.x1, key.position.y1};
                    break;
                }
            }
            if (name.empty()) { name = device.resolveKey(block.id(), keyId); }

            db.emplace_back(KeyDatabase::Key{
                keyIndex,
                device.decodeKeyId(block.id(), keyId),
                name,
                position
            });
            ++keyIndex;
        }
    }
    return db;
}

/// Applies the configuration to a string_map, matching profiles and resolving
/// effect names. Returns the list of Effect entries in the configuration that
/// should be loaded for the context. Returned list references Configuration
/// entries directly, and are therefore invalidated by any operation that
/// invalidates configuration's iterators.
keyleds::RenderLoop::effect_plugin_list DeviceManager::loadEffects(const string_map & context)
{
    // Match context against profile lookups
    const Configuration::Profile * profile = nullptr;
    const Configuration::Profile * defaultProfile = nullptr;
    const Configuration::Profile * overlayProfile = nullptr;

    for (const auto & profileEntry : m_configuration->profiles()) {
        const auto & devices = profileEntry.devices();
        if (!devices.empty() && std::find(devices.begin(), devices.end(), m_name) == devices.end())
            continue;
        if (profileEntry.name() == defaultProfileName) {
            defaultProfile = &profileEntry;
        } else if (profileEntry.name() == overlayProfileName) {
            overlayProfile = &profileEntry;
        } else if (profileEntry.lookup().match(context)) {
            DEBUG("profile matches: ", profileEntry.name());
            profile = &profileEntry;
        }
    }
    if (profile == nullptr) { profile = defaultProfile; }
    VERBOSE("selected profile <", profile->name(), ">");

    // Collect effect names from selected profile and resolve them
    std::vector<const Configuration::Effect *> effects;

    for (const auto & name : profile->effects()) {
        auto eit = std::find_if(m_configuration->effects().begin(), m_configuration->effects().end(),
                                [&name](auto & effect) { return effect.name() == name; });
        if (eit == m_configuration->effects().end()) {
            ERROR("profile <", profile->name(), "> references unknown effect <", name, ">");
            continue;
        }
        effects.push_back(&*eit);
    }
    if (overlayProfile != nullptr) {
        for (const auto & name : overlayProfile->effects()) {
            auto eit = std::find_if(m_configuration->effects().begin(), m_configuration->effects().end(),
                                    [&name](auto & effect) { return effect.name() == name; });
            if (eit == m_configuration->effects().end()) {
                ERROR("profile <", profile->name(), "> references unknown effect <", name, ">");
                continue;
            }
            effects.push_back(&*eit);
        }
    }

    RenderLoop::effect_plugin_list plugins;
    for (const auto & effect : effects) {
        const auto & loadedEffect = getEffect(*effect);
        const auto & effectPlugins = loadedEffect.plugins();
        std::copy(effectPlugins.begin(),
                  effectPlugins.end(),
                  std::back_inserter(plugins));
    }
    return plugins;
}

DeviceManager::LoadedEffect & DeviceManager::getEffect(const Configuration::Effect & conf)
{
    auto eit = std::lower_bound(
        m_effects.begin(), m_effects.end(), conf.name(),
        [](const auto & effect, const auto & name) { return effect.name() < name; }
    );
    if (eit != m_effects.end() && eit->name() == conf.name()) { return *eit; }

    // Load groups
    EffectPluginFactory::group_list groups;

    auto group_from_conf = [this](const auto & conf) {
        return m_keyDB.makeGroup(conf.first, conf.second.begin(), conf.second.end());
    };
    std::transform(conf.groups().begin(), conf.groups().end(),
                   std::back_inserter(groups), group_from_conf);
    std::transform(m_configuration->groups().begin(), m_configuration->groups().end(),
                   std::back_inserter(groups), group_from_conf);

    // Load effects
    auto & manager = EffectPluginManager::instance();
    LoadedEffect::plugin_list plugins;
    for (const auto & pluginConf : conf.plugins()) {
        auto plugin = manager.get(pluginConf.name());
        if (!plugin) {
            ERROR("plugin ", pluginConf.name(), " not found");
            continue;
        }
        VERBOSE("loaded plugin ", pluginConf.name());
        plugins.emplace_back(plugin->createEffect(*this, pluginConf, groups));
    }

    eit = m_effects.emplace(eit, conf.name(), std::move(plugins));
    return *eit;
}

keyleds::RenderLoop::effect_plugin_list DeviceManager::LoadedEffect::plugins() const
{
    RenderLoop::effect_plugin_list result;
    result.reserve(m_plugins.size());
    std::transform(m_plugins.begin(),
                   m_plugins.end(),
                   std::back_inserter(result),
                   [](auto & ptr) { return ptr.get(); });
    return result;
}
