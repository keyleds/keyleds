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
#ifndef KEYLEDS_PLUGINS_LUA_LUAEFFECT_H_F038C73D
#define KEYLEDS_PLUGINS_LUA_LUAEFFECT_H_F038C73D

#include <memory>
#include "keyledsd/effect/PluginHelper.h"
#include "plugins/lua/Environment.h"

struct lua_State;

namespace std {
    template <> struct default_delete<lua_State> { void operator()(lua_State *) const; };
}


namespace keyleds { namespace plugin { namespace lua {

/****************************************************************************/

class LuaEffect final : public ::plugin::Effect, public keyleds::lua::Environment::Controller
{
    using state_ptr = std::unique_ptr<lua_State>;
public:
                    LuaEffect(std::string name, EffectService &, state_ptr);
                    LuaEffect(const LuaEffect &) = delete;
                    ~LuaEffect();

    // Factory method
    static std::unique_ptr<LuaEffect> create(const std::string & name, EffectService &,
                                             const std::string & code);

public: // Effect interface for keyleds & lua init hook
    void            init();
    void            render(unsigned long ms, RenderTarget & target) override;
    void            handleContextChange(const string_map &) override;
    void            handleGenericEvent(const string_map &) override;
    void            handleKeyEvent(const KeyDatabase::Key &, bool) override;

public: // Environment::Controller interface for lua
    void            print(const std::string &) const override;
    bool            parseColor(const std::string &, RGBAColor *) const override;
    RenderTarget *  createRenderTarget() override;
    void            destroyRenderTarget(RenderTarget *) override;
    int             createThread(lua_State * lua, int nargs) override;
    void            destroyThread(lua_State * lua, Thread &) override;

private:
           void     setupState();
           void     stepThreads(unsigned ms);
           void     runThread(Thread &, lua_State * thread, int nargs);
    static bool     pushHook(lua_State *, const char *);
    static bool     handleError(lua_State *, EffectService &, int code);
private:
    std::string     m_name;         ///< Name of the effect, from config file
    EffectService & m_service;      ///< For communicating with keyleds
    state_ptr       m_state;        ///< Lua container this effect's scripts runs in
    bool            m_enabled;      ///< Should render/event handlers be run?
};

/****************************************************************************/

} } } // namespace keyleds::plugin::lua

#endif
