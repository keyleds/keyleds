#ifndef KEYLEDSD_DEVICEMANAGER_H
#define KEYLEDSD_DEVICEMANAGER_H

#include <QObject>
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/RenderLoop.h"
#include "tools/DeviceWatcher.h"
#include <list>
#include <memory>
#include <string>
struct KeyledsdTarget;

namespace keyleds {

class Layout;
class RenderLoop;

class DeviceManager : public QObject
{
    Q_OBJECT
public:
    typedef std::unique_ptr<Layout> layout_ptr;
    typedef std::unique_ptr<RenderLoop> renderloop_ptr;
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
    static std::string      loadSerial(device::Description &);
    static std::string      layoutName(const Device &);
    layout_ptr              loadLayout(const Device &);
    RenderLoop::renderer_list loadRenderers(const Configuration::Stack &);

private:
    std::string             m_serial;
    device::Description     m_description;
    Device                  m_device;
    layout_ptr              m_layout;
    renderloop_ptr          m_renderLoop;

    const Configuration &   m_configuration;
};

};

#endif
