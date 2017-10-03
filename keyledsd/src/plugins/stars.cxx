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
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/colors.h"

using keyleds::DeviceManager;
using keyleds::RGBAColor;

/****************************************************************************/

class StarsPlugin final : public keyleds::EffectPlugin
{
    using KeyGroup = KeyDatabase::KeyGroup;

    struct Star
    {
        const KeyDatabase::Key *    key;
        RGBAColor                   color;
        unsigned                    age;
    };

public:
    StarsPlugin(const keyleds::DeviceManager & manager,
                const keyleds::Configuration::Plugin & conf,
                const keyleds::EffectPluginFactory::group_list groups)
     : m_manager(manager),
       m_buffer(manager.getRenderTarget()),
       m_duration(1000)
    {
        auto duration = std::stoul(conf["duration"]);
        if (duration > 0) { m_duration = duration; }

        auto number = std::stoul(conf["number"]);
        m_stars.resize(number > 0 ? number : 8);

        // Load color list
        for (const auto & item : conf.items()) {
            if (item.first.rfind("color", 0) == 0) {
                m_colors.push_back(RGBAColor::parse(item.second));
            }
        }

        // Load key list
        const auto & groupStr = conf["group"];
        if (!groupStr.empty()) {
            auto git = std::find_if(
                groups.begin(), groups.end(),
                [groupStr](const auto & group) { return group.name() == groupStr; });
            if (git != groups.end()) { m_keys = *git; }
        }

        // Get ready
        std::fill(m_buffer.begin(), m_buffer.end(), RGBAColor{0, 0, 0, 0});

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
            m_buffer[star.key->index] = RGBAColor(
                star.color.red,
                star.color.green,
                star.color.blue,
                star.color.alpha * (m_duration - star.age) / m_duration
            );
        }

        blend(target, m_buffer);
    }

    void rebirth(Star & star)
    {
        using distribution = std::uniform_int_distribution<>;

        if (star.key != nullptr) {
            m_buffer[star.key->index] = RGBAColor{0, 0, 0, 0};
        }
        if (!m_keys.empty()) {
            star.key = &m_keys[distribution(0, m_keys.size() - 1)(m_random)];
        } else {
            star.key = &m_manager.keyDB()[distribution(0, m_manager.keyDB().size() - 1)(m_random)];
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
    const DeviceManager &   m_manager;
    RenderTarget            m_buffer;       ///< this plugin's rendered state
    std::minstd_rand        m_random;       ///< picks stars when they are reborn

    unsigned                m_duration;     ///< how long stars stay alive, in milliseconds
    std::vector<RGBAColor>  m_colors;       ///< list of colors to choose from
    KeyGroup                m_keys;         ///< what keys the effect applies to. Empty for whole keyboard.

    std::vector<Star>       m_stars;        ///< all the star objects
};

REGISTER_EFFECT_PLUGIN("stars", StarsPlugin)
