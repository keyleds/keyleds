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
#ifndef KEYLEDSD_DEVICEMANAGER_H_0517383B
#define KEYLEDSD_DEVICEMANAGER_H_0517383B

#include <QObject>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/RenderLoop.h"
#include <memory>
#include <string>
#include <vector>

namespace device { class Description; }

namespace keyleds {

class Context;
class LayoutDescription;

/** Main device manager
 *
 * Centralizes all operations and information for a specific device.
 * It is given a device instance to manage and a reference to current
 * configuration at creation time, and coordinates feature detection,
 * layout management, and related objects' life cycle.
 */
class DeviceManager final : public QObject
{
    Q_OBJECT
private:
    /** An effect, fully loaded with plugins
     *
     * Holds a list of loaded effect plugins to include while rendering device status
     * and the matching effect is enabled.
     */
    class LoadedEffect final
    {
    public:
        typedef std::vector<std::unique_ptr<EffectPlugin>> plugin_list;
    public:
                            LoadedEffect(plugin_list && plugins);
                            LoadedEffect(LoadedEffect &&) = default;
        RenderLoop::effect_plugin_list plugins() const;
    private:
        plugin_list m_plugins;
    };
    typedef std::map<std::string, LoadedEffect> effect_map;

public:
    typedef std::vector<std::string> dev_list;
public:
                            DeviceManager(const device::Description &,
                                          Device &&,
                                          const Configuration &,
                                          const Context &,
                                          QObject *parent = 0);
                            ~DeviceManager() override;

    const std::string &     serial() const noexcept { return m_serial; }
    const dev_list &        eventDevices() const { return m_eventDevices; }
    const Device &          device() const { return m_device; }
    const KeyDatabase &     keyDB() const { return m_keyDB; }

    RenderTarget            getRenderTarget() const { return RenderLoop::renderTargetFor(m_device); }

          bool              paused() const { return m_renderLoop.paused(); }

public slots:
    void                    setContext(const Context &);
    void                    handleGenericEvent(const Context &);
    void                    handleKeyEvent(int, bool);
    void                    setPaused(bool);
    void                    reloadConfiguration();

private:
    static std::string      getSerial(const device::Description &);
    static dev_list         findEventDevices(const device::Description &);
    static std::string      layoutName(const Device &);
    static LayoutDescription loadLayoutDescription(const Configuration &, const Device &);
    static KeyDatabase      buildKeyDB(const Configuration &, const Device &);

    RenderLoop::effect_plugin_list loadEffects(const Context &);
    LoadedEffect &          getEffect(const Configuration::Effect &);

private:
    const Configuration &   m_configuration;    ///< Reference to service configuration

    const std::string       m_serial;           ///< Device serial number
    const dev_list          m_eventDevices;     ///< List of event device paths that the
                                                ///  physical device can communicate on.
    Device                  m_device;           ///< The device handled by this manager
    const KeyDatabase       m_keyDB;            ///< Fully loaded key descriptions

    effect_map              m_effects;          ///< Map of effect id to loaded effect instance
    RenderLoop              m_renderLoop;       ///< The RenderLoop in charge of the device
};

};

#endif
