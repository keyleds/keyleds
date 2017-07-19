#include <memory>
#include "keyledsd/common.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/Device.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"

using keyleds::RGBAColor;

class Rule final
{
    typedef std::vector<keyleds::Device::key_indices> key_list;
public:
                        Rule(key_list keys, keyleds::RGBAColor color)
                         : m_keys(keys), m_color(color) {}
    const key_list &    keys() const { return m_keys; }
    const RGBAColor &   color() const { return m_color; }
private:
    key_list            m_keys;
    RGBAColor           m_color;
};



class FillRenderer final : public keyleds::Renderer
{
public:
    FillRenderer(const keyleds::DeviceManager &,
                 const keyleds::Configuration::Plugin & conf,
                 const keyleds::IRendererPlugin::group_map & groups)
    {
        auto cit = conf.items().find("color");
        if (cit != conf.items().end()) {
            m_fill = RGBAColor::parse(cit->second);
        } else {
            m_fill = RGBAColor(0, 0, 0, 0);
        }

        for (const auto & item : conf.items()) {
            if (item.first == "color") { continue; }
            auto git = groups.find(item.first);
            if (git == groups.end()) { continue; }
            m_rules.emplace_back(git->second, RGBAColor::parse(item.second));
        }
    }

    void render(unsigned long, keyleds::RenderTarget & target) override
    {
        if (m_fill.alpha > 0) {
            std::fill(target.begin(), target.end(), m_fill);
        }
        for (const auto & rule : m_rules) {
            for (const auto & key : rule.keys()) {
                target.get(key) = rule.color();
            }
        }
    }

private:
    RGBAColor           m_fill;
    std::vector<Rule>   m_rules;
};

REGISTER_RENDERER("fill", FillRenderer)
