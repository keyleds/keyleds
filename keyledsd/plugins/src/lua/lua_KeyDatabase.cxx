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
#include "lua/lua_KeyDatabase.h"

#include <array>
#include <cstdlib>
#include <lua.hpp>
#include "lua/lua_common.h"
#include "lua/lua_Key.h"

using keyleds::KeyDatabase;

namespace keyleds { namespace lua {

/****************************************************************************/

static int angle(lua_State * lua)
{
    const auto * db = lua_check<const KeyDatabase *>(lua, 1);
    const auto * keyA = lua_check<const KeyDatabase::Key *>(lua, 2);
    const auto * keyB = lua_check<const KeyDatabase::Key *>(lua, 3);
    lua_pushnumber(lua, db->angle(*keyA, *keyB));
    return 1;
}

static int distance(lua_State * lua)
{
    const auto * db = lua_check<const KeyDatabase *>(lua, 1);
    const auto * keyA = lua_check<const KeyDatabase::Key *>(lua, 2);
    const auto * keyB = lua_check<const KeyDatabase::Key *>(lua, 3);
    lua_pushnumber(lua, db->distance(*keyA, *keyB));
    return 1;
}

static int findKeyCode(lua_State * lua)
{
    const auto * db = lua_check<const KeyDatabase *>(lua, 1);
    const auto code = luaL_checkinteger(lua, 2);

    if (code > std::numeric_limits<int>::max()) {
        lua_pushnil(lua);
        return 1;
    }

    auto it = db->findKeyCode(static_cast<int>(code));
    if (it != db->end()) {
        lua_push(lua, &*it);
    } else {
        lua_pushnil(lua);
    }
    return 1;
}

static int findName(lua_State * lua)
{
    const auto * db = lua_check<const KeyDatabase *>(lua, 1);

    auto it = db->findName(luaL_checkstring(lua, 2));
    if (it != db->end()) {
        lua_push(lua, &*it);
    } else {
        lua_pushnil(lua);
    }
    return 1;
}

static const luaL_Reg methods[] = {
    { "angle",          angle },
    { "distance",       distance },
    { "findKeyCode",    findKeyCode },
    { "findName",       findName },
    { nullptr,          nullptr }
};

/****************************************************************************/

static int index(lua_State * lua)
{
    const auto * db = lua_to<const KeyDatabase *>(lua, 1);

    auto idx = lua_tointeger(lua, 2);
    if (idx != 0) {
        if (static_cast<size_t>(std::abs(idx)) > db->size()) {
            return luaL_error(lua, badIndexErrorMessage, idx);
        }
        auto decoded = idx > 0
                     ? static_cast<unsigned>(idx) - 1u
                     : db->size() - static_cast<unsigned>(-idx);
        lua_push(lua, &(*db)[decoded]);
        return 1;
    }
    if (lua_handleMethodIndex(lua, 2, methods)) { return 1; }
    return lua_keyError(lua, 2);
}

static int len(lua_State * lua)
{
    const auto * db = lua_to<const KeyDatabase *>(lua, 1);
    lua_pushinteger(lua, static_cast<lua_Integer>(db->size()));
    return 1;
}

/****************************************************************************/

const char * const metatable<const KeyDatabase *>::name = "LKeyDatabase";
const struct luaL_Reg metatable<const KeyDatabase *>::meta_methods[] = {
    { "__index",        index },
    { "__len",          len },
    { nullptr,          nullptr}
};

} } // namespace keyleds::lua
