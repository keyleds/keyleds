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
#include <QCoreApplication>
#include <unistd.h>
#include "keyledsd/Configuration.h"
#include "keyledsd/ContextWatcher.h"
#include "keyledsd/Device.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/Service.h"
#include "keyleds.h"
#include "logging.h"

LOGGING("service");

using keyleds::Context;
using keyleds::Service;

/****************************************************************************/

Service::Service(Configuration & configuration, QObject * parent)
    : QObject(parent),
      m_configuration(configuration),
      m_deviceWatcher(nullptr)
{
    m_active = false;
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceAdded(const device::Description &)),
                     this, SLOT(onDeviceAdded(const device::Description &)));
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceRemoved(const device::Description &)),
                     this, SLOT(onDeviceRemoved(const device::Description &)));
    DEBUG("created");
}

Service::~Service()
{
    setActive(false);
    m_devices.clear();
    DEBUG("destroyed");
}

void Service::init()
{
    setActive(true);
}

void Service::setActive(bool active)
{
    DEBUG("switching to ", active ? "active" : "inactive", " mode");
    m_deviceWatcher.setActive(active);
    m_active = active;
}

void Service::setContext(const Context & context)
{
    DEBUG("setContext ", context);
    m_context = context;
    for (auto & deviceEntry : m_devices) { deviceEntry.second->setContext(m_context); }
}

void Service::onDeviceAdded(const device::Description & description)
{
    DEBUG("device added: ", description.devNode());
    try {
        auto device = Device(description.devNode());
        auto manager = std::make_unique<DeviceManager>(
            device::Description(description), std::move(device), m_configuration, m_context
        );
        m_fileWatcher.subscribe(description.devNode(), FileWatcher::event::Attrib,
                                onFileWatchEvent, manager.get());
        emit deviceManagerAdded(*manager);

        auto dit = m_configuration.devices().find(manager->serial());
        INFO("opened device ", description.devNode(),
             ": serial ", manager->serial(),
             " [", dit != m_configuration.devices().end() ? dit->second : std::string(), ']',
             ", model ", manager->device().model(),
             " firmware ", manager->device().firmware(),
             ", <", manager->device().name(), ">");

        m_devices[description.devPath()] = std::move(manager);

    } catch (Device::error & error) {
        // Suppress hid version error, it just means it's not the kind of device we want
        if (error.code() != KEYLEDS_ERROR_HIDNOPP &&
            error.code() != KEYLEDS_ERROR_HIDVERSION) {
            ERROR("not opening device ", description.devNode(), ": ", error.what());
        }
    }
}

void Service::onDeviceRemoved(const device::Description & description)
{
    auto it = m_devices.find(description.devPath());
    if (it != m_devices.end()) {
        auto manager = std::move(it->second);
        m_devices.erase(it);

        INFO("removing device ", manager->serial());
        m_fileWatcher.unsubscribe(onFileWatchEvent, manager.get());
        emit deviceManagerRemoved(*manager);

        if (m_devices.empty() && m_configuration.autoQuit()) {
            QCoreApplication::quit();
        }
    }
}

void Service::onFileWatchEvent(void * managerPtr, FileWatcher::event, uint32_t, std::string)
{
    auto manager = static_cast<DeviceManager *>(managerPtr);
    int result = access(manager->device().path().c_str(), R_OK | W_OK);
    manager->setPaused(result != 0);
}
