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
#include <cstdint>
#include "keyledsd/common.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/RenderLoop.h"

using KeyGroup = keyleds::KeyDatabase::KeyGroup;
using keyleds::RGBAColor;
using keyleds::RenderTarget;
static constexpr float pi = 3.14159265358979f;

class BreathePlugin final : public keyleds::EffectPlugin
{
public:
    BreathePlugin(const keyleds::DeviceManager & manager,
                  const keyleds::Configuration::Plugin & conf,
                  const keyleds::EffectPluginFactory::group_list & groups)
     : m_buffer(manager.getRenderTarget()),
       m_time(0), m_period(10000)
    {
        auto color = RGBAColor(0, 0, 0, 0);
        const auto & colorStr = conf["color"];
        if (!colorStr.empty()) { color = RGBAColor::parse(colorStr); }
        m_alpha = color.alpha;
        color.alpha = 0;

        const auto & groupStr = conf["group"];
        if (!groupStr.empty()) {
            auto git = std::find_if(
                groups.begin(), groups.end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != groups.end()) { m_keys = *git; }
        }

        auto period = std::stoul(conf["period"]);
        if (period > 0) { m_period = period; }

        std::fill(m_buffer.begin(), m_buffer.end(), color);
    }

    void render(unsigned long ms, RenderTarget & target) override
    {
        m_time += ms;
        if (m_time >= m_period) { m_time -= m_period; }

        float t = float(m_time) / float(m_period);
        float alphaf = -std::cos(2.0f * pi * t);
        uint8_t alpha = m_alpha * (unsigned(128.0f * alphaf) + 128) / 256;

        if (m_keys.empty()) {
            for (auto & key : m_buffer) { key.alpha = alpha; }
        } else {
            for (const auto & key : m_keys) { m_buffer.get(key.index).alpha = alpha; }
        }
        blend(target, m_buffer);
    }

private:
    RenderTarget    m_buffer;
    KeyGroup        m_keys;
    uint8_t         m_alpha;

    unsigned        m_time;
    unsigned        m_period;
};

REGISTER_EFFECT_PLUGIN("breathe", BreathePlugin)
