#include <QtCore>
#include <assert.h>
#include <libudev.h>
#include <stdexcept>
#include "tools/DeviceWatcher.h"

using device::Description;
using device::DeviceWatcher;
using device::FilteredDeviceWatcher;

typedef std::unique_ptr<struct udev_enumerate,struct udev_enumerate*(*)(struct udev_enumerate*)> udev_enumerate_ptr;

/****************************************************************************/

Description::Description(struct udev_device * device)
    : m_device(nullptr, udev_device_unref)
{
    struct udev_list_entry * first, * current;
    assert(device != NULL);

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
        if (val != NULL) {
            m_attributes[key] = val;
        }
    }
    m_device.reset(udev_device_ref(device));
}

Description::Description(const Description & other)
    : m_device(nullptr, udev_device_unref)
{
    m_properties = other.m_properties;
    m_tags = other.m_tags;
    m_attributes = other.m_attributes;
    m_device.reset(udev_device_ref(other.m_device.get()));
}

Description Description::parent() const
{
    struct udev_device * dev = udev_device_get_parent(m_device.get());  // unowned
    if (dev == NULL) {
        throw std::logic_error("Device " + sysPath() + " has no parent");
    }
    return Description(dev);
}

Description Description::parentWithType(const std::string & subsystem,
                                        const std::string & devtype) const
{
    struct udev_device * dev;   // unowned
    dev = udev_device_get_parent_with_subsystem_devtype(
        m_device.get(),
        subsystem.empty() ? NULL : subsystem.c_str(),
        devtype.empty() ? NULL : devtype.c_str()
    );
    if (dev == NULL) {
        throw std::logic_error("No parent with specified type for device " + sysPath());
    }
    return Description(dev);
}

std::vector<Description> Description::descendantsWithType(const std::string & subsystem) const
{
    std::vector<Description> result;

    auto udev = udev_device_get_udev(m_device.get());
    udev_enumerate_ptr enumerator(udev_enumerate_new(udev), udev_enumerate_unref);

    udev_enumerate_add_match_parent(enumerator.get(), m_device.get());
    udev_enumerate_add_match_subsystem(enumerator.get(), subsystem.c_str());

    if (udev_enumerate_scan_devices(enumerator.get()) < 0) {
        return result;
    }

    struct udev_list_entry * first, * current;
    first = udev_enumerate_get_list_entry(enumerator.get());

    udev_list_entry_foreach(current, first) {
        const char * syspath = udev_list_entry_get_name(current);
        auto device = udev_device_ptr(
            udev_device_new_from_syspath(udev, syspath),
            udev_device_unref
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
      m_udev(nullptr, udev_unref),
      m_monitor(nullptr, udev_monitor_unref)
{
    m_active = false;
    m_udev.reset(udev == nullptr ? udev_new() : udev_ref(udev));
}

DeviceWatcher::~DeviceWatcher()
{
    /* empty */
}

void DeviceWatcher::scan()
{
    udev_enumerate_ptr enumerator(udev_enumerate_new(m_udev.get()), udev_enumerate_unref);

    setupEnumerator(*enumerator);

    if (udev_enumerate_scan_devices(enumerator.get()) < 0) {
        //TODO error handling
        return;
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
            Description::udev_device_ptr device(nullptr, udev_device_unref);
            device.reset(udev_device_new_from_syspath(m_udev.get(), syspath));
            if (device != nullptr) {
                auto description = Description(device.get());
                if (isVisible(description)) {
                    result.emplace(std::make_pair(syspath, std::move(description)));
                }
            }
        }
    }

    for (auto i = m_known.begin(); i != m_known.end(); ++i) {
        if (result.find(i->first) == result.end()) { emit deviceRemoved(i->second); }
    }
    m_known.swap(result);
    for (auto i = m_known.begin(); i != m_known.end(); ++i) {
        if (result.find(i->first) == result.end()) { emit deviceAdded(i->second); }
    }
}

void DeviceWatcher::setActive(bool active)
{
    if (active == m_active) { return; }
    if (active) {
        m_monitor.reset(udev_monitor_new_from_netlink(m_udev.get(), "udev"));
        if (m_monitor == nullptr) {
            //TODO handle error
            return;
        }

        setupMonitor(*m_monitor);

        udev_monitor_enable_receiving(m_monitor.get());
        int fd = udev_monitor_get_fd(m_monitor.get());
        m_udevNotifier.reset(new QSocketNotifier(fd, QSocketNotifier::Read, this));

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
    Description::udev_device_ptr device(udev_monitor_receive_device(m_monitor.get()),
                                        udev_device_unref);
    if (device == nullptr) {
        //TODO handle error
        return;
    }

    const char * syspath = udev_device_get_syspath(device.get());
    QString action = udev_device_get_action(device.get());
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

void FilteredDeviceWatcher::setupEnumerator(struct udev_enumerate & enumerator) const
{
    if (!m_matchSubsystem.empty()) {
        udev_enumerate_add_match_subsystem(&enumerator, m_matchSubsystem.c_str());
    }
    for (auto it = m_matchAttributes.begin(); it != m_matchAttributes.end(); ++it) {
        udev_enumerate_add_match_sysattr(&enumerator, it->first.c_str(), it->second.c_str());
    }
    for (auto it = m_matchProperties.begin(); it != m_matchProperties.end(); ++it) {
        udev_enumerate_add_match_property(&enumerator, it->first.c_str(), it->second.c_str());
    }
    for (auto it = m_matchTags.begin(); it != m_matchTags.end(); ++it) {
        udev_enumerate_add_match_tag(&enumerator, it->c_str());
    }
}

void FilteredDeviceWatcher::setupMonitor(struct udev_monitor & monitor) const
{
    if (!m_matchSubsystem.empty()) {
        udev_monitor_filter_add_match_subsystem_devtype(
            &monitor,
            m_matchSubsystem.c_str(),
            m_matchDevType.empty() ? NULL : m_matchDevType.c_str()
        );
    }
    for (auto it = m_matchTags.begin(); it != m_matchTags.end(); ++it) {
        udev_monitor_filter_add_match_tag(&monitor, it->c_str());
    }
}

bool FilteredDeviceWatcher::isVisible(const Description & dev) const
{
    // Check attributes that either monitor or enumerate miss
    if (!m_matchDevType.empty() && m_matchDevType != dev.devType()) {
        return false;
    }

    for (auto it = m_matchAttributes.begin(); it != m_matchAttributes.end(); ++it) {
        auto devit = dev.attributes().find(it->first);
        if (devit == dev.attributes().end() || it->second != devit->second) { return false; }
    }
    for (auto it = m_matchProperties.begin(); it != m_matchProperties.end(); ++it) {
        auto devit = dev.properties().find(it->first);
        if (devit == dev.properties().end() || it->second != devit->second) { return false; }
    }
    return true;
}
