#ifndef KEYLEDSD_KEYLEDSSERVICE
#define KEYLEDSD_KEYLEDSSERVICE

#include <QObject>
#include <memory>
#include <string>
#include <unordered_map>
#include "config.h"
#include "keyledsd/DeviceManager.h"
#include "tools/DeviceWatcher.h"
#include "tools/SessionWatcher.h"

namespace keyleds {

class Configuration;

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
    bool                active() const { return m_active; }
    const device_map &  devices() const { return m_devices; }

public slots:
    void                init();
    void                setActive(bool val);

signals:
    void                deviceManagerAdded(keyleds::DeviceManager &);
    void                deviceManagerRemoved(keyleds::DeviceManager &);

private slots:
    void                onDeviceAdded(const device::Description &);
    void                onDeviceRemoved(const device::Description &);
    void                onDeviceLoopFinished();

private:
    Configuration &     m_configuration;
    bool                m_active;
    device_map          m_devices;

    DeviceWatcher       m_deviceWatcher;
    SessionWatcher      m_sessionWatcher;
};

};

#endif
