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
#include "plugins/lua/lua_RGBAColor.h"

#include <array>
#include <cstring>
#include <iomanip>
#include <sstream>
#include <lua.hpp>
#include "keyledsd/colors.h"
#include "plugins/lua/lua_common.h"

using keyleds::RGBAColor;

namespace keyleds { namespace lua {

/****************************************************************************/

static constexpr std::array<const char *, 4> keys = {{ "red", "green", "blue", "alpha" }};

static int indexForKey(lua_State * lua, const char * key)
{
    for (unsigned idx = 0; idx < keys.size(); ++idx) {
        if (std::strcmp(keys[idx], key) == 0) {
            return idx + 1;
        }
    }
    return luaL_error(lua, badKeyErrorMessage, key);
}

static int equal(lua_State * lua)
{
    for (int i = 1; i <= 4; ++i) {
        lua_rawgeti(lua, 1, i);
        lua_rawgeti(lua, 2, i);
        if (lua_tonumber(lua, -2) != lua_tonumber(lua, -1)) {
            lua_pushboolean(lua, false);
            return 1;
        }
        lua_pop(lua, 2);
    }
    lua_pushboolean(lua, true);
    return 1;
}

static int index(lua_State * lua)
{
    lua_rawgeti(lua, 1, indexForKey(lua, luaL_checkstring(lua, 2)));
    return 1;
}

static int newIndex(lua_State * lua)
{
    lua_rawseti(lua, 1, indexForKey(lua, luaL_checkstring(lua, 2)));
    return 1;
}

static int toString(lua_State * lua)
{
    std::ostringstream buffer;
    buffer <<std::fixed <<std::setprecision(3);
    buffer <<"color(";
    lua_rawgeti(lua, 1, 1); buffer <<lua_tonumber(lua, -1) <<", ";
    lua_rawgeti(lua, 1, 2); buffer <<lua_tonumber(lua, -1) <<", ";
    lua_rawgeti(lua, 1, 3); buffer <<lua_tonumber(lua, -1) <<", ";
    lua_rawgeti(lua, 1, 4); buffer <<lua_tonumber(lua, -1) <<")";
    lua_pushstring(lua, buffer.str().c_str());
    return 1;
}

/****************************************************************************/

void lua_push(lua_State * lua, RGBAColor value)
{
    lua_createtable(lua, 4, 0);
    luaL_getmetatable(lua, metatable<RGBAColor>::name);
    lua_setmetatable(lua, -2);
    lua_pushnumber(lua, lua_Number(value.red) / 255.0);
    lua_rawseti(lua, -2, 1);
    lua_pushnumber(lua, lua_Number(value.green) / 255.0);
    lua_rawseti(lua, -2, 2);
    lua_pushnumber(lua, lua_Number(value.blue) / 255.0);
    lua_rawseti(lua, -2, 3);
    lua_pushnumber(lua, lua_Number(value.alpha) / 255.0);
    lua_rawseti(lua, -2, 4);
}

/****************************************************************************/

const char * metatable<RGBAColor>::name = "LRGBAColor";
const struct luaL_Reg metatable<RGBAColor>::meta_methods[] = {
    { "__eq",       equal },
    { "__index",    index },
    { "__newindex", newIndex },
    { "__tostring", toString },
    { nullptr,      nullptr}
};

} } // namespace keyleds::lua
