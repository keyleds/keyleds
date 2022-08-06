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
#include "keyledsd/service/DeviceManager.h"
#include "keyledsd/service/DeviceManager_util.h"

#include "config.h"
#include "keyledsd/logging.h"
#include "keyledsd/service/EffectService.h"
#include "keyledsd/tools/DeviceWatcher.h"
#include <algorithm>
#include <cassert>
#include <unistd.h>

LOGGING("dev-manager");

namespace keyleds::service {

static constexpr char defaultProfileName[] = "__default__";
static constexpr char overlayProfileName[] = "__overlay__";

/****************************************************************************/

DeviceManager::DeviceManager(EffectManager & effectManager, FileWatcher & fileWatcher,
                             const tools::device::Description & description,
                             std::unique_ptr<device::Device> device,
                             const Configuration * conf)
    : m_effectManager(effectManager),
      m_configuration(nullptr),
      m_sysPath(description.sysPath()),
      m_serial(getSerial(description)),
      m_eventDevices(findEventDevices(description)),
      m_device(std::move(device)),
      m_fileWatcherSub(fileWatcher.subscribe(description.devNode(), FileWatcher::Event::Attrib,
                                             std::bind(&DeviceManager::handleFileEvent, this,
                                                       std::placeholders::_1, std::placeholders::_2,
                                                       std::placeholders::_3))),
      m_keyDB(setupKeyDatabase(*m_device)),
      m_renderLoop(*m_device, KEYLEDSD_RENDER_FPS)
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

    m_renderLoop.renderers().clear();
    m_effectGroups.clear();
    m_activeEffects.clear();

    m_configuration = conf;
    m_name = getDeviceName(*conf, m_serial);
}

void DeviceManager::setContext(const string_map & context)
{
    m_activeEffects = loadEffects(context);
    DEBUG("enabling ", m_activeEffects.size(), " effects for loop ", &m_renderLoop);

    // Notify newly-active effects of context change
    auto lock = m_renderLoop.lock();
    for (auto * effect : m_activeEffects) {
        effect->handleContextChange(context);
    }

    auto & renderers = m_renderLoop.renderers();
    renderers.clear();
    renderers.reserve(m_activeEffects.size());
    std::copy(m_activeEffects.begin(), m_activeEffects.end(), std::back_inserter(renderers));
}

void DeviceManager::handleFileEvent(FileWatcher::Event, uint32_t, const std::string &)
{
    int result = access(m_device->path().c_str(), R_OK | W_OK);
    setPaused(result != 0);
}

void DeviceManager::handleGenericEvent(const string_map & context)
{
    auto lock = m_renderLoop.lock();
    for (auto * effect : m_activeEffects) { effect->handleGenericEvent(context); }
}

void DeviceManager::handleKeyEvent(int keyCode, bool press)
{
    // Convert raw key code into a reference to its database entry
    auto it = m_keyDB.findKeyCode(keyCode);
    if (it == m_keyDB.end()) {
        DEBUG("unknown key ", keyCode, " on device ", m_serial);
        return;
    }

    // Pass event to active effects
    auto lock = m_renderLoop.lock();
    for (const auto & effect : m_activeEffects) { effect->handleKeyEvent(*it, press); }
    DEBUG("key ", it->name, " ", press ? "pressed" : "released", " on device ", m_serial);
}

void DeviceManager::setPaused(bool val)
{
    m_renderLoop.setPaused(val);
}

