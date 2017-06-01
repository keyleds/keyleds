#include <memory>
#include "keyledsd/common.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"


class FillRenderer final : public keyleds::Renderer
{
    const keyleds::Device & m_device;
public:
    FillRenderer(const keyleds::DeviceManager & manager,
                 const keyleds::Configuration::Plugin & conf)
        : m_device(manager.device())
    {
        auto it = conf.items().find("color");
        if (it != conf.items().end()) {
            m_color = keyleds::RGBColor::parse(it->second);
        }
    }

    void render(unsigned long, keyleds::RenderTarget & target) override
    {
        for (auto it = target.keys.begin(); it != target.keys.end(); ++it) {
            it->red = m_color.red;
            it->green = m_color.green;
            it->blue = m_color.blue;
            it->alpha = 255;
        }
    }

private:
    keyleds::RGBColor   m_color;
};

REGISTER_RENDERER("fill", FillRenderer)
