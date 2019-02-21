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
#include "lua/LuaEffect.h"
#include <algorithm>
#include <cassert>
#include <memory>
#include <vector>

using keyleds::plugin::lua::LuaEffect;

namespace keyleds::plugin {


class LuaPlugin final : public Plugin
{
    struct StateInfo {
        std::unique_ptr<LuaEffect>  effect;
    };
    using state_list = std::vector<StateInfo>;

public:
    explicit LuaPlugin(const char *) {}

    Effect * createEffect(const std::string & name, EffectService & service) override
    {
        auto source = service.getFile("effects/" + name + ".lua");
        if (source.empty()) { return nullptr; }

        StateInfo info;
        try {
            info.effect = LuaEffect::create(name, service, source);
        } catch (std::exception & err) {
            service.log(logging::error::value, err.what());
            return nullptr;
        }

        service.getFile({});    // let the service clear file data

        if (!info.effect) { return nullptr; }

        return m_states.emplace_back(std::move(info)).effect.get();
    }

    void destroyEffect(Effect * ptr, EffectService &) override
    {
        auto it = std::find_if(m_states.begin(), m_states.end(),
                               [ptr](const auto & state) { return state.effect.get() == ptr; });
        assert(it != m_states.end());

        if (it != m_states.end() - 1) { *it = std::move(m_states.back()); }
        m_states.pop_back();
    }


private:
    state_list  m_states;
};

KEYLEDSD_EXPORT_PLUGIN("lua", LuaPlugin);

} // namespace keyleds::plugin
