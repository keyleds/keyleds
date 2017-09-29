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

#include <memory>
#include <string>
#include <vector>
#include "keyledsd/Configuration.h"
#include "keyledsd/KeyDatabase.h"
#include "config.h"

namespace keyleds {

class Context;
class DeviceManager;

/****************************************************************************/
/** Effect plugin interface
 *
 * This interface is instanciated once per device, through the EffectPluginFactory
 */
class EffectPlugin
{
public:
    virtual         ~EffectPlugin();

    /// Modifies the target to reflect effect's display once the specified time has elapsed
    virtual void    render(unsigned long nanosec, RenderTarget & target) = 0;

    /// Invoked whenever the context of the service has changed while the plugin is active.
    /// Since plugins are loaded on context changes, this means this is always called
    /// once before periodic calls to render start.
    virtual void    handleContextChange(const Context &);

    /// Invoked whenever the Service is sent a generic event while the plugin is active.
    /// Context holds whatever values the event includes, keyledsd does not use it.
    virtual void    handleGenericEvent(const Context &);

    /// Invoked whenever the user presses or releases a key while the plugin is active.
    virtual void    handleKeyEvent(const KeyDatabase::Key &, bool press);
};

/****************************************************************************/
/** Effect plugin factory interface
 *
 * Each effect plugin must register a factory it with the plugin manager.
 * This interface is instanciated exactly once per plugin, and is used to create
 * renderer objects when devices are initialized.
 */
class EffectPluginFactory
{
public:
    using group_list = std::vector<KeyDatabase::KeyGroup>;
public:
    virtual                 ~EffectPluginFactory();

    virtual const std::string & name() const noexcept = 0;

    /// Builds an effect plugin instance for a specific device.
    /// Passed references are guaranteed to remain valid until after the
    /// EffectPlugin is destroyed.
    virtual std::unique_ptr<EffectPlugin> createEffect(const DeviceManager &,
                                                       const Configuration::Plugin &,
                                                       const group_list) = 0;
};

/****************************************************************************/
/** Effect plugin manager
 *
 * Singleton manager that tracks effect plugin factories
 */
class EffectPluginManager final
{
    // Singleton lifecycle
                            EffectPluginManager() = default;
public:
                            EffectPluginManager(const EffectPluginManager &) = delete;
                            EffectPluginManager(EffectPluginManager &&) = delete;

    static EffectPluginManager & instance();

    // Plugin management
public:
    using plugin_list = std::vector<EffectPluginFactory *>;
public:

    const plugin_list &     plugins() const { return m_plugins; }
    void                    registerPlugin(EffectPluginFactory *);
    EffectPluginFactory *   get(const std::string & name);

private:
    plugin_list             m_plugins;
};

/****************************************************************************/
/** Renderer plugin template
 *
 * Provides a default implementation for a plugin factory, allowing simple
 * registration of simple EffectPlugin classes with the plugin manager.
 */
template<class T> class DefaultEffectPluginFactory final : public EffectPluginFactory
{
public:
    DefaultEffectPluginFactory(std::string name) : m_name(name)
    {
        EffectPluginManager::instance().registerPlugin(this);
    }

    const std::string & name() const noexcept override { return m_name; }
    std::unique_ptr<EffectPlugin> createEffect(const DeviceManager & manager,
                                               const Configuration::Plugin & conf,
                                               const group_list groups) override
    {
        return std::make_unique<T>(manager, conf, std::move(groups));
    }
protected:
    const std::string   m_name;
};

#define REGISTER_EFFECT_PLUGIN(name, klass) \
    static USED const keyleds::DefaultEffectPluginFactory<klass> maker_##klass(name);

/****************************************************************************/

};

#endif
