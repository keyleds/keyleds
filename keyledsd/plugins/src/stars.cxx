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
#include <random>
#include <vector>

using namespace std::literals::chrono_literals;
using keyleds::parseDuration;

/****************************************************************************/

class StarsEffect final : public plugin::Effect
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
       m_buffer(service.createRenderTarget())
    {
        m_duration = parseDuration<milliseconds>(service.getConfig("duration")).value_or(m_duration);

        auto number = keyleds::parseNumber(service.getConfig("number")).value_or(8);
        m_stars.resize(number);

        // Load color list
        for (const auto & item : service.configuration()) {
            if (item.first.rfind("color", 0) == 0) {
                auto color = RGBAColor::parse(item.second);
                if (color) { m_colors.push_back(*color); }
            }
        }

        // Load key list
        const auto & groupStr = service.getConfig("group");
        if (!groupStr.empty()) {
            auto git = std::find_if(
                service.keyGroups().begin(), service.keyGroups().end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != service.keyGroups().end()) { m_keys = &*git; }
        }

        // Get ready
        std::fill(m_buffer->begin(), m_buffer->end(), RGBAColor{0, 0, 0, 0});

        for (std::size_t idx = 0; idx < m_stars.size(); ++idx) {
            auto & star = m_stars[idx];
            rebirth(star);
            star.age = idx * m_duration / m_stars.size();
        }
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        for (auto & star : m_stars) {
            star.age += elapsed;
            if (star.age >= m_duration) { rebirth(star); }
            (*m_buffer)[star.key->index] = RGBAColor(
                star.color.red,
                star.color.green,
                star.color.blue,
                RGBAColor::channel_type(star.color.alpha * (m_duration - star.age) / m_duration)
            );
        }

        blend(target, *m_buffer);
    }

    void rebirth(Star & star)
    {
        if (star.key != nullptr) {
            (*m_buffer)[star.key->index] = RGBAColor{0, 0, 0, 0};
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
    const EffectService &   m_service;
    RenderTarget *          m_buffer = nullptr; ///< this plugin's rendered state
    std::minstd_rand        m_random;           ///< picks stars when they are reborn

    milliseconds            m_duration = 1000ms;///< how long stars stay alive
    std::vector<RGBAColor>  m_colors;           ///< list of colors to choose from
    const KeyGroup *        m_keys = nullptr;   ///< what keys the effect applies to.

    std::vector<Star>       m_stars;            ///< all the star objects
};

KEYLEDSD_SIMPLE_EFFECT("stars", StarsEffect);
