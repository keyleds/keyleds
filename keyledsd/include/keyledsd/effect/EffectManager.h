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

#include <memory>
#include <string>
#include <vector>
#include "keyledsd/effect/interfaces.h"

struct module_definition;

namespace keyleds { namespace effect {

/****************************************************************************/

class EffectManager final
{
    class PluginTracker;

    class effect_deleter final
    {
        EffectManager * m_manager;
        PluginTracker * m_tracker;
        std::unique_ptr<interface::EffectService> m_service;
    public:
        effect_deleter();
        effect_deleter(EffectManager * manager, PluginTracker * tracker,
                       std::unique_ptr<interface::EffectService> service);
        effect_deleter(effect_deleter &&) noexcept;
        ~effect_deleter();
        void operator()(interface::Effect * ptr) const;
    };

    using path_list = std::vector<std::string>;
public:
    using effect_ptr = std::unique_ptr<interface::Effect, effect_deleter>;

public:
                        EffectManager();
                        ~EffectManager();
                        EffectManager(const EffectManager &) = delete;

    path_list &         searchPaths() { return m_searchPaths; }
    const path_list &   searchPaths() const { return m_searchPaths; }

    /// Adds a module to the manager - used for statically linked modules
    bool                add(const std::string & name, const module_definition *,
                            std::string * error);

    /// Loads a dynamic module
    bool                load(const std::string & name, std::string * error);

    /// Returns a list of all known plugin names
    std::vector<std::string> pluginNames() const;

    /// Instantiates the effect of given name, using the passed configuration
    effect_ptr          createEffect(const std::string & name, std::unique_ptr<interface::EffectService>);

private:
    void                unload(PluginTracker &);
    void                destroyEffect(PluginTracker &, interface::EffectService &, interface::Effect *);

private:
    path_list                                   m_searchPaths;
    std::vector<std::unique_ptr<PluginTracker>> m_plugins;
};

/****************************************************************************/

} } // namespace keyleds::effect

#endif
