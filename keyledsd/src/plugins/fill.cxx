#include <memory>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/plugin.h"

using keyleds::IRenderer;
using keyleds::IRendererPlugin;
using keyleds::RendererPluginManager;
using keyleds::RendererPlugin;

class FillRenderer final : public IRenderer
{
    const keyleds::Device & m_device;
public:
    FillRenderer(const keyleds::DeviceManager & manager,
                 const keyleds::Configuration::Plugin &)
        : m_device(manager.device()) {}

    bool isFilter() const override { return true; }

    void render(KeyledsdTarget & target) const override
    {
        const auto & blocks = m_device.blocks();
        size_t idx = 0;
        for (auto it = blocks.cbegin(); it != blocks.cend(); ++it) {
            for (size_t i = 0; i < it->keys().size(); ++i) {
                auto & color = target.colors[idx++];
                color.red = m_red;
                color.green = m_green;
                color.blue = m_blue;
                color.alpha = 255;
            }
        }
    }
public:
    static const uint8_t m_red = 128;
    static const uint8_t m_green = 224;
    static const uint8_t m_blue = 255;
};

static const keyleds::RendererPlugin<FillRenderer> maker("fill");
