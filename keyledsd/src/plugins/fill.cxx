/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017 Julien Hartmann, juli1.hartmann@gmail.com
 *
 * This program is free software: you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation, either version 3 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program.  If not, see <http://www.gnu.org/licenses/>.
 */
#include <algorithm>
#include <vector>
#include "keyledsd/Configuration.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/colors.h"

using keyleds::RGBAColor;

/****************************************************************************/

class FillPlugin final : public keyleds::EffectPlugin
{
    using KeyGroup = KeyDatabase::KeyGroup;

    class Rule final
    {
    public:
                            Rule(KeyGroup keys, RGBAColor color)
                            : m_keys(std::move(keys)), m_color(color) {}
        const KeyGroup &    keys() const { return m_keys; }
        const RGBAColor &   color() const { return m_color; }
    private:
        KeyGroup            m_keys;
        RGBAColor           m_color;
    };

public:
    FillPlugin(const keyleds::DeviceManager &,
               const keyleds::Configuration::Plugin & conf,
               const keyleds::EffectPluginFactory::group_list groups)
     : m_fill(0, 0, 0, 0)
    {
        const auto & colorStr = conf["color"];
        if (!colorStr.empty()) { m_fill = RGBAColor::parse(colorStr); }

        for (const auto & item : conf.items()) {
            if (item.first == "color") { continue; }
            auto git = std::find_if(
                groups.begin(), groups.end(),
                [item](const auto & group) { return group.name() == item.first; }
            );
            if (git == groups.end()) { continue; }
            m_rules.emplace_back(*git, RGBAColor::parse(item.second));
        }
    }

    void render(unsigned long, RenderTarget & target) override
    {
        if (m_fill.alpha > 0) {
            std::fill(target.begin(), target.end(), m_fill);
        }
        for (const auto & rule : m_rules) {
            for (const auto & key : rule.keys()) {
                target[key.index] = rule.color();
            }
        }
    }

private:
    RGBAColor           m_fill;         ///< color to fill whole target with before applying rules
    std::vector<Rule>   m_rules;        ///< each rule maps a key group to a color
};

REGISTER_EFFECT_PLUGIN("fill", FillPlugin)
