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
#include <optional>
#include <random>
#include <vector>

using namespace std::literals::chrono_literals;

static constexpr auto transparent = keyleds::RGBAColor{0, 0, 0, 0};

/****************************************************************************/

namespace keyleds::plugin {

class StarsEffect final : public SimpleEffect
{
    using KeyGroup = KeyDatabase::KeyGroup;

    struct Star
    {
        const KeyDatabase::Key *    key;
        RGBAColor                   color;
        milliseconds                age;
    };

public:
    explicit StarsEffect(EffectService & service)
     : m_service(service),
       m_colors(parseColors(service)),
       m_duration(tools::parseDuration<milliseconds>(service.getConfig("duration")).value_or(1s)),
       m_keys(findGroup(service.keyGroups(), service.getConfig("group"))),
       m_buffer(*service.createRenderTarget())
    {
        auto number = std::clamp(
            unsigned(tools::parseNumber(service.getConfig("number")).value_or(8)),
            1u, service.keyDB().size()
        );
        m_stars.resize(number);

        // Get ready
        std::fill(m_buffer.begin(), m_buffer.end(), transparent);

        for (std::size_t idx = 0; idx < m_stars.size(); ++idx) {
            auto & star = m_stars[idx];
            rebirth(star);
            star.age = idx * m_duration / m_stars.size();
        }
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

    static const std::optional<KeyGroup> findGroup(const std::vector<KeyGroup> & groups,
                                                   const std::string & name)
    {
        if (name.empty()) { return std::nullopt; }
        auto it = std::find_if(groups.begin(), groups.end(),
                               [&](auto & group) { return group.name() == name; });
        if (it == groups.end()) { return std::nullopt; }
        return *it;
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        for (auto & star : m_stars) {
            star.age += elapsed;
            if (star.age >= m_duration) { rebirth(star); }
            m_buffer[star.key->index] = RGBAColor(
                star.color.red,
                star.color.green,
                star.color.blue,
                RGBAColor::channel_type(star.color.alpha * (m_duration - star.age) / m_duration)
            );
        }

        blend(target, m_buffer);
    }

    void rebirth(Star & star)
    {
        if (star.key != nullptr) {
            m_buffer[star.key->index] = transparent;
        }
        if (m_keys) {
            using distribution = std::uniform_int_distribution<KeyGroup::size_type>;
            star.key = &(*m_keys)[distribution(0, m_keys->size() - 1)(m_random)];
        } else {
            using distribution = std::uniform_int_distribution<KeyDatabase::size_type>;
            star.key = &m_service.keyDB()[distribution(0, m_service.keyDB().size() - 1)(m_random)];
        }
        if (m_colors.empty()) {
            using distribution = std::uniform_int_distribution<unsigned int>;
            auto colordist = distribution(std::numeric_limits<RGBAColor::channel_type>::min(),
                                          std::numeric_limits<RGBAColor::channel_type>::max());
            star.color.red = RGBAColor::channel_type(colordist(m_random));
            star.color.green = RGBAColor::channel_type(colordist(m_random));
            star.color.blue = RGBAColor::channel_type(colordist(m_random));
            star.color.alpha = 255u;
        } else {
            using distribution = std::uniform_int_distribution<std::size_t>;
            star.color = m_colors[distribution(0, m_colors.size() - 1)(m_random)];
        }
        star.age = milliseconds::zero();
    }


private:
    const EffectService &           m_service;
    const std::vector<RGBAColor>    m_colors;   ///< list of colors to choose from
    const milliseconds              m_duration; ///< how long stars stay alive
    const std::optional<KeyGroup>   m_keys;     ///< what keys the effect applies to.

    RenderTarget &          m_buffer;           ///< this plugin's rendered state
    std::minstd_rand        m_random;           ///< picks stars when they are reborn
    std::vector<Star>       m_stars;            ///< all the star objects
};

KEYLEDSD_SIMPLE_EFFECT("stars", StarsEffect);

} // namespace keyleds::plugin
