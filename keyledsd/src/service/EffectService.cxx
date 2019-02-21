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
#include "keyledsd/service/EffectService.h"

#include "config.h"
#include "keyledsd/colors.h"
#include "keyledsd/logging.h"
#include "keyledsd/service/DeviceManager.h"
#include "keyledsd/tools/Paths.h"
#include <algorithm>
#include <cassert>
#include <fstream>

LOGGING("effect-service");

using keyleds::service::EffectService;

/****************************************************************************/

EffectService::EffectService(const DeviceManager & manager,
                             const Configuration & configuration,
                             const Configuration::Effect & effectConfiguration,
                             std::vector<KeyGroup> keyGroups)
 : m_manager(manager),
   m_configuration(configuration),
   m_effectConfiguration(effectConfiguration),
   m_keyGroups(std::move(keyGroups))
{}

EffectService::~EffectService() = default;

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

const EffectService::color_map & EffectService::colors() const
    { return m_configuration.customColors; }

const EffectService::config_map & EffectService::configuration() const
    { return m_effectConfiguration.items; }

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
        auto file = tools::paths::open<std::ifstream>(
            tools::paths::XDG::Data, KEYLEDSD_DATA_PREFIX "/" + name, std::ios::binary
        );
        if (file) {
            m_fileData.assign(std::istreambuf_iterator<char>(file->stream),
                              std::istreambuf_iterator<char>());
        }
    }
    return m_fileData;
}

void EffectService::log(logging::level_t level, const char * msg)
{
    l_logger.print(level, m_effectConfiguration.name + ": " + msg);
}
