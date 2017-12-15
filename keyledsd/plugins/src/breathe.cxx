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
#include <cmath>
#include "keyledsd/PluginHelper.h"
#include "keyledsd/utils.h"

static constexpr float pi = 3.14159265358979f;

/****************************************************************************/

class BreateEffect final : public plugin::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    BreateEffect(EffectService & service)
     : m_buffer(service.createRenderTarget()),
       m_keys(nullptr),
       m_time(0), m_period(10000)
    {
        auto color = RGBAColor(255, 255, 255, 255);
        RGBAColor::parse(service.getConfig("color"), &color);
        m_alpha = color.alpha;
        color.alpha = 0;

        const auto & groupStr = service.getConfig("group");
        if (!groupStr.empty()) {
            auto git = std::find_if(
                service.keyGroups().begin(), service.keyGroups().end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != service.keyGroups().end()) { m_keys = &*git; }
        }

        keyleds::parseNumber(service.getConfig("period"), &m_period);

        std::fill(m_buffer->begin(), m_buffer->end(), color);
    }

    void render(unsigned long ms, RenderTarget & target) override
    {
        m_time += ms;
        if (m_time >= m_period) { m_time -= m_period; }

        float t = float(m_time) / float(m_period);
        float alphaf = -std::cos(2.0f * pi * t);
        uint8_t alpha = m_alpha * (unsigned(128.0f * alphaf) + 128) / 256;

        if (m_keys) {
            for (const auto & key : *m_keys) { (*m_buffer)[key.index].alpha = alpha; }
        } else {
            for (auto & key : *m_buffer) { key.alpha = alpha; }
        }
        blend(target, *m_buffer);
    }

private:
    RenderTarget *  m_buffer;       ///< this plugin's rendered state
    const KeyGroup* m_keys;         ///< what keys the effect applies to. Empty for whole keyboard.
    uint8_t         m_alpha;        ///< peak alpha value through the breathing cycle

    unsigned        m_time;         ///< time in milliseconds since beginning of current cycle
    unsigned        m_period;       ///< total duration of a cycle in milliseconds
};

KEYLEDSD_SIMPLE_EFFECT("breathe", BreateEffect);
