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
#include "plugins/lua/lua_types.h"

#include <lua.hpp>
#include <cassert>
#include "plugins/lua/lua_common.h"

namespace keyleds { namespace lua {

/****************************************************************************/

void detail::registerType(lua_State * lua, const char * name,
                          const luaL_reg * methods, const luaL_reg * metaMethods, bool weakTable)
{
    assert(lua);
    assert(name);
    assert(metaMethods);

    SAVE_TOP(lua);
    luaL_newmetatable(lua, name);
    luaL_register(lua, nullptr, metaMethods);

    // metatable["__metatable"] = nil   -- makes metatable invisible to lua
    lua_pushnil(lua);
    lua_setfield(lua, -2, "__metatable");

    lua_pop(lua, 1);
    CHECK_TOP(lua, 0);

    if (methods) {
        luaL_register(lua, name, methods);
        lua_pop(lua, 1);
        CHECK_TOP(lua, 0);
    }

    if (weakTable) {
        lua_pushlightuserdata(lua, const_cast<luaL_Reg *>(metaMethods));
        lua_newtable(lua);              // push(weaktable)
        lua_createtable(lua, 0, 1);     // push(metatable)
        lua_pushliteral(lua, "v");      // push("v")
        lua_setfield(lua, -2, "mode");  // pop("v")
        lua_setmetatable(lua, -2);      // pop(metatable)
        lua_rawset(lua, LUA_REGISTRYINDEX);
        CHECK_TOP(lua, 0);
    }
}

bool detail::isType(lua_State * lua, int index, const char * name)
{
    assert(lua);
    assert(name);

    SAVE_TOP(lua);
    if (lua_getmetatable(lua, index) == 0) { return false; }
    luaL_getmetatable(lua, name);
    bool result = lua_rawequal(lua, -2, -1);
    lua_pop(lua, 2);
    CHECK_TOP(lua, 0);

    return result;
}

void detail::lua_pushref(lua_State * lua, const void * value, const char * name, const luaL_reg * metaMethods)
{
    SAVE_TOP(lua);
    lua_pushlightuserdata(lua, const_cast<luaL_reg *>(metaMethods));
    lua_rawget(lua, LUA_REGISTRYINDEX);
    lua_pushlightuserdata(lua, const_cast<void *>(value));
    lua_rawget(lua, -2);

    const void ** ptr = const_cast<const void **>(static_cast<const void * const *>(lua_topointer(lua, -1)));
    if (!ptr) {
        lua_pop(lua, 1);

        ptr = static_cast<const void **>(lua_newuserdata(lua, sizeof(void *)));
        *ptr = value;
        luaL_getmetatable(lua, name);
        lua_setmetatable(lua, -2);

        lua_pushlightuserdata(lua, const_cast<void *>(value));
        lua_pushvalue(lua, -2);
        lua_rawset(lua, -4);
    }
    lua_remove(lua, -2);
    CHECK_TOP(lua, +1);
}

} } // namespace keyleds::lua
