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
#include "keyledsd/service/EffectManager.h"

#include "keyledsd/logging.h"
#include "keyledsd/plugin/module.h"
#include "keyledsd/tools/DynamicLibrary.h"
#include <algorithm>
#include <array>
#include <unistd.h>

LOGGING("effect-manager");

using keyleds::plugin::module_definition;
using keyleds::service::EffectManager;
using keyleds::tools::DynamicLibrary;

static std::string lastError;
static void errorCallback(const char * err) { lastError = err; }
static constexpr struct keyleds::plugin::host_definition hostDefinition = {
    KEYLEDSD_VERSION_MAJOR, KEYLEDSD_VERSION_MINOR,
    errorCallback
};

static constexpr char moduleEntry[] = "keyledsd_module";
static constexpr std::array<unsigned char, 16> keyledsdModuleUUID = {{
    KEYLEDSD_MODULE_SIGNATURE
}};

/****************************************************************************/

/** Loaded plugin library tracker.
 *
 * Keeps track of a loaded plugin: use count, detected entry points.
 * Calls EffectManager::unload() when destroyed/
 */
class EffectManager::PluginTracker final
{
public:
    PluginTracker(EffectManager & manager, std::string name, DynamicLibrary && library,
                  const module_definition * definition, plugin::Plugin * instance)
     : m_manager(manager),
       m_name(std::move(name)),
       m_library(std::move(library)),
       m_definition(definition),
       m_instance(instance) {}
                                PluginTracker(const PluginTracker &) = delete;
    PluginTracker &             operator=(const PluginTracker &) = delete;
                                ~PluginTracker() { m_manager.unload(*this); }

    const std::string &         name() const { return m_name; }
    const module_definition *   definition() const { return m_definition; }
    plugin::Plugin *            instance() const { return m_instance; }

    void                        incrementUseCount() { ++m_useCount; }
    void                        decrementUseCount() { --m_useCount; }
    unsigned                    useCount() const { return m_useCount; }

private:
    EffectManager &             m_manager;      ///< Owning plugin manager instance
    const std::string           m_name;         ///< Name used to load the plugin
    DynamicLibrary              m_library;      ///< Actual underlying library of the plugin
    const module_definition *   m_definition;   ///< Plugin description structure
    plugin::Plugin *            m_instance;     ///< Instance created by the plugin itself
    unsigned                    m_useCount = 0; ///< Number of loaded effects using this plugin
};

/****************************************************************************/

EffectManager::effect_deleter::effect_deleter() = default;

EffectManager::effect_deleter::effect_deleter(EffectManager * manager, PluginTracker * tracker,
                                              std::unique_ptr<plugin::EffectService> service)
 : m_manager(manager),
   m_tracker(tracker),
   m_service(std::move(service))
{}

EffectManager::effect_deleter::effect_deleter(effect_deleter &&) noexcept = default;

EffectManager::effect_deleter::~effect_deleter() = default;

void EffectManager::effect_deleter::operator()(plugin::Effect * ptr) const
{
    m_manager->destroyEffect(*m_tracker, *m_service, ptr);
}

/****************************************************************************/

EffectManager::EffectManager() = default;
EffectManager::~EffectManager() = default;

/** Add a static / pre-loaded plugin to the manager.
 * @param name Plugin name
 * @param definition Plugin description structure, with signature and entry point pointers.
 * @param [out] error A string that will hold the error message on failure. May be `NULL`.
 * @return `true` on success, `false` on failure.
 */
bool EffectManager::add(const std::string & name, const module_definition * definition, std::string * error)
{
    // Invoke plugin's initialization entry point
    auto * plugin = static_cast<plugin::Plugin *>((*definition->initialize)(&hostDefinition));
    if (!plugin) {
        if (error) { *error = std::move(lastError); }
        return false;
    }

    // Create plugin tracker entry
    try {
        m_plugins.push_back(std::make_unique<PluginTracker>(
            *this, name, DynamicLibrary(), definition, plugin
        ));
    } catch (...) {
        (*definition->shutdown)(&hostDefinition, plugin);
        throw;
    }
    INFO("initialized static plugin <", name, ">");
    return true;
}

