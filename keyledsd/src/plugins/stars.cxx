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
#include <random>
#include <vector>
#include "keyledsd/effect/PluginHelper.h"

/****************************************************************************/

class StarsEffect final : public plugin::Effect
{
    using KeyGroup = KeyDatabase::KeyGroup;

    struct Star
    {
        const KeyDatabase::Key *    key;
        RGBAColor                   color;
        unsigned                    age;
    };

public:
    StarsEffect(EffectService & service)
     : m_service(service),
       m_buffer(service.createRenderTarget()),
       m_duration(1000),
       m_keys(nullptr)
    {
        auto duration = std::stoul(service.getConfig("duration"));
        if (duration > 0) { m_duration = duration; }

        auto number = std::stoul(service.getConfig("number"));
        m_stars.resize(number > 0 ? number : 8);

        // Load color list
        for (const auto & item : service.configuration()) {
            RGBAColor color;
            if (item.first.rfind("color", 0) == 0 && service.parseColor(item.second, &color)) {
                m_colors.push_back(color);
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

    void render(unsigned long ms, RenderTarget & target) override
    {
        for (auto & star : m_stars) {
            star.age += ms;
            if (star.age >= m_duration) { rebirth(star); }
            (*m_buffer)[star.key->index] = RGBAColor(
                star.color.red,
                star.color.green,
                star.color.blue,
                star.color.alpha * (m_duration - star.age) / m_duration
            );
        }

        blend(target, *m_buffer);
    }

    void rebirth(Star & star)
    {
        using distribution = std::uniform_int_distribution<>;

        if (star.key != nullptr) {
            (*m_buffer)[star.key->index] = RGBAColor{0, 0, 0, 0};
        }
        if (m_keys) {
            star.key = &(*m_keys)[distribution(0, m_keys->size() - 1)(m_random)];
        } else {
            star.key = &m_service.keyDB()[distribution(0, m_service.keyDB().size() - 1)(m_random)];
        }
        if (m_colors.empty()) {
            auto colordist = distribution(std::numeric_limits<RGBAColor::channel_type>::min(),
                                          std::numeric_limits<RGBAColor::channel_type>::max());
            star.color.red = colordist(m_random);
            star.color.green = colordist(m_random);
            star.color.blue = colordist(m_random);
            star.color.alpha = 255;
        } else {
            star.color = m_colors[distribution(0, m_colors.size() - 1)(m_random)];
        }
        star.age = 0;
    }


private:
    const EffectService &   m_service;
    RenderTarget *          m_buffer;       ///< this plugin's rendered state
    std::minstd_rand        m_random;       ///< picks stars when they are reborn

    unsigned                m_duration;     ///< how long stars stay alive, in milliseconds
    std::vector<RGBAColor>  m_colors;       ///< list of colors to choose from
    const KeyGroup *        m_keys;         ///< what keys the effect applies to. Empty for whole keyboard.

    std::vector<Star>       m_stars;        ///< all the star objects
};

KEYLEDSD_SIMPLE_EFFECT("stars", StarsEffect);
