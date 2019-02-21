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
#include <vector>

using namespace std::literals::chrono_literals;
using keyleds::tools::parseDuration;

static constexpr auto transparent = keyleds::RGBAColor{0, 0, 0, 0};
static constexpr auto white = keyleds::RGBAColor{255, 255, 255, 255};

/****************************************************************************/

namespace keyleds::plugin {

class FeedbackEffect final : public SimpleEffect
{
    struct KeyPress
    {
        const KeyDatabase::Key *    key;    ///< Entry in the database
        milliseconds                age;    ///< How long ago the press happened
    };

public:
    explicit FeedbackEffect(EffectService & service)
      : m_color(getConfig<RGBAColor>(service, "color").value_or(white)),
        m_sustain(getConfig<milliseconds>(service, "sustain").value_or(750ms)),
        m_decay(getConfig<milliseconds>(service, "decay").value_or(500ms)),
        m_buffer(*service.createRenderTarget())
    {
        std::fill(m_buffer.begin(), m_buffer.end(), transparent);
    }

    void render(milliseconds elapsed, RenderTarget & target) override
    {
        const auto lifetime = m_sustain + m_decay;

        for (auto & keyPress : m_presses) {
            RGBAColor color;

            keyPress.age += elapsed;
            if (keyPress.age <= m_sustain) {
                color = m_color;
            } else if (keyPress.age < lifetime) {
                color = {
                    m_color.red,
                    m_color.green,
                    m_color.blue,
                    RGBAColor::channel_type(
                        m_color.alpha * (lifetime - keyPress.age) / m_decay
                    )
                };
            } else {
                color = transparent;
            }
            m_buffer[keyPress.key->index] = color;
        }
        m_presses.erase(
            std::remove_if(m_presses.begin(), m_presses.end(),
                           [lifetime](const auto & keyPress){ return keyPress.age >= lifetime; }),
            m_presses.end()
        );
        blend(target, m_buffer);
    }

    void handleKeyEvent(const KeyDatabase::Key & key, bool) override
    {
        for (auto & keyPress : m_presses) {
            if (keyPress.key == &key) {
                keyPress.age = milliseconds::zero();
                return;
            }
        }
        m_presses.push_back({ &key, milliseconds::zero() });
    }

private:
    const RGBAColor     m_color;        ///< color taken by keys on keypress
    const milliseconds  m_sustain;      ///< how long key remains at full color
    const milliseconds  m_decay;        ///< how long it takes for keys to fade out

    RenderTarget &      m_buffer;       ///< this plugin's rendered state
    std::vector<KeyPress> m_presses;    ///< list of recent keypresses still drawn
};

KEYLEDSD_SIMPLE_EFFECT("feedback", FeedbackEffect);

} // namespace keyleds::plugin
