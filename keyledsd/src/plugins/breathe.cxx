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
#include <cmath>
#include <cstdint>
#include "keyledsd/common.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/DeviceManager.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/RenderLoop.h"
#include "tools/accelerated.h"

using keyleds::KeyDatabase;
using keyleds::RGBAColor;
using keyleds::RenderTarget;
static constexpr float pi = 3.14159265358979f;

class BreatheRenderer final : public keyleds::Renderer
{
public:
    BreatheRenderer(const keyleds::DeviceManager & manager,
                    const keyleds::Configuration::Plugin & conf,
                    const keyleds::IRendererPlugin::group_map & groups)
     : m_buffer(manager.getRenderTarget()),
       m_time(0), m_period(10000)
    {
        auto color = RGBAColor(0, 0, 0, 0);
        auto cit = conf.items().find("color");
        if (cit != conf.items().end()) {
            color = RGBAColor::parse(cit->second);
        }
        m_alpha = color.alpha;
        color.alpha = 0;

        std::fill(m_buffer.begin(), m_buffer.end(), color);

        auto kit = conf.items().find("group");
        if (kit != conf.items().end()) {
            auto git = groups.find(kit->second);
            if (git != groups.end()) { m_keys = git->second; }
        }

        auto pit = conf.items().find("period");
        if (pit != conf.items().end()) {
            auto period = std::stoul(pit->second);
            if (period > 0) { m_period = period; }
        }
    }

    void render(unsigned long ns, RenderTarget & target) override
    {
        m_time += ns / 1000000;
        if (m_time >= m_period) { m_time -= m_period; }

        float t = float(m_time) / float(m_period);
        float alphaf = -std::cos(2.0f * pi * t);
        uint8_t alpha = m_alpha * (unsigned(128.0f * alphaf) + 128) / 256;

        if (m_keys.empty()) {
            for (auto & key : m_buffer) { key.alpha = alpha; }
        } else {
            for (const auto & key : m_keys) { m_buffer.get(key->index).alpha = alpha; }
        }
        blend(reinterpret_cast<uint8_t*>(target.data()),
              reinterpret_cast<uint8_t*>(m_buffer.data()), m_buffer.size());
    }

private:
    RenderTarget m_buffer;
    std::vector<const KeyDatabase::Key *> m_keys;
    uint8_t             m_alpha;

    unsigned            m_time;
    unsigned            m_period;
};

REGISTER_RENDERER("breathe", BreatheRenderer)
