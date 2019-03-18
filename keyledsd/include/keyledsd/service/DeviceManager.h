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
#ifndef KEYLEDSD_INTERNAL
#   error "Internal header - must not be pulled into plugins"
#endif

#include "keyledsd/service/Configuration.h"
#include "keyledsd/service/EffectManager.h"
#include "keyledsd/service/RenderLoop.h"
#include "keyledsd/tools/FileWatcher.h"
#include "keyledsd/KeyDatabase.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace keyleds::device { class Device; }
namespace keyleds::tools::device { class Description; }

namespace keyleds::service {

/****************************************************************************/

namespace detail {
    /// An effect group, fully loaded with effects
    struct EffectGroup final
    {
        std::string                             name;
        std::vector<EffectManager::effect_ptr>  effects;
    };
}


/** Main device manager
 *
 * Centralizes all operations and information for a specific device.
 * It is given a device instance to manage and a reference to current
 * configuration at creation time, and coordinates feature detection,
 * layout management, and related objects' life cycle.
 */
class DeviceManager final
{
    using Effect = plugin::Effect;
    using FileWatcher = tools::FileWatcher;
    using string_map = std::vector<std::pair<std::string, std::string>>;
public:
    using dev_list = std::vector<std::string>;
public:
                            DeviceManager(EffectManager &, FileWatcher &,
                                          const tools::device::Description &,
                                          std::unique_ptr<device::Device>,
                                          const Configuration *);
                            ~DeviceManager();

    const std::string &     sysPath() const noexcept { return m_sysPath; }
    const std::string &     serial() const noexcept { return m_serial; }
    const std::string &     name() const noexcept { return m_name; }
    const dev_list &        eventDevices() const { return m_eventDevices; }
    const device::Device &  device() const { return *m_device; }
    const KeyDatabase &     keyDB() const { return m_keyDB; }

          bool              paused() const { return m_renderLoop.paused(); }

public:
    void                    setConfiguration(const Configuration *);
    void                    setContext(const string_map &);
    void                    handleFileEvent(FileWatcher::Event, uint32_t, const std::string &);
    void                    handleGenericEvent(const string_map &);
    void                    handleKeyEvent(int, bool);
    void                    setPaused(bool);
    void                    forceRefresh() { m_renderLoop.forceRefresh(); }

private:
    /// Loads the list of effects to activate for the given context
    std::vector<Effect *>   loadEffects(const string_map & context);

    /// Instanciates an effect, combining its configuration with this device's info
    const detail::EffectGroup & getEffectGroup(const Configuration::EffectGroup &);

private:
    EffectManager &         m_effectManager;    ///< Manages the lifecycle of effects
    const Configuration *   m_configuration;    ///< Reference to service configuration

    const std::string       m_sysPath;          ///< Device path on sys filesystem
    const std::string       m_serial;           ///< Device serial number
          std::string       m_name;             ///< User-given name
    const dev_list          m_eventDevices;     ///< List of event device paths that the
                                                ///  physical device can communicate on.
    std::unique_ptr<device::Device> m_device;   ///< The device handled by this manager
    FileWatcher::subscription m_fileWatcherSub; ///< Ensures we get notifications for devnode events
    const KeyDatabase       m_keyDB;            ///< Fully loaded key descriptions

    std::vector<detail::EffectGroup> m_effectGroups;    ///< Loaded effect group instances
    RenderLoop              m_renderLoop;       ///< The RenderLoop in charge of the device
    std::vector<Effect *>   m_activeEffects;    ///< Effects currently active on m_renderLoop
};

/****************************************************************************/

} // namespace keyleds::service

#endif
