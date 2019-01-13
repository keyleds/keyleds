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
#include <fstream>
#include "keyledsd/DeviceManager.h"
#include "keyledsd/colors.h"
#include "tools/Paths.h"
#include "logging.h"
#include "config.h"

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
{}

const std::string & EffectService::deviceName() const
    { return m_manager.name(); }

const std::string & EffectService::deviceModel() const
    { return m_manager.device().model(); }

const std::string & EffectService::deviceSerial() const
    { return m_manager.serial(); }

const keyleds::KeyDatabase & EffectService::keyDB() const
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

keyleds::RenderTarget * EffectService::createRenderTarget()
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

    if (it != m_renderTargets.end() - 1) { *it = std::move(m_renderTargets.back()); }
    m_renderTargets.pop_back();
}

const std::string & EffectService::getFile(const std::string & name)
{
    m_fileData.clear();
    if (!name.empty()) {
        std::ifstream file;
        std::string actualPath;
        tools::paths::open(file, tools::paths::XDG::Data, KEYLEDSD_DATA_PREFIX "/" + name,
                           std::ios::binary, &actualPath);
        if (file) {
            m_fileData.assign(std::istreambuf_iterator<char>(file),
                              std::istreambuf_iterator<char>());
        }
    }
    return m_fileData;
}

void EffectService::log(unsigned level, const char * msg)
{
    l_logger.print(level, m_configuration.name() + ": " + msg);
}
