#include <memory>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"

using keyleds::RenderTarget;
using keyleds::Renderer;
using keyleds::IRendererPlugin;
using keyleds::RendererPluginManager;
using keyleds::RendererPlugin;

class FillRenderer final : public Renderer
{
    const keyleds::Device & m_device;
public:
    FillRenderer(const keyleds::DeviceManager & manager,
                 const keyleds::Configuration::Plugin &)
        : m_device(manager.device())
    {
        m_increase = true;
        m_red = 0;
        m_green = 224;
        m_blue = 255;
    }

    bool isFilter() const override { return true; }

    void render(unsigned long, RenderTarget & target) override
    {
        if (m_increase) {
            if (++m_red == 255) { m_increase = false; }
        } else {
            if (--m_red == 0) { m_increase = true; }
        }
        for (auto it = target.keys.begin(); it != target.keys.end(); ++it) {
            it->red = m_red;
            it->green = m_green;
            it->blue = m_blue;
            it->alpha = 255;
        }
    }

private:
    bool    m_increase;
    uint8_t m_red;
    uint8_t m_green;
    uint8_t m_blue;
};

static const keyleds::RendererPlugin<FillRenderer> maker("fill");
