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
#include <unordered_map>
#include "keyledsd/Context.h"
#include "keyledsd/Device.h"
#include "tools/DeviceWatcher.h"
#include "tools/FileWatcher.h"

namespace keyleds {

class Configuration;
class DeviceManager;

/** Main service
 *
 * Manages devices and their manager, and dispatches events.
 */
class Service : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive);
public:
    typedef std::unordered_map<std::string, std::unique_ptr<DeviceManager>> device_map;
public:
                        Service(Configuration & configuration, QObject *parent = 0);
                        Service(const Service &) = delete;
                        ~Service() override;

    Configuration &     configuration() { return m_configuration; }
    const Configuration & configuration() const { return m_configuration; }
    const Context &     context() const { return m_context; }
    bool                active() const { return m_active; }
    const device_map &  devices() const { return m_devices; }

public slots:
    void                init();
    void                setActive(bool val);
    void                setContext(const keyleds::Context &);

signals:
    void                deviceManagerAdded(keyleds::DeviceManager &);   // Fires right before device is added
    void                deviceManagerRemoved(keyleds::DeviceManager &); // Fires right after device is removed

private slots:
    void                onDeviceAdded(const device::Description &);
    void                onDeviceRemoved(const device::Description &);

private:
    static void         onFileWatchEvent(void *, FileWatcher::event, uint32_t, std::string);

private:
    Configuration &     m_configuration;    ///< Service configuration
    Context             m_context;          ///< Current context. Used when instanciating new managers
    bool                m_active;           ///< If clear, the service stops watching devices
    device_map          m_devices;          ///< Map of serial number to DeviceManager instances

    DeviceWatcher       m_deviceWatcher;    ///< Connection to libudev
    FileWatcher         m_fileWatcher;      ///< Connection to inotify
};

};

#endif
