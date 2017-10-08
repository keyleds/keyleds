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
#include "keyledsd/effect/EffectService.h"

#include <algorithm>
#include <cassert>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/colors.h"
#include "logging.h"

LOGGING("effect-service");

using keyleds::effect::EffectService;

/****************************************************************************/

EffectService::EffectService(const DeviceManager & manager,
                             const Configuration::Effect & configuration,
                             std::vector<KeyGroup> keyGroups)
 : m_manager(manager),
   m_configuration(configuration),
   m_keyGroups(std::move(keyGroups))
{}

EffectService::~EffectService()
{
    DEBUG("disposing of ", m_renderTargets.size(), " render targets from ", m_configuration.name());
}

const std::string & EffectService::deviceName() const
    { return m_manager.name(); }

const std::string & EffectService::deviceModel() const
    { return m_manager.device().model(); }

const std::string & EffectService::deviceSerial() const
    { return m_manager.serial(); }

const EffectService::KeyDatabase & EffectService::keyDB() const
    { return m_manager.keyDB(); }

const std::vector<EffectService::KeyGroup> & EffectService::keyGroups() const
    { return m_keyGroups; }

const EffectService::string_map & EffectService::configuration() const
    { return m_configuration.items(); }

const std::string & EffectService::getConfig(const std::string & key) const
{
    static const std::string empty;
    auto it = std::find_if(m_configuration.items().begin(), m_configuration.items().end(),
                           [key](const auto & item) { return item.first == key; });
    return it != m_configuration.items().end() ? it->second : empty;
}

bool EffectService::parseColor(const std::string & str, RGBAColor * color) const
{
    return RGBAColor::parse(str, color);
}

EffectService::RenderTarget * EffectService::createRenderTarget()
{
    m_renderTargets.push_back(
        std::make_unique<RenderTarget>(m_manager.getRenderTarget())
    );
    DEBUG("created RenderTarget(", m_renderTargets.back().get(), ")");
    return m_renderTargets.back().get();
}

void EffectService::destroyRenderTarget(RenderTarget * ptr)
{
    auto it = std::find_if(m_renderTargets.begin(), m_renderTargets.end(),
                           [ptr](const auto & item) { return item.get() == ptr; });
    assert(it != m_renderTargets.end());
    std::iter_swap(it, m_renderTargets.end() - 1);
    m_renderTargets.pop_back();
}
