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
class IRenderer;
class Service;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<Layout> layout_ptr;
    typedef std::unique_ptr<IRenderer> renderer_ptr;
    typedef std::list<renderer_ptr> renderer_list;
public:
                            DeviceManager(device::Description && description,
                                          Device && device,
                                          const Configuration &,
                                          QObject *parent = 0);
    virtual                 ~DeviceManager();

    const std::string &     serial() const { return m_serial; }
    const device::Description & description() const { return m_description; }
    const Device &          device() const { return m_device; }
          bool              hasLayout() const { return m_layout != nullptr; }
    const Layout &          layout() const { return *m_layout; }

private:
    static std::string      layoutName(const Device &);
    layout_ptr              loadLayout(const Device &);
    void                    loadRenderers();

private:
    std::string             m_serial;
    device::Description     m_description;
    Device                  m_device;
    layout_ptr              m_layout;
    renderer_list           m_renderers;

    const Configuration &   m_configuration;
    AnimationLoop           m_loop;
};

};

#endif
