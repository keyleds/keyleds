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
#include <cmath>

using namespace std::literals::chrono_literals;

static constexpr auto pi = 3.14159265358979f;
static constexpr auto white = keyleds::RGBAColor{255, 255, 255, 255};

/****************************************************************************/

namespace keyleds::plugin {

class BreatheEffect final : public SimpleEffect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    explicit BreatheEffect(EffectService & service, milliseconds period)
     : m_period(period),
       m_keys(findGroup(service.keyGroups(), service.getConfig("group"))),
       m_buffer(*service.createRenderTarget())
    {
        auto color = RGBAColor::parse(service.getConfig("color")).value_or(white);
        std::swap(color.alpha, m_alpha);

        std::fill(m_buffer.begin(), m_buffer.end(), color);
    }

    static BreatheEffect * create(EffectService & service)
    {
        auto period = tools::parseDuration<milliseconds>(service.getConfig("period")).value_or(10s);
        if (period < 1s) {
            service.log(3, "minimum value for period is 1000ms");
            return nullptr;
        }
        return new BreatheEffect(service, period);
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        m_time += elapsed;
        if (m_time >= m_period) { m_time -= m_period; }

        float t = float(m_time.count()) / float(m_period.count());
        float alphaf = -std::cos(2.0f * pi * t);
        auto alpha = RGBAColor::channel_type(m_alpha * (unsigned(128.0f * alphaf) + 128) / 256);

        if (m_keys) {
            for (const auto & key : *m_keys) { m_buffer[key.index].alpha = alpha; }
        } else {
            for (auto & key : m_buffer) { key.alpha = alpha; }
        }
        blend(target, m_buffer);
    }

private:
    static const std::optional<KeyGroup> findGroup(const std::vector<KeyGroup> & groups,
                                                   const std::string & name)
    {
        if (name.empty()) { return std::nullopt; }
        auto it = std::find_if(groups.begin(), groups.end(),
                               [&](auto & group) { return group.name() == name; });
        if (it == groups.end()) { return std::nullopt; }
        return *it;
    }

private:
    const milliseconds              m_period;   ///< total duration of a cycle
    const std::optional<KeyGroup>   m_keys;     ///< what keys the effect applies to
    uint8_t                         m_alpha = 0;///< peak alpha value through the breathing cycle

    RenderTarget &  m_buffer;           ///< this plugin's rendered state
    milliseconds    m_time = 0ms;       ///< time since beginning of current cycle
};

KEYLEDSD_SIMPLE_EFFECT("breathe", BreatheEffect);

} // namespace keyleds::plugin
