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
#include <limits>
#include <vector>

static constexpr auto transparent = keyleds::RGBAColor{0, 0, 0, 0};

/****************************************************************************/

namespace keyleds::plugin {

class FillEffect final : public SimpleEffect
{
    enum class Mode { Blend, Overwrite };

public:
    explicit FillEffect(EffectService & service)
     : m_buffer(*service.createRenderTarget())
    {
        auto fillColor = getConfig<RGBAColor>(service, "color").value_or(transparent);
        std::fill(m_buffer.begin(), m_buffer.end(), fillColor);

        for (const auto & item : service.configuration()) {
            if (item.first == "color") { continue; }
            if (!std::holds_alternative<std::string>(item.second)) { continue; }

            auto group = parseConfig<KeyDatabase::KeyGroup>(service, item.first);
            auto color = parseConfig<RGBAColor>(service, std::get<std::string>(item.second));

            if (group && color) {
                std::for_each(group->begin(), group->end(), [this, &color](const auto & key) {
                    m_buffer[key.index] = *color;
                });
            }
        }

        bool hasAlpha = std::any_of(m_buffer.begin(), m_buffer.end(), [](const auto & val) {
            return val.alpha < std::numeric_limits<RGBAColor::channel_type>::max();
        });
        m_mode = hasAlpha ? Mode::Blend : Mode::Overwrite;
    }

    void render(milliseconds, RenderTarget & target) override
    {
        switch (m_mode) {
        case Mode::Blend:
            blend(target, m_buffer);
            break;
        case Mode::Overwrite:
            std::copy(m_buffer.cbegin(), m_buffer.cend(), target.begin());
            break;
        }
    }

private:
    RenderTarget &          m_buffer;   ///< this plugin's rendered state
    Mode                    m_mode = Mode::Overwrite; ///< how to use target buffer
};

KEYLEDSD_SIMPLE_EFFECT("fill", FillEffect);

} // namespace keyleds::plugin
