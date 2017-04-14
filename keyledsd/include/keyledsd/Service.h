#ifndef KEYLEDSD_KEYLEDSSERVICE
#define KEYLEDSD_KEYLEDSSERVICE

#include <QObject>
#include <string>
#include <vector>
#include "config.h"
#include "keyledsd/Device.h"
#include "tools/SessionWatcher.h"

namespace keyleds {

class Configuration;

class Service : public QObject
{
    Q_OBJECT
    Q_PROPERTY(bool active READ active WRITE setActive);
public:
    typedef std::map<std::string, Device> device_map;
public:
                Service(Configuration & configuration, QObject *parent = 0);
                Service(const Service &) = delete;

    bool        active() const { return m_active; }

public slots:
    void        init();
    void        setActive(bool val);

private slots:
    void        onDeviceAdded(const device::DeviceDescription &);
    void        onDeviceRemoved(const device::DeviceDescription &);

private:
    Configuration &         m_configuration;
    bool                    m_active;
    device_map              m_devices;

    DeviceWatcher           m_deviceWatcher;
    SessionWatcher          m_sessionWatcher;
};

};

#endif
