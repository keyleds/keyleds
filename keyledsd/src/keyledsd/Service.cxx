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
#include "keyledsd/Service.h"

#include <QCoreApplication>
#include <cassert>
#include <functional>
#include <sstream>
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/DisplayManager.h"
#include "tools/XWindow.h"
#include "keyleds.h"
#include "logging.h"

LOGGING("service");

using keyleds::Service;

/****************************************************************************/

static void merge(std::vector<std::pair<std::string, std::string>> & lhs,
                  const std::vector<std::pair<std::string, std::string>> & rhs)
{
    for (auto & newItem : rhs) {
        auto it = std::find_if(
            lhs.begin(), lhs.end(),
            [&newItem](const auto & item) { return item.first == newItem.first; }
        );
        if (it != lhs.end()) {
            // Key exists
            if (newItem.second.empty()) {
                // Value is empty, destroy key
                std::iter_swap(it, lhs.end() - 1);
                lhs.pop_back();
            } else {
                // Update with new value
                it->second = newItem.second;
            }
        } else if (!newItem.second.empty()) {
            // Create key
            lhs.emplace_back(newItem);
        }
    }
}

static std::string to_string(const std::vector<std::pair<std::string, std::string>> & val)
{
    std::ostringstream out;
    bool first = true;
    out <<'(';
    for (const auto & entry : val) {
        if (!first) { out <<", "; }
        first = false;
        out <<entry.first <<'=' <<entry.second;
    }
    out <<')';
    return out.str();
}

/****************************************************************************/

Service::Service(EffectManager & effectManager,
                 std::unique_ptr<Configuration> configuration, QObject * parent)
    : QObject(parent),
      m_effectManager(effectManager),
      m_configuration(nullptr),
      m_autoQuit(false),
      m_active(false),
      m_deviceWatcher(nullptr)
{
    QObject::connect(&m_deviceWatcher, &DeviceWatcher::deviceAdded,
                     this, &Service::onDeviceAdded);
    QObject::connect(&m_deviceWatcher, &DeviceWatcher::deviceRemoved,
                     this, &Service::onDeviceRemoved);
    setConfiguration(std::move(configuration));
    DEBUG("created");
}

Service::~Service()
{
    setActive(false);
    m_devices.clear();
}

void Service::init()
{
    try {
        auto display = std::make_unique<xlib::Display>();
        onDisplayAdded(display);
    } catch (xlib::Error & err) {
        CRITICAL("X display initialization failed: ", err.what());
        QCoreApplication::quit();
        return;
    }
    setActive(true);
}

/****************************************************************************/

void Service::setConfiguration(std::unique_ptr<Configuration> config)
{
    using std::swap;
    m_fileWatcherSub = FileWatcher::subscription(); // destroy it so it isn't reused

    // old configuration must not be destroyed until propagation is complete
    swap(m_configuration, config);

    // Propagate configuration
    for (auto & device : m_devices) { device->setConfiguration(m_configuration.get()); }
    setContext({}); // force context reloading without changing it

    // Setup configuration file watch
    if (!m_configuration->path().empty()) {
        m_fileWatcherSub = m_fileWatcher.subscribe(
            m_configuration->path(), FileWatcher::event::CloseWrite,
            std::bind(&Service::onConfigurationFileChanged, this, std::placeholders::_1)
        );
    }
}

void Service::setAutoQuit(bool val)
{
    m_autoQuit = val;
}

void Service::setActive(bool active)
{
    VERBOSE("switching to ", active ? "active" : "inactive", " mode");
    m_deviceWatcher.setActive(active);
    m_active = active;
}

void Service::setContext(const string_map & context)
{
    merge(m_context, context);
    VERBOSE("setContext ", ::to_string(m_context));
    for (auto & device : m_devices) { device->setContext(m_context); }
}

void Service::handleGenericEvent(const string_map & context)
{
    for (auto & device : m_devices) { device->handleGenericEvent(context); }
}

void Service::handleKeyEvent(const std::string & devNode, int key, bool press)
{
    for (auto & device : m_devices) {
        const auto & evDevs = device->eventDevices();
        if (std::find(evDevs.begin(), evDevs.end(), devNode) != evDevs.end()) {
            device->handleKeyEvent(key, press);
            break;
        }
    }
}

/****************************************************************************/

void Service::onConfigurationFileChanged(FileWatcher::event event)
{
    INFO("reloading ", m_configuration->path());

    std::unique_ptr<Configuration> conf;
    try {
        conf = Configuration::loadFile(m_configuration->path());
    } catch (std::exception & error) {
        CRITICAL("reloading failed: ", error.what());
    }
    if (conf != nullptr) {
        setConfiguration(std::move(conf));
        return; // setConfiguration reloads the watch unconditionally
    }

    if ((event & FileWatcher::event::Ignored) != 0) {
        // Happens when editors swap in the configuration file instead of rewriting it
        m_fileWatcherSub = m_fileWatcher.subscribe(
            m_configuration->path(), FileWatcher::event::CloseWrite,
            std::bind(&Service::onConfigurationFileChanged, this, std::placeholders::_1)
        );
    }
}

void Service::onDeviceAdded(const ::device::Description & description)
{
    VERBOSE("device added: ", description.devNode());
    try {
        auto device = Device::open(description.devNode());
        auto manager = std::make_unique<DeviceManager>(
            m_effectManager, m_fileWatcher,
            description, std::move(device), m_configuration.get()
        );
        manager->setContext(m_context);

        emit deviceManagerAdded(*manager);

        INFO("opened device ", description.devNode(),
             " [", manager->name(), ']',
             ", model ", manager->device().model(),
             " firmware ", manager->device().firmware(),
             ", <", manager->device().name(), ">");

        manager->setPaused(false);
        m_devices.emplace_back(std::move(manager));

    } catch (Device::error & error) {
        // Suppress hid version error, it just means it's not the kind of device we want
        if (error.expected()) {
            VERBOSE("not opening device ", description.devNode(), ": ", error.what());
        } else {
            ERROR("not opening device ", description.devNode(), ": ", error.what());
        }
    }
}

void Service::onDeviceRemoved(const ::device::Description & description)
{
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
                           [&description](const auto & device) {
                               return device->sysPath() == description.sysPath();
                           });
    if (it != m_devices.end()) {
        std::unique_ptr<DeviceManager> manager = std::move(*it);
        std::iter_swap(it, m_devices.end() - 1);
        m_devices.pop_back();

        INFO("removing device ", manager->serial());

        emit deviceManagerRemoved(*manager);

        if (m_devices.empty() && m_autoQuit) {
            QCoreApplication::quit();
        }
    }
}

/****************************************************************************/

void Service::onDisplayAdded(std::unique_ptr<xlib::Display> & display)
{
    INFO("connected to display ", display->name());
    auto displayManager = std::make_unique<DisplayManager>(std::move(display));
    QObject::connect(displayManager.get(), &DisplayManager::contextChanged,
                     this, &Service::setContext);
    QObject::connect(displayManager.get(), &DisplayManager::keyEventReceived,
                     this, &Service::handleKeyEvent);
    displayManager->scanDevices();
    setContext(displayManager->currentContext());

    m_displays.emplace_back(std::move(displayManager));
}

void Service::onDisplayRemoved()
{
    assert(m_displays.size() == 1);
    INFO("disconnecting from display ", m_displays.front()->display().name());
    m_displays.clear();
}
