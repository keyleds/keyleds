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
#include <vector>
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/colors.h"

using keyleds::RGBAColor;

/****************************************************************************/

class FeedbackEffect final : public keyleds::Effect
{
    struct KeyPress
    {
        const KeyDatabase::Key *    key;    ///< Entry in the database
        unsigned                    age;    ///< How long ago the press happened in ms
    };

public:
    FeedbackEffect(const keyleds::DeviceManager & manager,
                   const keyleds::Configuration::Effect & conf,
                   const keyleds::EffectPluginFactory::group_list)
     : m_buffer(manager.getRenderTarget()),
       m_duration(3000)
    {
        m_color = RGBAColor::parse(conf["color"]);

        auto duration = std::stoul(conf["duration"]);
        if (duration > 0) { m_duration = duration; }

        // Get ready
        std::fill(m_buffer.begin(), m_buffer.end(), RGBAColor{0, 0, 0, 0});
    }

    void render(unsigned long ms, RenderTarget & target) override
    {
        for (auto & keyPress : m_presses) {
            keyPress.age += ms;
            if (keyPress.age > m_duration) { keyPress.age = m_duration; }
            m_buffer[keyPress.key->index] = RGBAColor(
                m_color.red,
                m_color.green,
                m_color.blue,
                m_color.alpha * (m_duration - keyPress.age) / m_duration
            );
        }
        m_presses.erase(std::remove_if(m_presses.begin(), m_presses.end(),
                                       [this](const auto & keyPress){ return keyPress.age >= m_duration; }),
                        m_presses.end());
        blend(target, m_buffer);
    }

    void handleKeyEvent(const KeyDatabase::Key & key, bool) override
    {
        for (auto & keyPress : m_presses) {
            if (keyPress.key == &key) {
                keyPress.age = 0;
                return;
            }
        }
        m_presses.push_back({ &key, 0 });
    }

private:
    RenderTarget        m_buffer;       ///< this plugin's rendered state

    RGBAColor           m_color;        ///< color taken by keys on keypress
    unsigned            m_duration;     ///< how long it takes for keys to fade out in ms
    std::vector<KeyPress> m_presses;    ///< list of recent keypresses still drawn
};

REGISTER_EFFECT_PLUGIN("feedback", FeedbackEffect)
