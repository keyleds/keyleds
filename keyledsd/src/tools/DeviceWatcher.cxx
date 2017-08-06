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
#include <QtCore>
#include <libudev.h>
#include <cassert>
#include <stdexcept>
#include <string>
#include "tools/DeviceWatcher.h"

using device::Description;
using device::DeviceWatcher;
using device::FilteredDeviceWatcher;

void std::default_delete<struct udev>::operator()(struct udev *ptr) const
    { udev_unref(ptr); }
void std::default_delete<struct udev_monitor>::operator()(struct udev_monitor *ptr) const
    { udev_monitor_unref(ptr); }
void std::default_delete<struct udev_enumerate>::operator()(struct udev_enumerate *ptr) const
    { udev_enumerate_unref(ptr); }
void std::default_delete<struct udev_device>::operator()(struct udev_device *ptr) const
    { udev_device_unref(ptr); }

/****************************************************************************/

Description::Description(struct udev_device * device)
    : m_device(udev_device_ref(device))
{
    struct udev_list_entry * first, * current;
    assert(device != nullptr);

    first = udev_device_get_properties_list_entry(device);
    udev_list_entry_foreach(current, first) {
        std::string key(udev_list_entry_get_name(current));
        std::string val(udev_list_entry_get_value(current));
        m_properties[key] = val;
    }

    first = udev_device_get_tags_list_entry(device);
    udev_list_entry_foreach(current, first) {
        m_tags.push_back(udev_list_entry_get_name(current));
    }
    m_tags.shrink_to_fit();

    first = udev_device_get_sysattr_list_entry(device);
    udev_list_entry_foreach(current, first) {
        std::string key(udev_list_entry_get_name(current));
        const char * val = udev_device_get_sysattr_value(device, key.c_str());
        if (val != nullptr) {
            m_attributes[key] = val;
        }
    }
}

Description::Description(const Description & other)
    : m_device(udev_device_ref(other.m_device.get()))
{
    m_properties = other.m_properties;
    m_tags = other.m_tags;
    m_attributes = other.m_attributes;
}

Description Description::parent() const
{
    auto dev = udev_device_get_parent(m_device.get());  // unowned
    if (dev == nullptr) {
        throw std::logic_error("Device " + sysPath() + " has no parent");
    }
    return Description(dev);
}

Description Description::parentWithType(const std::string & subsystem,
                                        const std::string & devtype) const
{
    auto dev = udev_device_get_parent_with_subsystem_devtype(
        m_device.get(),
        subsystem.empty() ? nullptr : subsystem.c_str(),
        devtype.empty() ? nullptr : devtype.c_str()
    );
    if (dev == nullptr) {
        throw std::logic_error("No parent with specified type for device " + sysPath());
    }
    return Description(dev);
}

std::vector<Description> Description::descendantsWithType(const std::string & subsystem) const
{
    std::vector<Description> result;

    auto udev = udev_device_get_udev(m_device.get());
    auto enumerator = std::unique_ptr<struct udev_enumerate>(udev_enumerate_new(udev));

    udev_enumerate_add_match_parent(enumerator.get(), m_device.get());
    udev_enumerate_add_match_subsystem(enumerator.get(), subsystem.c_str());

    if (udev_enumerate_scan_devices(enumerator.get()) < 0) {
        return result;
    }

    struct udev_list_entry * first, * current;
    first = udev_enumerate_get_list_entry(enumerator.get());

    udev_list_entry_foreach(current, first) {
        const char * syspath = udev_list_entry_get_name(current);
        auto device = std::unique_ptr<struct udev_device>(
            udev_device_new_from_syspath(udev, syspath)
        );
        if (device != nullptr) {
            result.emplace_back(device.get());
        }
    }

    return result;
}

static std::string safe(const char * s) { return s == nullptr ? std::string() : std::string(s); }

std::string Description::devPath() const { return safe(udev_device_get_devpath(m_device.get())); }
std::string Description::subsystem() const { return safe(udev_device_get_subsystem(m_device.get())); }
std::string Description::devType() const { return safe(udev_device_get_devtype(m_device.get())); }
std::string Description::sysPath() const { return safe(udev_device_get_syspath(m_device.get())); }
std::string Description::sysName() const { return safe(udev_device_get_sysname(m_device.get())); }
std::string Description::sysNum() const { return safe(udev_device_get_sysnum(m_device.get())); }
std::string Description::devNode() const { return safe(udev_device_get_devnode(m_device.get())); }
std::string Description::driver() const { return safe(udev_device_get_driver(m_device.get())); }
bool Description::isInitialized() const { return udev_device_get_is_initialized(m_device.get()); }

unsigned long long Description::usecSinceInitialized() const
{
    return udev_device_get_usec_since_initialized(m_device.get());
}

/****************************************************************************/

DeviceWatcher::DeviceWatcher(struct udev * udev, QObject *parent)
    : QObject(parent),
      m_active(false),
      m_udev(udev == nullptr ? udev_new() : udev_ref(udev))
{
    if (m_udev == nullptr) {
        throw Error("udev initialization failed");
    }
}

DeviceWatcher::~DeviceWatcher()
{
    /* empty */
}

