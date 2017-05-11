#ifndef KEYLEDSD_DEVICEMANAGER_H
#define KEYLEDSD_DEVICEMANAGER_H

#include <QObject>
#include "keyledsd/Device.h"
#include "tools/AnimationLoop.h"
#include "tools/DeviceWatcher.h"
#include <memory>
#include <string>

namespace keyleds {

class Configuration;
class Layout;
class Service;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<Layout> layout_ptr;
public:
                            DeviceManager(Device && device,
                                          device::Description && description,
                                          const Configuration &,
                                          QObject *parent = 0);
    virtual                 ~DeviceManager();

    const device::Description & description() const { return m_description; }
    const Device &          device() const { return m_device; }
    const std::string &     serial() const { return m_serial; }
          bool              hasLayout() const { return m_layout != nullptr; }
    const Layout &          layout() const { return *m_layout; }

private:
    static std::string      layoutName(const Device &);
    layout_ptr              loadLayout(const Device &);

private:
    device::Description     m_description;
    Device                  m_device;
    std::string             m_serial;
    const Configuration &   m_configuration;
    layout_ptr              m_layout;
    AnimationLoop           m_loop;
};

};

#endif
