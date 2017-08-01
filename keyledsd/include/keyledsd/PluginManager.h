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
#ifndef KEYLEDSD_PLUGIN_MANAGER_H_72D24293
#define KEYLEDSD_PLUGIN_MANAGER_H_72D24293

#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/KeyDatabase.h"
#include "config.h"

namespace keyleds {

class DeviceManager;
class Renderer;

/****************************************************************************/
/** Renderer plugin interface
 *
 * Each plugin must expose this interface and register it with the plugin manager.
 * This interface is instanciated exactly once per plugin, and is used to create
 * renderer objects when profiles are loaded.
 */
class IRendererPlugin
{
public:
    typedef std::map<std::string, std::vector<const KeyDatabase::Key*>> group_map;
public:
    virtual                 ~IRendererPlugin();

    virtual const std::string & name() const noexcept = 0;
    virtual std::unique_ptr<Renderer> createRenderer(const DeviceManager &,
                                                     const Configuration::Plugin &,
                                                     const group_map &) = 0;
};

/****************************************************************************/
/** Renderer plugin manager
 *
 * Singleton manager that tracks renderer plugins
 */
class RendererPluginManager final
{
    // Singleton lifecycle
                            RendererPluginManager() = default;
public:
                            RendererPluginManager(const RendererPluginManager &) = delete;
                            RendererPluginManager(RendererPluginManager &&) = delete;

    static RendererPluginManager & instance();

    // Plugin management
public:
    typedef std::unordered_map<std::string, IRendererPlugin *> plugin_map;
public:

    const plugin_map &      plugins() const { return m_plugins; }
    void                    registerPlugin(std::string name, IRendererPlugin * plugin);
    IRendererPlugin *       get(const std::string & name);

private:
    plugin_map              m_plugins;
};

/****************************************************************************/
/** Renderer plugin template
 *
 * Provides a default implementation for a plugin, allowing simple registration
 * of IRender-derived classes with the plugin manager.
 */
template<class T> class RendererPlugin : public IRendererPlugin
{
public:
    RendererPlugin(std::string name) : m_name(name)
    {
        RendererPluginManager::instance().registerPlugin(m_name, this);
    }

    const std::string & name() const noexcept override { return m_name; }
    std::unique_ptr<Renderer> createRenderer(const DeviceManager & manager,
                                             const Configuration::Plugin & conf,
                                             const group_map & groups) override
    {
        return std::make_unique<T>(manager, conf, groups);
    }
protected:
    const std::string       m_name;
};

#define REGISTER_RENDERER(name, klass) \
    static USED const keyleds::RendererPlugin<klass> maker_##klass(name);

/****************************************************************************/

};

#endif
