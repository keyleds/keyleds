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
#include "keyledsd/service/Service.h"

#include "keyleds.h"
#include "keyledsd/device/Logitech.h"
#include "keyledsd/logging.h"
#include "keyledsd/service/Configuration.h"
#include "keyledsd/service/DeviceManager.h"
#include "keyledsd/service/DisplayManager.h"
#include "keyledsd/tools/XWindow.h"
#include <cassert>
#include <functional>
#include <optional>
#include <sstream>
#include <uv.h>

LOGGING("service");

using keyleds::service::Service;

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
                if (it != lhs.end() - 1) { *it = std::move(lhs.back()); }
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

Service::Service(EffectManager & effectManager, tools::FileWatcher & fileWatcher,
                 Configuration configuration, uv_loop_t & loop)
    : m_effectManager(effectManager),
      m_fileWatcher(fileWatcher),
      m_loop(loop),
      m_deviceWatcher(loop)
{
    using namespace std::placeholders;
    connect(m_deviceWatcher.deviceAdded, this, std::bind(&Service::onDeviceAdded, this, _1));
    connect(m_deviceWatcher.deviceRemoved, this, std::bind(&Service::onDeviceRemoved, this, _1));
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
        auto display = std::make_unique<tools::xlib::Display>();
        onDisplayAdded(display);
    } catch (tools::xlib::Error & err) {
        CRITICAL("X display initialization failed: ", err.what());
        uv_stop(&m_loop);
        return;
    }
    setActive(true);
}

/****************************************************************************/

void Service::setConfiguration(Configuration config)
{
    using std::swap;
    m_fileWatcherSub = FileWatcher::subscription(); // destroy it so it isn't reused

    // old configuration must not be destroyed until propagation is complete
    swap(m_configuration, config);

    // Propagate configuration
    for (auto & device : m_devices) { device->setConfiguration(&m_configuration); }
    setContext({}); // force context reloading without changing it

    // Setup configuration file watch
    if (!m_configuration.path.empty()) {
        m_fileWatcherSub = m_fileWatcher.subscribe(
            m_configuration.path, FileWatcher::Event::CloseWrite,
            std::bind(&Service::onConfigurationFileChanged, this, std::placeholders::_1)
        );
    }
}

void Service::setAutoQuit(bool val)
{
    m_autoQuit = val;
}

void Service::setActive(bool val)
{
    VERBOSE("switching to ", val ? "active" : "inactive", " mode");
    m_deviceWatcher.setActive(val);
    m_active = val;
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

void Service::onConfigurationFileChanged(FileWatcher::Event event)
{
    INFO("reloading ", m_configuration.path);

    auto conf = Configuration();
    try {
        conf = Configuration::loadFile(m_configuration.path);
    } catch (std::exception & error) {
        CRITICAL("reloading failed: ", error.what());
    }
    if (!conf.path.empty()) {
        setConfiguration(std::move(conf));
        return; // setConfiguration reloads the watch unconditionally
    }

    if ((event & FileWatcher::Event::Ignored) != 0) {
        // Happens when editors swap in the configuration file instead of rewriting it
        m_fileWatcherSub = m_fileWatcher.subscribe(
            m_configuration.path, FileWatcher::Event::CloseWrite,
            std::bind(&Service::onConfigurationFileChanged, this, std::placeholders::_1)
        );
    }
}

void Service::onDeviceAdded(const tools::device::Description & description)
{
    VERBOSE("device added: ", description.devNode());
    try {
        auto device = device::Logitech::open(description.devNode());
        auto manager = std::make_unique<DeviceManager>(
            m_effectManager, m_fileWatcher,
            description, std::move(device), &m_configuration
        );
        manager->setContext(m_context);

        deviceManagerAdded.emit(*manager);

        INFO("opened device ", description.devNode(),
             " [", manager->name(), ']',
             ", model ", manager->device().model(),
             " firmware ", manager->device().firmware(),
             ", <", manager->device().name(), ">");

        manager->setPaused(false);
        m_devices.emplace_back(std::move(manager));

    } catch (device::Device::error & error) {
        if (error.expected()) {
            VERBOSE("not opening device ", description.devNode(), ": ", error.what());
        } else {
            ERROR("not opening device ", description.devNode(), ": ", error.what());
        }
    }
}

void Service::onDeviceRemoved(const tools::device::Description & description)
{
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
                           [&description](const auto & device) {
                               return device->sysPath() == description.sysPath();
                           });
    if (it != m_devices.end()) {
        auto manager = std::move(*it);
        if (it != m_devices.end() - 1) { *it = std::move(m_devices.back()); }
        m_devices.pop_back();

        INFO("removing device ", manager->serial());

        deviceManagerRemoved.emit(*manager);

        if (m_devices.empty() && m_autoQuit) {
            uv_stop(&m_loop);
        }
    }
}

/****************************************************************************/

void Service::onDisplayAdded(std::unique_ptr<tools::xlib::Display> & display)
{
    INFO("connected to display ", display->name());
    auto displayManager = std::make_unique<DisplayManager>(std::move(display), m_loop);

    using namespace std::placeholders;
    connect(displayManager->contextChanged, this,
            std::bind(&Service::setContext, this, _1));
    connect(displayManager->keyEventReceived, this,
            std::bind(&Service::handleKeyEvent, this, _1, _2, _3));

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
