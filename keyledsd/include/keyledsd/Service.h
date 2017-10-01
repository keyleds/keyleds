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
#ifndef KEYLEDSD_KEYLEDSSERVICE_884F711D
#define KEYLEDSD_KEYLEDSSERVICE_884F711D

#include <QObject>
#include <memory>
#include <string>
#include <vector>
#include "keyledsd/Context.h"
#include "keyledsd/Device.h"
#include "tools/DeviceWatcher.h"
#include "tools/FileWatcher.h"

namespace xlib { class Display; }

namespace keyleds {

class Configuration;
class DeviceManager;
class DisplayManager;

/** Main service
 *
 * Only one instance typically exists per run. It ties all other objects
 * together, notably managing event watchers and device managers, and
 * passing messages around.
 */
class Service final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive);
public:
    using device_list = std::vector<std::unique_ptr<DeviceManager>>;
    using display_list = std::vector<std::unique_ptr<DisplayManager>>;
public:
                        Service(std::unique_ptr<Configuration>, QObject *parent = nullptr);
                        Service(const Service &) = delete;
                        ~Service() override;

    const Configuration & configuration() const { return *m_configuration; }
    bool                autoQuit() const { return m_autoQuit; }
    const Context &     context() const { return m_context; }
    bool                active() const { return m_active; }
    const device_list & devices() const { return m_devices; }

    void                setConfiguration(std::unique_ptr<Configuration>);
    void                setAutoQuit(bool);
    void                setActive(bool val);

public slots:
    void                init();             ///< Invoked once to complete event-loop-depenent setup
    void                setContext(const keyleds::Context &);
    void                handleGenericEvent(const keyleds::Context &);
    void                handleKeyEvent(const std::string &, int, bool);

signals:
    /// Fires whenever a device is added - whether it is in devices list is undefined
    void                deviceManagerAdded(keyleds::DeviceManager &);
    /// Fires whenever a device is removed - whether it is still in devices list is undefined
    void                deviceManagerRemoved(keyleds::DeviceManager &);

private:
    // Events from watchers
    void                onConfigurationFileChanged(FileWatcher::event);
    void                onDeviceAdded(const device::Description &);
    void                onDeviceRemoved(const device::Description &);
    void                onDisplayAdded(std::unique_ptr<xlib::Display> &);
    void                onDisplayRemoved();
private:
    std::unique_ptr<Configuration> m_configuration;
    bool                m_autoQuit;         ///< Quit when last device is removed?

    Context             m_context;          ///< Current context. Used when instanciating new managers
    bool                m_active;           ///< If clear, the service stops watching devices
    device_list         m_devices;          ///< Map of serial number to DeviceManager instances
    display_list        m_displays;         ///< Connections to X displays

    DeviceWatcher       m_deviceWatcher;    ///< Connection to libudev
    FileWatcher         m_fileWatcher;      ///< Connection to inotify
    FileWatcher::subscription m_fileWatcherSub; ///< Notifications for conf change
};

};

#endif
