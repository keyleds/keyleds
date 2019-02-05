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
#include <optional>
#include <string>

using namespace std::literals::chrono_literals;
using keyleds::parseDuration;

static constexpr float pi = 3.14159265358979f;
static constexpr unsigned int accuracy = 1024;
static constexpr auto transparent = keyleds::RGBAColor{0, 0, 0, 0};

static_assert(accuracy && ((accuracy & (accuracy - 1)) == 0),
              "accuracy must be a power of two");

/****************************************************************************/

class WaveEffect final : public plugin::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    explicit WaveEffect(EffectService & service)
     : m_service(service),
       m_period(parseDuration<milliseconds>(service.getConfig("period")).value_or(10s)),
       m_keys(findGroup(service.keyGroups(), service.getConfig("group"))),
       m_phases(computePhases(service.keyDB(), m_keys,
                float(keyleds::parseNumber(service.getConfig("length")).value_or(1000)),
                float(keyleds::parseNumber(service.getConfig("direction")).value_or(0)))),
       m_colors(generateColorTable(parseColors(service))),
       m_buffer(*service.createRenderTarget())
    {
        std::fill(m_buffer.begin(), m_buffer.end(), transparent);
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

                m_buffer[(*m_keys)[idx].index] = m_colors[tphi];
            }
        } else {
            const auto & keyDB = m_service.keyDB();
            assert(keyDB.size() == m_phases.size());
            for (KeyDatabase::size_type idx = 0; idx < keyDB.size(); ++idx) {
                auto tphi = (t >= m_phases[idx] ? 0 : accuracy) + t - m_phases[idx];

                m_buffer[keyDB[idx].index] = m_colors[tphi];
            }
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

    static std::vector<unsigned>
    computePhases(const KeyDatabase & keyDB, const std::optional<KeyGroup> keys,
                  const float length, const float direction)
    {
        auto frequency = 1000.0f / length;
        auto freqX = frequency * std::sin(2.0f * pi / 360.0f * direction);
        auto freqY = frequency * std::cos(2.0f * pi / 360.0f * direction);
        auto bounds = keyDB.bounds();

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

        auto phases = std::vector<unsigned>();
        if (keys) {
            phases.resize(keys->size());
            std::transform(keys->begin(), keys->end(), phases.begin(), keyPhase);
        } else {
            phases.resize(keyDB.size());
            std::transform(keyDB.begin(), keyDB.end(), phases.begin(), keyPhase);
        }
        return phases;
    }

    static std::vector<RGBAColor> parseColors(const EffectService & service)
    {
        auto colors = std::vector<RGBAColor>();
        for (const auto & item : service.configuration()) {
            if (item.first.rfind("color", 0) == 0) {
                auto color = RGBAColor::parse(item.second);
                if (color) { colors.push_back(*color); }
            }
        }
        return colors;
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
    const EffectService &           m_service;
    const milliseconds              m_period;   ///< total duration of a cycle.
    const std::optional<KeyGroup>   m_keys;     ///< what keys the effect applies to.
    const std::vector<unsigned>     m_phases;   ///< one per key in m_keys or one per key in m_buffer.
                                                ///< From 0 (no phase shift) to 1000 (2*pi shift)
    const std::vector<RGBAColor>    m_colors;   ///< pre-computed color samples.

    RenderTarget &                  m_buffer;   ///< this plugin's rendered state
    milliseconds                    m_time = 0ms; ///< time since beginning of current cycle.
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
