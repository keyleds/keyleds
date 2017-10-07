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
#include "keyledsd/effect/EffectManager.h"

#include <unistd.h>
#include <algorithm>
#include "keyledsd/effect/EffectService.h"
#include "keyledsd/effect/module.h"
#include "tools/DynamicLibrary.h"
#include "logging.h"

LOGGING("effect-manager");

using keyleds::effect::EffectManager;
using tools::DynamicLibrary;

static std::string lastError;
static void errorCallback(const char * err) { lastError = err; }
static constexpr struct host_definition hostDefinition = {
    KEYLEDSD_VERSION_MAJOR, KEYLEDSD_VERSION_MINOR,
    errorCallback
};

static constexpr char moduleEntry[] = "keyledsd_module";
static constexpr std::array<unsigned char, 16> keyledsdModuleUUID = {{
    KEYLEDSD_MODULE_SIGNATURE
}};

/****************************************************************************/

class EffectManager::PluginTracker final
{
    using Plugin = interface::Plugin;
public:
    PluginTracker(EffectManager & manager, std::string name, DynamicLibrary && library,
                  const module_definition * definition, Plugin * instance)
     : m_manager(manager),
       m_name(std::move(name)),
       m_library(std::move(library)),
       m_definition(definition),
       m_instance(instance),
       m_useCount(0) {}
    ~PluginTracker() { m_manager.unload(*this); }

    const std::string &         name() const { return m_name; }
    const module_definition *   definition() const { return m_definition; }
    Plugin *                    instance() const { return m_instance; }

    void                        incrementUseCount() { ++m_useCount; }
    void                        decrementUseCount() { --m_useCount; }
    unsigned                    useCount() const { return m_useCount; }

private:
    EffectManager &             m_manager;
    const std::string           m_name;
    DynamicLibrary              m_library;
    const module_definition *   m_definition;
    Plugin *                    m_instance;
    unsigned                    m_useCount;
};

/****************************************************************************/

EffectManager::effect_deleter::effect_deleter()
 : m_manager(nullptr),
   m_tracker(nullptr),
   m_service(nullptr)
{}

EffectManager::effect_deleter::effect_deleter(EffectManager * manager, PluginTracker * tracker,
                                              std::unique_ptr<EffectService> service)
 : m_manager(manager),
   m_tracker(tracker),
   m_service(std::move(service))
{}

EffectManager::effect_deleter::effect_deleter(effect_deleter &&) noexcept = default;

EffectManager::effect_deleter::~effect_deleter() {}

void EffectManager::effect_deleter::operator()(interface::Effect * ptr) const
{
    m_manager->destroyEffect(m_tracker, ptr);
}

/****************************************************************************/

EffectManager::EffectManager() {}
EffectManager::~EffectManager() {}

bool EffectManager::add(const std::string & name, const module_definition * definition, std::string * error)
{
    auto * plugin = static_cast<interface::Plugin *>((*definition->initialize)(&hostDefinition));
    if (!plugin) {
        if (error) { *error = std::move(lastError); }
        return false;
    }

    try {
        m_plugins.push_back(std::make_unique<PluginTracker>(
            *this, name, DynamicLibrary(), definition, plugin
        ));
    } catch (...) {
        (*definition->shutdown)(&hostDefinition, plugin);
        throw;
    }
    VERBOSE("initialized static plugin <", name, ">");
    return true;
}

bool EffectManager::load(const std::string & name, std::string * error)
{
    DynamicLibrary library;

    std::string fileName = "fx_" + name + ".so";
    for (const auto & path : m_searchPaths) {
        std::string fullPath = path + '/' + fileName;
        if (access(fullPath.c_str(), F_OK) < 0) { continue; }
        VERBOSE("loading ", name, " from ", fullPath);

        library = DynamicLibrary::load(fullPath, error);
        if (!library) { return false; }
    }

    if (!library) {
        if (error) { *error = "no module <" + fileName + "> in search paths"; }
        return false;
    }

    auto definition = static_cast<module_definition *>(library.getSymbol(moduleEntry));
    if (!definition) {
        if (error) { *error = "module entry point not found"; }
        return false;
    }

    if (!std::equal(keyledsdModuleUUID.begin(), keyledsdModuleUUID.end(), definition->signature)) {
        if (error) { *error = "invalid plugin signature"; }
        return false;
    }

    if (definition->abi_version != KEYLEDSD_ABI_VERSION) {
        if (error) { *error = "plugin was compiled with an incompatible compiler"; }
        return false;
    }

    if (definition->major != KEYLEDSD_VERSION_MAJOR) {
        if (error) {
            *error = "plugin version " + std::to_string(definition->major)
                   + " does not matchkeyleds version";
        }
        return false;
    }

    auto * plugin = static_cast<interface::Plugin *>((*definition->initialize)(&hostDefinition));
    if (!plugin) {
        if (error) { *error = std::move(lastError); }
        return false;
    }

    try {
        m_plugins.push_back(std::make_unique<PluginTracker>(
            *this, name, std::move(library), definition, plugin
        ));
    } catch (...) {
        (*definition->shutdown)(&hostDefinition, plugin);
        throw;
    }
    INFO("loaded plugin <", name, ">");
    return true;
}

void EffectManager::unload(PluginTracker & tracker)
{
    if (tracker.useCount() != 0) {
        CRITICAL("attempting to unload plugin ", tracker.name(), " but it has ",
                 tracker.useCount(), " objects still alive, trying anyway...");
    }

    if (!(*tracker.definition()->shutdown)(&hostDefinition, tracker.instance())) {
        ERROR("unloading plugin ", tracker.name(), ": ", lastError);
        lastError.clear();
    }
    VERBOSE("unloaded plugin <", tracker.name(), ">");
}

std::vector<std::string> EffectManager::pluginNames() const
{
    std::vector<std::string> names;
    names.reserve(m_plugins.size());
    std::transform(m_plugins.begin(), m_plugins.end(), std::back_inserter(names),
                   [](const auto & info){ return info->name(); });
    return names;
}

EffectManager::effect_ptr EffectManager::createEffect(
    const std::string & name, const DeviceManager & manager,
    const Configuration::Effect & conf, const std::vector<device::KeyDatabase::KeyGroup> & keyGroups)
{
    interface::Effect * effect = nullptr;
    PluginTracker * tracker = nullptr;

    auto service = std::make_unique<EffectService>(manager, conf, keyGroups);

    for (auto & info : m_plugins) {
        effect = info->instance()->createEffect(name, *service);
        if (effect) { tracker = info.get(); break; }
    }

    if (!effect) {
        VERBOSE("effect ", name, " not loaded, attempting auto-load");

        std::string error;
        if (!load(name, &error)) {
            ERROR(error);
            return {};
        }

        tracker = m_plugins.back().get();
        effect = tracker->instance()->createEffect(name, *service);
        if (!effect) {
            ERROR("error creating effect ", name, ": plugin returned nullptr");
            return nullptr;
        }
    }

    tracker->incrementUseCount();
    return effect_ptr(effect, {this, tracker, std::move(service)});
}

void EffectManager::destroyEffect(PluginTracker * tracker, interface::Effect * effect)
{
    tracker->instance()->destroyEffect(effect);
    tracker->decrementUseCount();
}