/** Load a plugin by name.
 * Given plugin is searched in all paths specified with searchPaths(). First matching library
 * is used.
 * @param name Plugin name.
 * @param [out] error A string that will hold the error message on failure. May be `NULL`.
 * @return `true` on success, `false` on failure.
 */
bool EffectManager::load(const std::string & name, std::string * error)
{
    // Locate library and open it
    auto fullPath = locatePlugin(name);
    if (fullPath.empty())  {
        if (error) { *error = "no module <" + name + "> in search paths"; }
        return false;
    }

    INFO("loading ", name, " from ", fullPath);

    auto library = DynamicLibrary::load(fullPath, error);
    if (!library) { return false; }

    // Lookup plugin definition structure
    auto definition = static_cast<const module_definition *>(library.getSymbol(moduleEntry));
    if (!definition) {
        if (error) { *error = "module entry point not found"; }
        return false;
    }

    // Check plugin signature and ABI version
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

    // Invoke plugin's initialization entry point
    auto * plugin = static_cast<plugin::Plugin *>((*definition->initialize)(&hostDefinition));
    if (!plugin) {
        if (error) { *error = std::move(lastError); }
        return false;
    }

    // Create plugin tracker entry
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

/** Get full path of named plugin
 * Be careful of race conditions, do not assume path is still valid when returned.
 * @return Full path, or empty string if none was found.
 */
std::string EffectManager::locatePlugin(const std::string & name) const
{
    // Load first library that matches plugin name
    const auto fileName = "fx_" + name + ".so";

    for (const auto & path : m_searchPaths) {
        auto fullPath = path;
        if (fullPath.back() != '/') { fullPath += '/'; }
        fullPath += fileName;

        if (access(fullPath.c_str(), F_OK) == 0) {
            return fullPath;
        }
    }
    return {};
}

/** Unload a plugin.
 * Callback invoked when destroying an entry in m_plugins.
 * @param tracker Plugin tracker instance
 */
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
    INFO("unloaded plugin <", tracker.name(), ">");
}

/** List of loaded plugins
 * @return Vector of loaded plugin names.
 */
std::vector<std::string> EffectManager::pluginNames() const
{
    std::vector<std::string> names;
    names.reserve(m_plugins.size());
    std::transform(m_plugins.begin(), m_plugins.end(), std::back_inserter(names),
                   [](const auto & info){ return info->name(); });
    return names;
}

/** Create an effect from a plugin.
 *
 * First query all loaded plugin, to see whether any can load the given effect.
 * Then try to load a plugin with that name. The point is this allows a single
 * plugin to handle many effects. For instance, lua plugin does that, looking
 * for a lua script with given name.
 * @param name Effect name.
 * @param service An effect service instance, that will handle communication with the effect
 *                during its lifetime.
 * @return Instantiated effect.
 */
EffectManager::effect_ptr EffectManager::createEffect(
    const std::string & name, std::unique_ptr<plugin::EffectService> service)
{
    plugin::Effect * effect = nullptr;
    PluginTracker * tracker = nullptr;
    bool sawName = false;

    // Look for a loaded plugin that can handle requested effect
    for (auto & info : m_plugins) {
        if (name == info->name()) { sawName = true; }
        effect = info->instance()->createEffect(name, *service);
        if (effect) { tracker = info.get(); break; }
    }

    if (!effect && !sawName) {
        DEBUG("effect ", name, " not loaded, attempting auto-load");

        std::string error;
        if (!load(name, &error)) {
            ERROR(error);
            return {};
        }

        // Create effect from loaded plugin
        tracker = m_plugins.back().get();
        effect = tracker->instance()->createEffect(name, *service);
    }

    if (!effect) {
        ERROR("error creating effect ", name, ": plugin returned nullptr");
        return nullptr;
    }

    tracker->incrementUseCount();
    return effect_ptr(effect, {this, tracker, std::move(service)});
}

/** Tracker callback to destroy an effect
 * @param tracker Plugin tracker instance invoking the callback.
 * @param service Effect service that handles communication with the effect.
 * @param effect Actual effect to destroy.
 */
void EffectManager::destroyEffect(PluginTracker & tracker, plugin::EffectService & service,
                                  plugin::Effect * effect)
{
    tracker.instance()->destroyEffect(effect, service);
    tracker.decrementUseCount();
}
