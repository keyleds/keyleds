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
/** @file
 * @brief C++ wrapper for libudev
 *
 * This wrapper presents a C++ interface for reading device information and
 * getting notifications from libudev.
 */
#ifndef TOOLS_DEVICE_WATCHER_H_20E285D9
#define TOOLS_DEVICE_WATCHER_H_20E285D9

/****************************************************************************/

#include <QObject>
#include <memory>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

class QSocketNotifier;
struct udev;
struct udev_monitor;
struct udev_enumerate;
struct udev_device;

/* We use unique_ptr to get automatic disposal of libudev structures
 * Declare the custom deleters here to ensure they are always used
 * Implement them in cxx file to prevent leakage of libudev.h into other modules
 */
namespace std {
    template<> struct default_delete<struct udev> { void operator()(struct udev *) const; };
    template<> struct default_delete<struct udev_monitor> { void operator()(struct udev_monitor *) const; };
    template<> struct default_delete<struct udev_enumerate> { void operator()(struct udev_enumerate *) const; };
    template<> struct default_delete<struct udev_device> { void operator()(struct udev_device *) const; };
}

/****************************************************************************/

namespace device {

class Error : public std::runtime_error
{
public:
    Error(const std::string & what) : std::runtime_error(what) {}
};

/** Device description
 *
 * Wraps a struct udev_device instance, which describes a single device.
 * Pre-loads all device properties and attributes for faster access, but at
 * the cost of heavier initializaiton.
 */
class Description final
{
public:
    using property_map = std::vector<std::pair<std::string, std::string>>;
    using tag_list = std::vector<std::string>;
    using attribute_map = std::vector<std::pair<std::string, std::string>>;
public:
                        Description(struct udev_device * device);
    explicit            Description(const Description & other);
                        Description(Description &&) = default;
    Description &       operator=(Description &&) = default;

    // Hierarchy navigation
    Description         parent() const;
    Description         parentWithType(const std::string & subsystem,
                                       const std::string & devtype) const;
    std::vector<Description> descendantsWithType(const std::string & subsystem) const;

    // Simple device property queries
    std::string         devPath() const;
    std::string         subsystem() const;
    std::string         devType() const;
    const std::string & sysPath() const { return m_sysPath; }
    std::string         sysName() const;
    std::string         sysNum() const;
    std::string         devNode() const;
    std::string         driver() const;
    bool                isInitialized() const;
    unsigned long long  seqNum() const;
    unsigned long long  usecSinceInitialized() const;

    // Structured device properties
    const property_map &    properties() const { return m_properties; };
    const tag_list &        tags() const { return m_tags; };
    const attribute_map &   attributes() const { return m_attributes; };

private:
    std::unique_ptr<struct udev_device> m_device;   ///< underlying libudev device instance
    std::string     m_sysPath;                      ///< path to device description - unique
    property_map    m_properties;                   ///< key-value map of libudev properties
    tag_list        m_tags;                         ///< string list of libudev tags
    attribute_map   m_attributes;                   ///< key-value map of libudev attributes
};

inline bool operator==(const Description & a, const Description & b)
    { return a.sysPath() == b.sysPath(); }
inline bool operator!=(const Description & a, const Description & b) { return !(a == b); }

/****************************************************************************/

/** Device watcher and enumerator
 *
 * Actively scans or passively monitors devices through libudev. Every device
 * addition or removal detected is run through filters and emits a signal
 * if it passes them.
 *
 * Scanning is incremental: first scan will emit a deviceAdded for all devices,
 * and subsequent scans will emit a combination of deviceAdded and deviceRemoved
 * signals for matching changes. When in active mode, changes are continuously
 * monitored and signals are emitted as changes happen.
 *
 * Deleting the watcher does not emit deviceRemoved signals.
 */
class DeviceWatcher : public QObject
{
    Q_OBJECT
private:
    using device_list = std::vector<Description>;
public:
                        DeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
                        ~DeviceWatcher() override;

    void                scan();                 ///< Rescans system's devices actively
    void                setActive(bool active); ///< Enables listening for system notifications

signals:
    void                deviceAdded(const device::Description &);
    void                deviceRemoved(const device::Description &);

protected:
    virtual void        setupEnumerator(struct udev_enumerate & enumerator) const;
    virtual void        setupMonitor(struct udev_monitor & monitor) const;
    virtual bool        isVisible(const Description & dev) const;

private:
    /// Invoked whenever system notifications from udev become available
    void                onMonitorReady(int socket);

private:
    bool                                    m_active;       ///< If set, the watcher is monitoring
                                                            ///  device changes
    std::unique_ptr<struct udev>            m_udev;         ///< Connection to udev, or nullptr
    std::unique_ptr<struct udev_monitor>    m_monitor;      ///< Monitoring endpoint, or nullptr
    std::unique_ptr<QSocketNotifier>        m_udevNotifier; ///< Connection monitor for event loop
    device_list                             m_known;        ///< List of device descriptions
};

/****************************************************************************/

/** Simple filter-based device watcher
 *
 * A device watcher that filters devices based on simple rules. All rules must
 * pass for a device to be matched. Rules will not update while the watcher
 * is active.
 */
class FilteredDeviceWatcher : public DeviceWatcher
{
    Q_OBJECT
public:
            FilteredDeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);

    void    setSubsystem(std::string val);
    void    setDevType(std::string val);
    void    addProperty(const std::string & key, std::string val);
    void    addTag(std::string val);
    void    addAttribute(const std::string & key, std::string val);

protected:
    void    setupEnumerator(struct udev_enumerate & enumerator) const override;
    void    setupMonitor(struct udev_monitor & monitor) const override;
    bool    isVisible(const Description & dev) const override;

private:
    std::string                 m_matchSubsystem;
    std::string                 m_matchDevType;
    Description::property_map   m_matchProperties;
    Description::tag_list       m_matchTags;
    Description::attribute_map  m_matchAttributes;
};

/****************************************************************************/

};

#endif
