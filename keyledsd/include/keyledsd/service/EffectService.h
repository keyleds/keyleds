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
#ifndef KEYLEDSD_EFFECT_DEVICE_MANAGER_PROXY_H_2DC03730
#define KEYLEDSD_EFFECT_DEVICE_MANAGER_PROXY_H_2DC03730
#ifndef KEYLEDSD_INTERNAL
#   error "Internal header - must not be pulled into plugins"
#endif

#include "keyledsd/plugin/interfaces.h"
#include "keyledsd/service/Configuration.h"
#include "keyledsd/KeyDatabase.h"
#include <memory>
#include <vector>


namespace keyleds::service {

class DeviceManager;

/****************************************************************************/

class EffectService final : public plugin::EffectService
{
    using KeyGroup = KeyDatabase::KeyGroup;
public:
    EffectService(const DeviceManager &, const Configuration::Effect &, std::vector<KeyGroup>);
    ~EffectService();

    const std::string & deviceName() const override;
    const std::string & deviceModel() const override;
    const std::string & deviceSerial() const override;

    const KeyDatabase & keyDB() const override;
    const std::vector<KeyGroup> & keyGroups() const override;

    const string_map &  configuration() const override;
    const std::string & getConfig(const char *) const override;

    RenderTarget *      createRenderTarget() override;
    void                destroyRenderTarget(RenderTarget *) override;

    const std::string & getFile(const std::string &) override;

    void                log(unsigned, const char * msg) override;

private:
    const DeviceManager &                       m_manager;
    const Configuration::Effect &               m_configuration;
    const std::vector<KeyGroup>                 m_keyGroups;
    std::vector<std::unique_ptr<RenderTarget>>  m_renderTargets;
    std::string                                 m_fileData;
};

/****************************************************************************/

} // namespace keyleds::service

#endif