void DeviceWatcher::scan()
{
    auto enumerator = std::unique_ptr<struct udev_enumerate>(udev_enumerate_new(m_udev.get()));

    setupEnumerator(*enumerator);

    if (udev_enumerate_scan_devices(enumerator.get()) < 0) {
        throw Error("udev device scan failed");
    }

    device_map result;

    struct udev_list_entry * first, * current;
    first = udev_enumerate_get_list_entry(enumerator.get());

    udev_list_entry_foreach(current, first) {
        const char * syspath = udev_list_entry_get_name(current);
        auto it = m_known.find(syspath);
        if (it != m_known.end()) {
            result.emplace(std::make_pair(syspath, std::move(it->second)));
        } else {
            auto device = std::unique_ptr<struct udev_device>(
                udev_device_new_from_syspath(m_udev.get(), syspath)
            );
            if (device != nullptr) {
                auto description = Description(device.get());
                if (isVisible(description)) {
                    result.emplace(std::make_pair(syspath, std::move(description)));
                }
            }
        }
    }

    for (const auto & entry : m_known) {
        if (result.find(entry.first) == result.end()) { emit deviceRemoved(entry.second); }
    }
    m_known.swap(result);
    for (const auto & entry : m_known) {
        if (result.find(entry.first) == result.end()) { emit deviceAdded(entry.second); }
    }
}

void DeviceWatcher::setActive(bool active)
{
    if (active == m_active) { return; }
    if (active) {
        m_monitor.reset(udev_monitor_new_from_netlink(m_udev.get(), "udev"));
        if (m_monitor == nullptr) {
            throw Error("udev notification initialization failed");
        }

        setupMonitor(*m_monitor);

        udev_monitor_enable_receiving(m_monitor.get());
        int fd = udev_monitor_get_fd(m_monitor.get());
        m_udevNotifier.reset(new QSocketNotifier(fd, QSocketNotifier::Read));

        QObject::connect(m_udevNotifier.get(), SIGNAL(activated(int)),
                         this, SLOT(onMonitorReady(int)));
        scan();

    } else {
        m_udevNotifier.reset(nullptr);
        m_monitor.reset(nullptr);
    }
    m_active = active;
}

void DeviceWatcher::onMonitorReady(int)
{
    auto device = std::unique_ptr<struct udev_device>(udev_monitor_receive_device(m_monitor.get()));
    if (device == nullptr) {
        throw Error("failed to read notification details from udev");
    }

    const char * syspath = udev_device_get_syspath(device.get());
    auto action = std::string(udev_device_get_action(device.get()));
    if (action == "add") {
        auto description = Description(device.get());
        if (isVisible(description)) {
            if (m_known.find(syspath) == m_known.end()) {
                auto result = m_known.emplace(std::make_pair(syspath, std::move(description)));
                emit deviceAdded(result.first->second);
            }
        }
    } else if (action == "remove") {
        auto it = m_known.find(syspath);
        if (it != m_known.end()) {
            emit deviceRemoved(it->second);
            m_known.erase(it);
        }
    }
}

void DeviceWatcher::setupEnumerator(struct udev_enumerate &) const { return; }
void DeviceWatcher::setupMonitor(struct udev_monitor &) const { return; }
bool DeviceWatcher::isVisible(const Description &) const { return true; }

/****************************************************************************/

FilteredDeviceWatcher::FilteredDeviceWatcher(struct udev * udev, QObject *parent)
 : DeviceWatcher(udev, parent)
{}

void FilteredDeviceWatcher::setSubsystem(std::string val)
{
    m_matchSubsystem = std::move(val);
}

void FilteredDeviceWatcher::setDevType(std::string val)
{
    m_matchDevType = std::move(val);
}

void FilteredDeviceWatcher::addProperty(const std::string & key, std::string val)
{
    m_matchProperties[key] = std::move(val);
}

void FilteredDeviceWatcher::addTag(std::string val)
{
    m_matchTags.push_back(std::move(val));
}

void FilteredDeviceWatcher::addAttribute(const std::string & key, std::string val)
{
    m_matchAttributes[key] = std::move(val);
}

void FilteredDeviceWatcher::setupEnumerator(struct udev_enumerate & enumerator) const
{
    if (!m_matchSubsystem.empty()) {
        udev_enumerate_add_match_subsystem(&enumerator, m_matchSubsystem.c_str());
    }
    for (const auto & entry : m_matchAttributes) {
        udev_enumerate_add_match_sysattr(&enumerator, entry.first.c_str(), entry.second.c_str());
    }
    for (const auto & entry : m_matchProperties) {
        udev_enumerate_add_match_property(&enumerator, entry.first.c_str(), entry.second.c_str());
    }
    for (const auto & entry : m_matchTags) {
        udev_enumerate_add_match_tag(&enumerator, entry.c_str());
    }
}

void FilteredDeviceWatcher::setupMonitor(struct udev_monitor & monitor) const
{
    if (!m_matchSubsystem.empty()) {
        udev_monitor_filter_add_match_subsystem_devtype(
            &monitor,
            m_matchSubsystem.c_str(),
            m_matchDevType.empty() ? nullptr : m_matchDevType.c_str()
        );
    }
    for (const auto & tag : m_matchTags) {
        udev_monitor_filter_add_match_tag(&monitor, tag.c_str());
    }
}

bool FilteredDeviceWatcher::isVisible(const Description & dev) const
{
    // Check attributes that either monitor or enumerate miss
    if (!m_matchDevType.empty() && m_matchDevType != dev.devType()) {
        return false;
    }

    for (const auto & entry : m_matchAttributes) {
        auto it = dev.attributes().find(entry.first);
        if (it == dev.attributes().end() || entry.second != it->second) { return false; }
    }
    for (const auto & entry : m_matchProperties) {
        auto it = dev.properties().find(entry.first);
        if (it == dev.properties().end() || entry.second != it->second) { return false; }
    }
    return true;
}
