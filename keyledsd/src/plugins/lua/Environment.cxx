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
#include "plugins/lua/Environment.h"

#include <cassert>
#include <lua.hpp>
#include <sstream>
#include "plugins/lua/lua_common.h"


namespace keyleds { namespace lua {

static const void * const controllerToken = &controllerToken;

/****************************************************************************/
// Global scope

static int luaPrint(lua_State * lua)        // (...) => ()
{
    int nargs = lua_gettop(lua);
    std::ostringstream buffer;

    for (int idx = 1; idx <= nargs; ++idx) {
        lua_getglobal(lua, "tostring");
        lua_pushvalue(lua, idx);
        lua_pcall(lua, 1, 1, 0);
        buffer <<lua_tostring(lua, -1);
        lua_pop(lua, 1);
    }

    auto * controller = Environment(lua).controller();
    if (!controller) { return luaL_error(lua, noEffectTokenErrorMessage); }

    controller->print(buffer.str());
    return 0;
}

static int luaToColor(lua_State * lua)      // (any) => (table)
{
    int nargs = lua_gettop(lua);
    if (nargs == 1) {
        // We are called as a conversion function
        if (lua_isstring(lua, 1)) {
            // On a string, parse it
            auto * controller = Environment(lua).controller();
            if (!controller) { return luaL_error(lua, noEffectTokenErrorMessage); }

            size_t size;
            const char * string = lua_tolstring(lua, 1, &size);
            RGBAColor color;
            if (controller->parseColor(std::string(string, size), &color)) {
                lua_push(lua, color);
                return 1;
            }
        }
    } else if (3 <= nargs && nargs <= 4) {
        if (nargs == 3) { lua_pushnumber(lua, 1.0); }
        if (lua_isnumber(lua, 1) && lua_isnumber(lua, 2) &&
            lua_isnumber(lua, 3) && lua_isnumber(lua, 4)) {
            lua_createtable(lua, 4, 0);
            luaL_getmetatable(lua, metatable<RGBAColor>::name);
            lua_setmetatable(lua, -2);
            lua_insert(lua, 1);
            lua_rawseti(lua, 1, 4);
            lua_rawseti(lua, 1, 3);
            lua_rawseti(lua, 1, 2);
            lua_rawseti(lua, 1, 1);
            return 1;
        }
    }
    lua_pushnil(lua);
    return 1;
}

/// Yields the calling animation
static int luaWait(lua_State * lua)
{
    if (!lua_isnumber(lua, 1)) {
        return luaL_argerror(lua, 1, "Duration must be a number");
    }
    lua_pushlightuserdata(lua, const_cast<void *>(Environment::waitToken));
    lua_pushvalue(lua, 1);
    return lua_yield(lua, 2);
}

static const luaL_reg keyledsGlobals[] = {
    { "print",      luaPrint    },
    { "tocolor",    luaToColor  },
    { "wait",       luaWait     },
    { nullptr, nullptr }
};

/****************************************************************************/

const void * const Environment::waitToken = &Environment::waitToken;

void Environment::openKeyleds(Controller * controller)
{
    SAVE_TOP(m_lua);

    // Save controller pointer
    lua_pushlightuserdata(m_lua, const_cast<void *>(controllerToken));
    lua_pushlightuserdata(m_lua, static_cast<void *>(controller));
    lua_rawset(m_lua, LUA_GLOBALSINDEX);

    // Register types
    registerType<const device::KeyDatabase *>(m_lua);
    registerType<const device::KeyDatabase::KeyGroup *>(m_lua);
    registerType<const device::KeyDatabase::Key *>(m_lua);
    registerType<device::RenderTarget *>(m_lua);
    registerType<RGBAColor>(m_lua);
    registerType<Thread>(m_lua);

    // Register globals
    lua_pushvalue(m_lua, LUA_GLOBALSINDEX);
    luaL_register(m_lua, nullptr, keyledsGlobals);
    lua_pop(m_lua, 1);

    CHECK_TOP(m_lua, 0);
}

Environment::Controller * Environment::controller() const
{
    SAVE_TOP(m_lua);

    lua_pushlightuserdata(m_lua, const_cast<void *>(controllerToken));
    lua_rawget(m_lua, LUA_GLOBALSINDEX);
    auto * controller = static_cast<Controller *>(const_cast<void *>(lua_topointer(m_lua, -1)));
    lua_pop(m_lua, 1);

    CHECK_TOP(m_lua, 0);
    return controller;
}

/****************************************************************************/

} } // namespace keyleds::lua
