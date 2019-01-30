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
#include "keyledsd/utils.h"
#include <algorithm>
#include <cmath>

using namespace std::literals::chrono_literals;
using keyleds::parseDuration;

static constexpr float pi = 3.14159265358979f;

/****************************************************************************/

class BreateEffect final : public plugin::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    explicit BreateEffect(EffectService & service)
     : m_buffer(service.createRenderTarget())
    {
        auto color = RGBAColor::parse(service.getConfig("color"))
                               .value_or(RGBAColor{255, 255, 255, 255});
        m_alpha = color.alpha;
        color.alpha = 0;

        const auto & groupStr = service.getConfig("group");
        if (!groupStr.empty()) {
            auto git = std::find_if(
                service.keyGroups().begin(), service.keyGroups().end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != service.keyGroups().end()) { m_keys = &*git; }
        }

        m_period = parseDuration<milliseconds>(service.getConfig("period")).value_or(m_period);

        std::fill(m_buffer->begin(), m_buffer->end(), color);
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        m_time += elapsed;
        if (m_time >= m_period) { m_time -= m_period; }

        float t = float(m_time.count()) / float(m_period.count());
        float alphaf = -std::cos(2.0f * pi * t);
        auto alpha = RGBAColor::channel_type(m_alpha * (unsigned(128.0f * alphaf) + 128) / 256);

        if (m_keys) {
            for (const auto & key : *m_keys) { (*m_buffer)[key.index].alpha = alpha; }
        } else {
            for (auto & key : *m_buffer) { key.alpha = alpha; }
        }
        blend(target, *m_buffer);
    }

private:
    RenderTarget *  m_buffer = nullptr; ///< this plugin's rendered state
    const KeyGroup* m_keys = nullptr;   ///< what keys the effect applies to. Empty for whole keyboard.
    uint8_t         m_alpha = 255;      ///< peak alpha value through the breathing cycle

    milliseconds    m_time = 0ms;       ///< time since beginning of current cycle
    milliseconds    m_period = 10000ms; ///< total duration of a cycle
};

KEYLEDSD_SIMPLE_EFFECT("breathe", BreateEffect);
