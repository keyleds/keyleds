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
#include "keyledsd/PluginHelper.h"
#include "keyledsd/tools/utils.h"
#include <algorithm>
#include <vector>

static constexpr auto transparent = keyleds::RGBAColor{0, 0, 0, 0};

/****************************************************************************/

namespace keyleds::plugin {

class FillEffect final : public SimpleEffect
{
    struct Rule final
    {
        KeyDatabase::KeyGroup   keys;
        RGBAColor               color;
    };

public:
    explicit FillEffect(EffectService & service)
     : m_fill(getConfig<RGBAColor>(service, "color").value_or(transparent)),
       m_rules(buildRules(service))
    {}

    void render(milliseconds, RenderTarget & target) override
    {
        if (m_fill.alpha > 0) {
            std::fill(target.begin(), target.end(), m_fill);
        }
        for (const auto & rule : m_rules) {
            for (const auto & key : rule.keys) {
                target[key.index] = rule.color;
            }
        }
    }

private:
    static std::vector<Rule> buildRules(const EffectService & service)
    {
        auto rules = std::vector<Rule>();
        for (const auto & item : service.configuration()) {
            if (item.first == "color") { continue; }
            if (!std::holds_alternative<std::string>(item.second)) { continue; }

            auto git = std::find_if(
                service.keyGroups().begin(), service.keyGroups().end(),
                [item](const auto & group) { return group.name() == item.first; }
            );
            if (git == service.keyGroups().end()) { continue; }

            auto color = RGBAColor::parse(std::get<std::string>(item.second));
            if (color) { rules.push_back({*git, *color}); }
        }
        return rules;
    }

private:
    const RGBAColor         m_fill;     ///< fill whole target before applying rules
    const std::vector<Rule> m_rules;    ///< each rule maps a key group to a color
};

KEYLEDSD_SIMPLE_EFFECT("fill", FillEffect);

} // namespace keyleds::plugin
