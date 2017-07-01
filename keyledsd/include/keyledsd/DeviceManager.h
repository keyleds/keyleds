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

class Context;
class Layout;

class DeviceManager : public QObject
{
    Q_OBJECT
private:
    class LoadedProfile final
    {
    public:
        typedef std::vector<std::unique_ptr<Renderer>> renderer_list;
    public:
                            LoadedProfile(renderer_list && renderers)
                             : m_renderers(std::forward<renderer_list>(renderers)) {}
                            LoadedProfile(LoadedProfile &&) = default;
        RenderLoop::renderer_list renderers() const;
    private:
        renderer_list   m_renderers;
    };

public:
    typedef std::unique_ptr<Layout> layout_ptr;
    typedef std::vector<std::string> dev_list;
public:
                            DeviceManager(const device::Description &,
                                          Device &&,
                                          const Configuration &,
                                          const Context &,
                                          QObject *parent = 0);
    virtual                 ~DeviceManager();

    const std::string &     serial() const { return m_serial; }
    const dev_list &        eventDevices() const { return m_eventDevices; }
    const Device &          device() const { return m_device; }
          bool              hasLayout() const { return m_layout != nullptr; }
    const Layout &          layout() const { return *m_layout; }

    Device::key_indices     resolveKeyName(const std::string &) const;

          bool              paused() const { return m_renderLoop.paused(); }

public slots:
    void                    setContext(const Context &);
    void                    setPaused(bool);
    void                    reloadConfiguration();

private:
    static std::string      getSerial(const device::Description &);
    static dev_list         findEventDevices(const device::Description &);
    static std::string      layoutName(const Device &);
    layout_ptr              loadLayout(const Device &);
    RenderLoop::renderer_list loadRenderers(const Context &);
    LoadedProfile &         getProfile(const Configuration::Profile &);

private:
    const Configuration &   m_configuration;

    std::string             m_serial;
    dev_list                m_eventDevices;
    Device                  m_device;
    layout_ptr              m_layout;

    std::unordered_map<Configuration::Profile::id_type, LoadedProfile> m_profiles;
    RenderLoop              m_renderLoop;
};

};

#endif
