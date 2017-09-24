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
#include <fstream>
#include <iomanip>
#include <sstream>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/LayoutDescription.h"
#include "keyledsd/PluginManager.h"
#include "tools/Paths.h"
#include "config.h"
#include "logging.h"

LOGGING("dev-manager");

using keyleds::Configuration;
using keyleds::Context;
using keyleds::DeviceManager;
using keyleds::EffectPluginManager;
using keyleds::KeyDatabase;
using keyleds::LayoutDescription;
using keyleds::RenderLoop;

static const char defaultProfileName[] = "__default__";
static const char overlayProfileName[] = "__overlay__";

/****************************************************************************/

static std::vector<const Configuration::Effect *> effectsForContext(
    const Configuration & config, const std::string & serial, const Context & context)
{
    auto dit = config.devices().find(serial);
    const auto & devLabel = (dit == config.devices().end()) ? serial : dit->second;

    const Configuration::Profile * profile = nullptr;
    const Configuration::Profile * defaultProfile = nullptr;
    const Configuration::Profile * overlayProfile = nullptr;

    for (const auto & profileEntry : config.profiles()) {
        const auto & devices = profileEntry.devices();
        if (!devices.empty() && std::find(devices.begin(), devices.end(), devLabel) == devices.end())
            continue;
        if (profileEntry.name() == defaultProfileName) {
            defaultProfile = &profileEntry;
        } else if (profileEntry.name() == overlayProfileName) {
            overlayProfile = &profileEntry;
        } else if (profileEntry.lookup().match(context)) {
            profile = &profileEntry;
        }
    }
    if (profile == nullptr) { profile = defaultProfile; }
    DEBUG("effectsForContext selected profile <", profile->name(), ">");

    auto result = std::vector<const Configuration::Effect *>();
    for (const auto & name : profile->effects()) {
        auto eit = config.effects().find(name);
        if (eit == config.effects().end()) {
            ERROR("profile <", profile->name(), "> references unknown effect <", name, ">");
            continue;
        }
        result.push_back(&eit->second);
    }
    if (overlayProfile != nullptr) {
        for (const auto & name : overlayProfile->effects()) {
            auto eit = config.effects().find(name);
            if (eit == config.effects().end()) {
                ERROR("overlay profile references unknown effect <", name, ">");
                continue;
            }
            result.push_back(&eit->second);
        }
    }
    return result;
}

/****************************************************************************/

DeviceManager::LoadedEffect::LoadedEffect(plugin_list && plugins)
 : m_plugins(std::move(plugins))
{}

/****************************************************************************/

DeviceManager::DeviceManager(const device::Description & description, Device && device,
                             const Configuration & conf, const Context & context, QObject *parent)
    : QObject(parent),
      m_configuration(conf),
      m_serial(getSerial(description)),
      m_eventDevices(findEventDevices(description)),
      m_device(std::move(device)),
      m_keyDB(buildKeyDB(conf, m_device)),
      m_renderLoop(m_device, loadEffects(context), 16)
{
    m_renderLoop.setPaused(false);
}

DeviceManager::~DeviceManager()
{
    m_renderLoop.stop();
}

void DeviceManager::setContext(const Context & context)
{
    m_renderLoop.setEffects(loadEffects(context));
}

void DeviceManager::setPaused(bool val)
{
    m_renderLoop.setPaused(val);
}

void DeviceManager::reloadConfiguration()
{
    DEBUG("Device(", this, ") clearing renderers cache");
    m_renderLoop.setEffects({});
    m_effects.clear();
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

LayoutDescription DeviceManager::loadLayoutDescription(const Configuration & conf, const Device & device)
{
    if (!device.hasLayout()) { return LayoutDescription(std::string(), {}); }

    auto paths = conf.layoutPaths();
    for (const auto & path : paths::getPaths(paths::XDG::Data, true)) {
        paths.push_back(path + "/" KEYLEDSD_DATA_PREFIX "/layouts");
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

    KeyDatabase::key_map db;
    for (Device::block_list::size_type bidx = 0; bidx < device.blocks().size(); ++bidx) {
        const auto & block = device.blocks()[bidx];

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

            db.emplace(
                name,
                KeyDatabase::Key{
                    { bidx, kidx },
                    device.decodeKeyId(block.id(), keyId),
                    name,
                    position
                }
            );
        }
    }
    return db;
}

keyleds::RenderLoop::effect_plugin_list DeviceManager::loadEffects(const Context & context)
{
    const auto & effects = effectsForContext(m_configuration, m_serial, context);

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
    {
        auto it = m_effects.find(conf.name());
        if (it != m_effects.end()) { return it->second; }
    }

    // Load groups
    keyleds::EffectPluginFactory::group_map groups;
    for (const auto & group : conf.groups()) {
        auto keys = decltype(groups)::mapped_type();
        for (const auto & name : group.second) {
            auto it = m_keyDB.find(name);
            if (it != m_keyDB.end()) {
                keys.push_back(&it->second);
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
            auto it = m_keyDB.find(name);
            if (it != m_keyDB.end()) {
                keys.push_back(&it->second);
            } else {
                DEBUG("ignoring global key ", name, ": not found on device ", m_serial);
            }
        }
        groups.emplace(group.first, std::move(keys));
    }

    // Load effects
    auto & manager = EffectPluginManager::instance();
    LoadedEffect::plugin_list plugins;
    for (const auto & pluginConf : conf.plugins()) {
        auto plugin = manager.get(pluginConf.name());
        if (!plugin) {
            ERROR("plugin ", pluginConf.name(), " not found");
            continue;
        }
        DEBUG("loaded plugin ", pluginConf.name());
        plugins.push_back(plugin->createEffect(*this, pluginConf, groups));
    }

    auto it = m_effects.emplace(conf.name(), LoadedEffect(std::move(plugins))).first;
    return it->second;
}

keyleds::RenderLoop::effect_plugin_list DeviceManager::LoadedEffect::plugins() const
{
    keyleds::RenderLoop::effect_plugin_list result;
    result.reserve(m_plugins.size());
    std::transform(m_plugins.begin(),
                   m_plugins.end(),
                   std::back_inserter(result),
                   [](auto & ptr) { return ptr.get(); });
    return result;
}