/// Applies the configuration to a string_map, matching profiles and resolving
/// effect names. Returns the list of Effect entries in the configuration that
/// should be loaded for the context. Returned list references Configuration
/// entries directly, and are therefore invalidated by any operation that
/// invalidates configuration's iterators.
std::vector<keyleds::plugin::Effect *> DeviceManager::loadEffects(const string_map & context)
{
    // Match context against profile lookups
    const Configuration::Profile * profile = nullptr;
    const Configuration::Profile * defaultProfile = nullptr;
    const Configuration::Profile * overlayProfile = nullptr;

    for (const auto & profileEntry : m_configuration->profiles) {
        const auto & devices = profileEntry.devices;
        if (!devices.empty() && std::find(devices.begin(), devices.end(), m_name) == devices.end()) {
            continue;
        }
        if (profileEntry.name == defaultProfileName) {
            defaultProfile = &profileEntry;
        } else if (profileEntry.name == overlayProfileName) {
            overlayProfile = &profileEntry;
        } else if (profileEntry.lookup.match(context)) {
            DEBUG("profile matches: ", profileEntry.name);
            profile = &profileEntry;
        }
    }
    if (!profile) {
        if (!defaultProfile) {
            ERROR("no profile matches and no default profile defined");
            return {};
        }
        profile = defaultProfile;
    }
    INFO("selected profile <", profile->name, ">");

    // Collect effect names from selected profile and resolve them
    std::vector<const Configuration::EffectGroup *> effectGroups;

    for (const auto & name : profile->effectGroups) {
        auto eit = std::find_if(m_configuration->effectGroups.begin(),
                                m_configuration->effectGroups.end(),
                                [&name](const auto & group) { return group.name == name; });
        if (eit == m_configuration->effectGroups.end()) {
            ERROR("profile <", profile->name, "> references unknown effect group <", name, ">");
            continue;
        }
        effectGroups.push_back(&*eit);
    }
    if (overlayProfile != nullptr) {
        for (const auto & name : overlayProfile->effectGroups) {
            auto eit = std::find_if(m_configuration->effectGroups.begin(),
                                    m_configuration->effectGroups.end(),
                                    [&name](const auto & group) { return group.name == name; });
            if (eit == m_configuration->effectGroups.end()) {
                ERROR("profile <", profile->name, "> references unknown effect group <", name, ">");
                continue;
            }
            effectGroups.push_back(&*eit);
        }
    }

    std::vector<Effect *> effectPtrs;
    for (const auto & effectGroup : effectGroups) {
        const auto & loadedEffectGroup = getEffectGroup(*effectGroup);
        const auto & effects = loadedEffectGroup.effects;
        std::transform(effects.begin(), effects.end(), std::back_inserter(effectPtrs),
                       [](const auto & ptr) { return ptr.get(); });
    }
    return effectPtrs;
}

const detail::EffectGroup & DeviceManager::getEffectGroup(const Configuration::EffectGroup & conf)
{
    auto eit = std::find_if(m_effectGroups.cbegin(), m_effectGroups.cend(),
                            [&](const auto & group) { return group.name == conf.name; });
    if (eit != m_effectGroups.cend()) { return *eit; }

    // Load key groups
    std::vector<KeyDatabase::KeyGroup> keyGroups;

    auto group_from_conf = [this](const auto & gconf) {
        return m_keyDB.makeGroup(gconf.name, gconf.keys.begin(), gconf.keys.end());
    };
    std::transform(conf.keyGroups.begin(), conf.keyGroups.end(),
                   std::back_inserter(keyGroups), group_from_conf);
    std::transform(m_configuration->keyGroups.begin(), m_configuration->keyGroups.end(),
                   std::back_inserter(keyGroups), group_from_conf);

    // Load effects
    std::vector<EffectManager::effect_ptr> effects;
    for (const auto & effectConf : conf.effects) {
        auto effect = m_effectManager.createEffect(
            effectConf.name, std::make_unique<EffectService>(
                *this, *m_configuration, effectConf, keyGroups
            )
        );
        if (!effect) {
            ERROR("plugin for effect ", effectConf.name, " not found");
            continue;
        }
        INFO("loaded plugin effect ", effectConf.name);
        effects.emplace_back(std::move(effect));
    }

    m_effectGroups.push_back({conf.name, std::move(effects)});
    return m_effectGroups.back();
}

} // namespace keyleds::service
