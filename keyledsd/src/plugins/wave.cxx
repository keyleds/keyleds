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
#include <cstdint>
#include <iostream>
#include <string>
#include "keyledsd/common.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/RenderLoop.h"
#include "logging.h"

LOGGING("plugin-wave");

using keyleds::KeyDatabase;
using keyleds::RGBAColor;
using keyleds::RenderTarget;
static constexpr float pi = 3.14159265358979f;

/****************************************************************************/

static unsigned fromConf(const keyleds::Configuration::Plugin & conf,
                         const std::string & key, unsigned def)
{
    auto it = conf.items().find(key);
    if (it == conf.items().end()) { return def; }
    std::size_t pos;
    auto value = std::stoul(it->second, &pos, 10);
    if (pos < it->second.size()) { return def; }
    return value;
}

/****************************************************************************/

class WaveRenderer final : public keyleds::Renderer
{
public:
    WaveRenderer(const keyleds::DeviceManager & manager,
                 const keyleds::Configuration::Plugin & conf,
                 const keyleds::IRendererPlugin::group_map & groups)
     : m_buffer(manager.getRenderTarget()),
       m_time(0),
       m_period(fromConf(conf, "period", 10000)),
       m_length(fromConf(conf, "length", 1000)),
       m_direction(fromConf(conf, "direction", 0))
    {
        // Load color list
        std::vector<RGBAColor> colors;
        auto cit = conf.items().begin();
        for (unsigned idx = 0; (cit = conf.items().find("color" + std::to_string(idx))) != conf.items().end(); ++idx) {
            colors.push_back(RGBAColor::parse(cit->second));
        }
        m_colors = generateColorTable(colors);

        // Load key list
        auto kit = conf.items().find("group");
        if (kit != conf.items().end()) {
            auto git = groups.find(kit->second);
            if (git != groups.end()) { m_keys = git->second; }
        }

        // Get ready
        computePhases(manager);
        std::fill(m_buffer.begin(), m_buffer.end(), RGBAColor{255, 0, 0, 0});
    }

    void render(unsigned long ns, RenderTarget & target) override
    {
        m_time += ns / 1000000;
        if (m_time >= m_period) { m_time -= m_period; }

        int t = 1024 * m_time / m_period;

        if (m_keys.empty()) {
            assert(m_buffer.size() == m_phases.size());
            for (std::size_t idx = 0; idx < m_buffer.size(); ++idx) {
                int tphi = t - m_phases[idx];
                if (tphi < 0) { tphi += 1024; }

                m_buffer[idx] = m_colors[tphi];
            }
        } else {
            assert(m_keys.size() == m_phases.size());
            for (std::size_t idx = 0; idx < m_keys.size(); ++idx) {
                int tphi = t - m_phases[idx];
                if (tphi < 0) { tphi += 1024; }

                m_buffer.get(m_keys[idx]->index) = m_colors[tphi];
            }
        }
        keyleds::blend(target, m_buffer);
    }

private:
    void computePhases(const keyleds::DeviceManager & manager)
    {
        int waveX = int(1024.0f * 1000.0f / float(m_length) * std::sin(2.0f * pi / 360.0f * float(m_direction)));
        int waveY = int(1024.0f * 1000.0f / float(m_length) * std::cos(2.0f * pi / 360.0f * float(m_direction)));
        auto bounds = manager.keyDB().bounds();

        m_phases.clear();
        const auto & blocks = manager.device().blocks();

        if (m_keys.empty()) {
            for (std::size_t bidx = 0; bidx < blocks.size(); ++bidx) {
                const auto & block = blocks[bidx];

                std::size_t kidx;
                for (kidx = 0; kidx < block.keys().size(); ++kidx) {
                    auto it = manager.keyDB().find(RenderTarget::key_descriptor{bidx, kidx});
                    if (it == manager.keyDB().end()) {
                        WARNING("Key(", bidx, ", ", kidx, ") code ", +block.keys()[kidx], " missing in database");
                        m_phases.push_back(0);
                        continue;
                    }

                    int x = (it->second.position.x0 + it->second.position.x1) / 2;
                    int y = (it->second.position.y0 + it->second.position.y1) / 2;
                    if (x == 0 && y == 0) {
                        m_phases.push_back(0);
                    } else {
                        // Reverse Y axis as keyboard layout uses top<down
                        x = 1024 * (x - bounds.x0) / (bounds.x1 - bounds.x0);
                        y = 1024 - 1024 * (y - bounds.x0) / (bounds.x1 - bounds.x0);
                        auto val = (waveX * x + waveY * y) / 1024 % 1024;
                        if (val < 0) { val += 1024; }
                        m_phases.push_back(val);
                    }
                }

                auto curPtr = &m_buffer.get(bidx, kidx);
                auto nextPtr = bidx + 1 < blocks.size() ? &m_buffer.get(bidx + 1, 0) : m_buffer.end();
                m_phases.insert(m_phases.end(), nextPtr - curPtr, 0);
            }
        } else {
            for (auto key : m_keys) {
                int x = (key->position.x0 + key->position.x1) / 2;
                int y = (key->position.y0 + key->position.y1) / 2;
                if (x == 0 && y == 0) {
                    m_phases.push_back(0);
                } else {
                    // Reverse Y axis as keyboard layout uses top<down
                    x = 1024 * (x - bounds.x0) / (bounds.x1 - bounds.x0);
                    y = 1024 - 1024 * (y - bounds.x0) / (bounds.x1 - bounds.x0);
                    auto val = (waveX * x + waveY * y) / 1024 % 1024;
                    if (val < 0) { val += 1024; }
                    m_phases.push_back(val);
                }
            }
        }
    }

    static std::vector<RGBAColor> generateColorTable(const std::vector<RGBAColor> & colors)
    {
        std::vector<RGBAColor> table(1024);

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
    RenderTarget            m_buffer;
    std::vector<const KeyDatabase::Key *> m_keys;
    std::vector<unsigned>   m_phases;   ///< one per key in m_keys or one per key in m_buffer.
                                        ///< From 0 (no phase shift) to 1000 (2*pi shift)
    std::vector<RGBAColor>  m_colors;

    unsigned            m_time;
    unsigned            m_period;
    unsigned            m_length;
    unsigned            m_direction;
};

REGISTER_RENDERER("wave", WaveRenderer)
