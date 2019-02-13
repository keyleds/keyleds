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
#ifndef KEYLEDSD_EFFECT_EFFECT_MANAGER_H_E6520FC7
#define KEYLEDSD_EFFECT_EFFECT_MANAGER_H_E6520FC7

#include "keyledsd/plugin/interfaces.h"
#include <memory>
#include <string>
#include <vector>

namespace keyleds::plugin { struct module_definition; }

namespace keyleds::service {

/****************************************************************************/

class EffectManager final
{
    class PluginTracker;

    /** Object that tracks all resources associated to an effect so it can
     * dispose of them when it deletes the effect itself
     */
    class effect_deleter final
    {
        EffectManager * m_manager = nullptr;
        PluginTracker * m_tracker = nullptr;
        std::unique_ptr<plugin::EffectService> m_service;
    public:
        effect_deleter();
        effect_deleter(EffectManager * manager, PluginTracker * tracker,
                       std::unique_ptr<plugin::EffectService> service);
        effect_deleter(effect_deleter &&) noexcept;
        ~effect_deleter();
        void operator()(plugin::Effect * ptr) const;
    };

    using path_list = std::vector<std::string>;
public:
    using effect_ptr = std::unique_ptr<plugin::Effect, effect_deleter>;

public:
                        EffectManager();
                        ~EffectManager();
                        EffectManager(const EffectManager &) = delete;
    EffectManager &     operator=(const EffectManager &) = delete;

    path_list &         searchPaths() { return m_searchPaths; }
    const path_list &   searchPaths() const { return m_searchPaths; }

    /// Adds a module to the manager - used for statically linked modules
    bool                add(const std::string & name, const plugin::module_definition *,
                            std::string * error);

    /// Loads a dynamic module
    bool                load(const std::string & name, std::string * error);

    /// Returns a list of all known plugin names
    std::vector<std::string> pluginNames() const;

    /// Instantiates the effect of given name, using the passed configuration
    effect_ptr          createEffect(const std::string & name,
                                     std::unique_ptr<plugin::EffectService>);

private:
    std::string         locatePlugin(const std::string & name) const;
    void                unload(PluginTracker &);
    void                destroyEffect(PluginTracker &, plugin::EffectService &,
                                      plugin::Effect *);

private:
    path_list                                   m_searchPaths;
    std::vector<std::unique_ptr<PluginTracker>> m_plugins;
};

/****************************************************************************/

} // namespace keyleds::service

#endif
