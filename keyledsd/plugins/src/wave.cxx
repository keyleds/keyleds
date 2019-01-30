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
#include <cassert>
#include <cmath>
#include <cstddef>
#include <string>

using namespace std::literals::chrono_literals;
using keyleds::parseDuration;

static constexpr float pi = 3.14159265358979f;
static constexpr unsigned int accuracy = 1024;

static_assert(accuracy && ((accuracy & (accuracy - 1)) == 0),
              "accuracy must be a power of two");

/****************************************************************************/

class WaveEffect final : public plugin::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    explicit WaveEffect(EffectService & service)
     : m_service(service),
       m_buffer(service.createRenderTarget())
    {
        m_period = parseDuration<milliseconds>(service.getConfig("period")).value_or(m_period);
        m_length = static_cast<unsigned>(
            keyleds::parseNumber(service.getConfig("length")).value_or(m_length)
        );
        m_direction = static_cast<unsigned>(
            keyleds::parseNumber(service.getConfig("direction")).value_or(m_direction)
        );

        // Load color list
        std::vector<RGBAColor> colors;
        for (const auto & item : service.configuration()) {
            if (item.first.rfind("color", 0) == 0) {
                auto color = RGBAColor::parse(item.second);
                if (color) { colors.push_back(*color); }
            }
        }
        m_colors = generateColorTable(colors);

        // Load key list
        const auto & groupStr = service.getConfig("group");
        if (!groupStr.empty()) {
            auto git = std::find_if(
                service.keyGroups().begin(), service.keyGroups().end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != service.keyGroups().end()) { m_keys = &*git; }
        }

        // Get ready
        computePhases(service.keyDB());
        std::fill(m_buffer->begin(), m_buffer->end(), RGBAColor{0, 0, 0, 0});
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        m_time += elapsed;
        if (m_time >= m_period) { m_time -= m_period; }

        auto t = accuracy * m_time / m_period;

        if (m_keys) {
            assert(m_keys->size() == m_phases.size());
            for (KeyGroup::size_type idx = 0; idx < m_keys->size(); ++idx) {
                auto tphi = (t >= m_phases[idx] ? 0 : accuracy) + t - m_phases[idx];

                (*m_buffer)[(*m_keys)[idx].index] = m_colors[tphi];
            }
        } else {
            const auto & keyDB = m_service.keyDB();
            assert(keyDB.size() == m_phases.size());
            for (KeyDatabase::size_type idx = 0; idx < keyDB.size(); ++idx) {
                auto tphi = (t >= m_phases[idx] ? 0 : accuracy) + t - m_phases[idx];

                (*m_buffer)[keyDB[idx].index] = m_colors[tphi];
            }
        }
        blend(target, *m_buffer);
    }

private:
    void computePhases(const KeyDatabase & keyDB)
    {
        auto frequency = 1000.0f / float(m_length);
        auto freqX = frequency * std::sin(2.0f * pi / 360.0f * float(m_direction));
        auto freqY = frequency * std::cos(2.0f * pi / 360.0f * float(m_direction));
        auto bounds = keyDB.bounds();

        m_phases.clear();

        auto keyPhase = [&](const auto & key) {
            auto x = (key.position.x0 + key.position.x1) / 2u;
            auto y = (key.position.y0 + key.position.y1) / 2u;

            // Reverse Y axis as keyboard layout uses top<down
            auto xpos = float(x - bounds.x0) / float(bounds.x1 - bounds.x0);
            auto ypos = 1.0f - float(y - bounds.x0) / float(bounds.x1 - bounds.x0);

            auto phase = std::fmod(freqX * xpos + freqY * ypos, 1.0f);
            if (phase < 0.0f) { phase += 1.0f; }
            return unsigned(phase * accuracy);
        };

        if (m_keys) {
            std::transform(m_keys->begin(), m_keys->end(),
                           std::back_inserter(m_phases), keyPhase);
        } else {
            std::transform(keyDB.begin(), keyDB.end(),
                           std::back_inserter(m_phases), keyPhase);
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
    const EffectService &   m_service;
    RenderTarget *          m_buffer = nullptr; ///< this plugin's rendered state
    const KeyGroup *        m_keys = nullptr;   ///< what keys the effect applies to.
    std::vector<unsigned>   m_phases;           ///< one per key in m_keys or one per key in m_buffer.
                                                ///< From 0 (no phase shift) to 1000 (2*pi shift)
    std::vector<RGBAColor>  m_colors;           ///< pre-computed color samples.

    milliseconds        m_time = 0ms;           ///< time since beginning of current cycle.
    milliseconds        m_period = 10000ms;     ///< total duration of a cycle.
    unsigned            m_length = 1000;        ///< wave length, in keyboard 1000th.
    unsigned            m_direction = 0;        ///< wave propagation direction (0 for North).
};

/****************************************************************************/

class WaveEffectPlugin final : public plugin::Plugin<WaveEffect> {
public:
    explicit WaveEffectPlugin(const char * name) : Plugin(name) {}

    keyleds::effect::interface::Effect *
    createEffect(const std::string & name, EffectService & service) override
    {
        if (name != this->name()) { return nullptr; }

        const auto & bounds = service.keyDB().bounds();
        if (!(bounds.x0 < bounds.x1 && bounds.y0 < bounds.y1)) {
            service.log(1, "effect requires a valid layout");
            return nullptr;
        }
        return plugin::Plugin<WaveEffect>::createEffect(name, service);
    }
};

KEYLEDSD_EXPORT_PLUGIN("wave", WaveEffectPlugin);
