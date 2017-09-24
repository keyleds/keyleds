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
#include <random>
#include <vector>
#include "keyledsd/common.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/RenderLoop.h"

using keyleds::KeyDatabase;
using keyleds::RenderTarget;
using keyleds::RGBAColor;

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

class StarsPlugin final : public keyleds::EffectPlugin
{
    struct Star
    {
        const KeyDatabase::Key *    key;
        RGBAColor                   color;
        unsigned                    age;
    };

public:
    StarsPlugin(const keyleds::DeviceManager & manager,
                const keyleds::Configuration::Plugin & conf,
                const keyleds::EffectPluginFactory::group_map & groups)
     : m_buffer(manager.getRenderTarget()),
       m_duration(fromConf(conf, "duration", 1000)),
       m_stars(fromConf(conf, "number", 8))
    {
        // Load color list
        auto cit = conf.items().begin();
        for (unsigned idx = 0; (cit = conf.items().find("color" + std::to_string(idx))) != conf.items().end(); ++idx) {
            m_colors.push_back(RGBAColor::parse(cit->second));
        }

        // Load key list
        auto kit = conf.items().find("group");
        if (kit != conf.items().end()) {
            auto git = groups.find(kit->second);
            if (git != groups.end()) { m_keys = git->second; }
        } else {
            m_keys.reserve(manager.keyDB().size());
            for (const auto & item : manager.keyDB()) {
                m_keys.push_back(&item.second);
            }
        }

        // Get ready
        std::fill(m_buffer.begin(), m_buffer.end(), RGBAColor{0, 0, 0, 0});

        for (std::size_t idx = 0; idx < m_stars.size(); ++idx) {
            auto & star = m_stars[idx];
            rebirth(star);
            star.age = idx * m_duration / m_stars.size();
        }
    }

    void render(unsigned long ms, keyleds::RenderTarget & target) override
    {
        for (auto & star : m_stars) {
            star.age += ms;
            if (star.age >= m_duration) { rebirth(star); }
            m_buffer.get(star.key->index) = RGBAColor(
                star.color.red,
                star.color.green,
                star.color.blue,
                star.color.alpha * (m_duration - star.age) / m_duration
            );
        }

        keyleds::blend(target, m_buffer);
    }

    void rebirth(Star & star)
    {
        typedef std::uniform_int_distribution<> distribution;

        if (star.key != nullptr) {
            m_buffer.get(star.key->index) = RGBAColor{0, 0, 0, 0};
        }
        star.key = m_keys[distribution(0, m_keys.size() - 1)(m_random)];
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
    RenderTarget            m_buffer;
    std::minstd_rand        m_random;

    unsigned                m_duration;
    std::vector<RGBAColor>  m_colors;
    std::vector<const KeyDatabase::Key *> m_keys;

    std::vector<Star>       m_stars;
};

REGISTER_EFFECT_PLUGIN("stars", StarsPlugin)
