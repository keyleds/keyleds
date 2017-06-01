#ifndef TOOLS_DEVICE_WATCHER_H
#define TOOLS_DEVICE_WATCHER_H

/****************************************************************************/

#include <QObject>
#include <map>
#include <memory>
#include <string>
#include <unordered_map>
#include <vector>

class QSocketNotifier;
struct udev;
struct udev_monitor;
struct udev_enumerate;
struct udev_device;

/****************************************************************************/

namespace device {

class Description final
{
public:
    typedef struct udev_device * (*udev_device_deleter)(struct udev_device*);
    typedef std::unique_ptr<struct udev_device, udev_device_deleter> udev_device_ptr;
    typedef std::map<std::string, std::string> property_map;
    typedef std::vector<std::string>           tag_list;
    typedef std::map<std::string, std::string> attribute_map;
public:
                    Description(struct udev_device * device);
    explicit        Description(const Description & other);
                    Description(Description && other) = default;

    Description     parent() const;
    Description     parentWithType(const std::string & subsystem,
                                   const std::string & devtype) const;
    std::vector<Description> descendantsWithType(const std::string & subsystem) const;

    std::string     devPath() const;
    std::string     subsystem() const;
    std::string     devType() const;
    std::string     sysPath() const;
    std::string     sysName() const;
    std::string     sysNum() const;
    std::string     devNode() const;
    std::string     driver() const;
    bool            isInitialized() const;
    unsigned long long seqNum() const;
    unsigned long long usecSinceInitialized() const;

    const property_map &    properties() const { return m_properties; };
    const tag_list &        tags() const { return m_tags; };
    const attribute_map &   attributes() const { return m_attributes; };

private:
    udev_device_ptr m_device;
    property_map    m_properties;
    tag_list        m_tags;
    attribute_map   m_attributes;
};

/****************************************************************************/

class DeviceWatcher : public QObject
{
    Q_OBJECT
protected:
    typedef struct udev_monitor*(*udev_monitor_deleter)(struct udev_monitor*);
    typedef std::unique_ptr<struct udev_monitor, udev_monitor_deleter> udev_monitor_ptr;
    typedef struct udev*(*udev_deleter)(struct udev*);
    typedef std::unique_ptr<struct udev, udev_deleter> udev_ptr;

    typedef std::unordered_map<std::string, Description> device_map;
public:
                        DeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
                        ~DeviceWatcher();

public slots:
    void                scan();
    void                setActive(bool active=true);

signals:
    void                deviceAdded(const device::Description &);
    void                deviceRemoved(const device::Description &);

protected:
    virtual void        setupEnumerator(struct udev_enumerate & enumerator) const;
    virtual void        setupMonitor(struct udev_monitor & monitor) const;
    virtual bool        isVisible(const Description & dev) const;

private slots:
    void                onMonitorReady(int socket);

private:
    bool                m_active;
    udev_ptr            m_udev;
    udev_monitor_ptr    m_monitor;
    std::unique_ptr<QSocketNotifier> m_udevNotifier;
    device_map          m_known;
};

/****************************************************************************/

class FilteredDeviceWatcher : public DeviceWatcher
{
    Q_OBJECT
public:
            FilteredDeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr)
                : DeviceWatcher(udev, parent) {};

    void    setSubsystem(std::string val) { m_matchSubsystem = val; }
    void    setDevType(std::string val) { m_matchDevType = val; }
    void    addProperty(std::string key, std::string val)
                { m_matchProperties[key] = val; }
    void    addTag(std::string val) { m_matchTags.push_back(val); }
    void    addAttribute(std::string key, std::string val)
                { m_matchAttributes[key] = val; }

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
