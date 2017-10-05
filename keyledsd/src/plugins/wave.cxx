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
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/colors.h"
#include "logging.h"

LOGGING("plugin-wave");

using keyleds::RGBAColor;

static constexpr float pi = 3.14159265358979f;
static constexpr int accuracy = 1024;

static_assert(accuracy && ((accuracy & (accuracy - 1)) == 0),
              "accuracy must be a power of two");

/****************************************************************************/

class WaveEffect final : public keyleds::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    WaveEffect(const keyleds::DeviceManager & manager,
               const keyleds::Configuration::Effect & conf,
               const keyleds::EffectPluginFactory::group_list groups)
     : m_buffer(manager.getRenderTarget()),
       m_time(0),
       m_period(10000),
       m_length(1000),
       m_direction(0)
    {
        auto period = std::stoul(conf["period"]);
        if (period > 0) { m_period = period; }

        auto length = std::stoul(conf["length"]);
        if (length > 0) { m_length = length; }

        auto direction = std::stoul(conf["direction"]);
        if (direction > 0) { m_direction = direction; }

        // Load color list
        std::vector<RGBAColor> colors;
        for (const auto & item : conf.items()) {
            if (item.first.rfind("color", 0) == 0) {
                colors.push_back(RGBAColor::parse(item.second));
            }
        }
        m_colors = generateColorTable(colors);

        // Load key list
        const auto & groupStr = conf["group"];
        if (!groupStr.empty()) {
            auto git = std::find_if(
                groups.begin(), groups.end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != groups.end()) { m_keys = *git; }
        }

        // Get ready
        computePhases(manager.keyDB());
        std::fill(m_buffer.begin(), m_buffer.end(), RGBAColor{0, 0, 0, 0});
    }

    void render(unsigned long ms, RenderTarget & target) override
    {
        m_time += ms;
        if (m_time >= m_period) { m_time -= m_period; }

        int t = accuracy * m_time / m_period;

        if (m_keys.empty()) {
            assert(m_buffer.size() == m_phases.size());
            for (std::size_t idx = 0; idx < m_buffer.size(); ++idx) {
                int tphi = t - m_phases[idx];
                if (tphi < 0) { tphi += accuracy; }

                m_buffer[idx] = m_colors[tphi];
            }
        } else {
            assert(m_keys.size() == m_phases.size());
            for (std::size_t idx = 0; idx < m_keys.size(); ++idx) {
                int tphi = t - m_phases[idx];
                if (tphi < 0) { tphi += accuracy; }

                m_buffer[m_keys[idx].index] = m_colors[tphi];
            }
        }
        blend(target, m_buffer);
    }

private:
    void computePhases(const KeyDatabase & keyDB)
    {
        float frequency = float(accuracy) * 1000.0f / float(m_length);
        int freqX = int(frequency * std::sin(2.0f * pi / 360.0f * float(m_direction)));
        int freqY = int(frequency * std::cos(2.0f * pi / 360.0f * float(m_direction)));
        auto bounds = keyDB.bounds();

        m_phases.clear();

        if (m_keys.empty()) {
            for (RenderTarget::size_type kidx = 0; kidx < m_buffer.size(); ++kidx) {
                auto it = keyDB.findIndex(kidx);
                if (it == keyDB.end()) {
                    WARNING("Key(", kidx, ") missing in database");
                    m_phases.push_back(0);
                    continue;
                }

                int x = (it->position.x0 + it->position.x1) / 2;
                int y = (it->position.y0 + it->position.y1) / 2;
                if (x == 0 && y == 0) {
                    m_phases.push_back(0);
                } else {
                    // Reverse Y axis as keyboard layout uses top<down
                    x = accuracy * (x - bounds.x0) / (bounds.x1 - bounds.x0);
                    y = accuracy - accuracy * (y - bounds.x0) / (bounds.x1 - bounds.x0);
                    auto val = (freqX * x + freqY * y) / accuracy % accuracy;
                    if (val < 0) { val += accuracy; }
                    m_phases.push_back(val);
                }
            }
        } else {
            for (const auto & key : m_keys) {
                int x = (key.position.x0 + key.position.x1) / 2;
                int y = (key.position.y0 + key.position.y1) / 2;
                if (x == 0 && y == 0) {
                    m_phases.push_back(0);
                } else {
                    // Reverse Y axis as keyboard layout uses top<down
                    x = accuracy * (x - bounds.x0) / (bounds.x1 - bounds.x0);
                    y = accuracy - accuracy * (y - bounds.x0) / (bounds.x1 - bounds.x0);
                    auto val = (freqX * x + freqY * y) / accuracy % accuracy;
                    if (val < 0) { val += accuracy; }
                    m_phases.push_back(val);
                }
            }
        }
    }

    static std::vector<RGBAColor> generateColorTable(const std::vector<RGBAColor> & colors)
    {
        std::vector<RGBAColor> table(accuracy);

        for (std::vector<RGBAColor>::size_type range = 0; range < colors.size(); ++range) {
            auto first = range * table.size() / colors.size();
            auto last = (range + 1) * table.size() / colors.size();
            auto colorA = colors[range];
            auto colorB = colors[range + 1 >= colors.size() ? 0 : range + 1];

            for (std::vector<RGBAColor>::size_type idx = first; idx < last; ++idx) {
                float ratio = float(idx - first) / float(last - first);

                table[idx] = RGBAColor{
                    RGBAColor::channel_type(colorA.red * (1.0f - ratio) + colorB.red * ratio),
                    RGBAColor::channel_type(colorA.green * (1.0f - ratio) + colorB.green * ratio),
                    RGBAColor::channel_type(colorA.blue * (1.0f - ratio) + colorB.blue * ratio),
                    RGBAColor::channel_type(colorA.alpha * (1.0f - ratio) + colorB.alpha * ratio),
                };
            }
        }
        return table;
    }

private:
    RenderTarget            m_buffer;   ///< this plugin's rendered state
    KeyGroup                m_keys;     ///< what keys the effect applies to. Empty for whole keyboard.
    std::vector<unsigned>   m_phases;   ///< one per key in m_keys or one per key in m_buffer.
                                        ///< From 0 (no phase shift) to 1000 (2*pi shift)
    std::vector<RGBAColor>  m_colors;   ///< pre-computed color samples, build by generateColorTable.

    unsigned            m_time;         ///< time in milliseconds since beginning of current cycle.
    unsigned            m_period;       ///< total duration of a cycle in milliseconds.
    unsigned            m_length;       ///< wave length, in keyboard 1000th.
    unsigned            m_direction;    ///< wave propagation direction, compass style (0 for North).
};

REGISTER_EFFECT_PLUGIN("wave", WaveEffect)
