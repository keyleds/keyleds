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
#include "lua/lua_RGBAColor.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <iomanip>
#include <limits>
#include <sstream>
#include <lua.hpp>
#include "keyledsd/colors.h"
#include "lua/lua_common.h"

using keyleds::RGBAColor;

namespace keyleds { namespace lua {

/****************************************************************************/

static constexpr std::array<const char *, 4> keys = {{ "red", "green", "blue", "alpha" }};

static int indexForKey(lua_State * lua, const char * key)
{
    static_assert(keys.size() < std::numeric_limits<int>::max(), "key number must fit an int");
    auto it = std::find_if(keys.begin(), keys.end(),
                           [&](auto item) { return std::strcmp(item, key) == 0; });
    if (it != keys.end()) {
        return static_cast<int>(it - keys.begin() + 1);
    }
    return luaL_error(lua, badKeyErrorMessage, key);
}

static int add(lua_State * lua)
{
    if (!lua_is<RGBAColor>(lua, 1)) { luaL_argerror(lua, 1, badTypeErrorMessage); }
    if (!lua_is<RGBAColor>(lua, 2)) { luaL_argerror(lua, 2, badTypeErrorMessage); }

    lua_rawgeti(lua, 2, 4);
    auto multiplier = lua_tonumber(lua, -1);

    lua_createtable(lua, 4, 0);
    luaL_getmetatable(lua, metatable<RGBAColor>::name);
    lua_setmetatable(lua, -2);

    for (int i = 1; i <= 3; ++i) {
        lua_rawgeti(lua, 1, i);
        lua_rawgeti(lua, 2, i);
        lua_pushnumber(lua, lua_tonumber(lua, -2) + multiplier * lua_tonumber(lua, -1));
        lua_rawseti(lua, -4, i);
        lua_pop(lua, 2);
    }
    lua_rawgeti(lua, 1, 4);
    lua_rawseti(lua, -2, 4);
    return 1;
}

static int div(lua_State * lua)
{
    if (!lua_is<RGBAColor>(lua, 1)) { luaL_argerror(lua, 1, badTypeErrorMessage); }
    auto divisor = luaL_checknumber(lua, 2);

    lua_createtable(lua, 4, 0);
    luaL_getmetatable(lua, metatable<RGBAColor>::name);
    lua_setmetatable(lua, -2);

    for (int i = 1; i <= 3; ++i) {
        lua_rawgeti(lua, 1, i);
        lua_pushnumber(lua, lua_tonumber(lua, -1) / divisor);
        lua_rawseti(lua, -3, i);
        lua_pop(lua, 1);
    }
    lua_rawgeti(lua, 1, 4);
    lua_rawseti(lua, -2, 4);
    return 1;
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

static int mul(lua_State * lua)
{
    if (!lua_is<RGBAColor>(lua, 1)) { luaL_argerror(lua, 1, badTypeErrorMessage); }
    auto multiplier = luaL_checknumber(lua, 2);

    lua_createtable(lua, 4, 0);
    luaL_getmetatable(lua, metatable<RGBAColor>::name);
    lua_setmetatable(lua, -2);

    for (int i = 1; i <= 3; ++i) {
        lua_rawgeti(lua, 1, i);
        lua_pushnumber(lua, lua_tonumber(lua, -1) * multiplier);
        lua_rawseti(lua, -3, i);
        lua_pop(lua, 1);
    }
    lua_rawgeti(lua, 1, 4);
    lua_rawseti(lua, -2, 4);
    return 1;
}

static int newIndex(lua_State * lua)
{
    lua_rawseti(lua, 1, indexForKey(lua, luaL_checkstring(lua, 2)));
    return 1;
}

static int sub(lua_State * lua)
{
    if (!lua_is<RGBAColor>(lua, 1)) { luaL_argerror(lua, 1, badTypeErrorMessage); }
    if (!lua_is<RGBAColor>(lua, 2)) { luaL_argerror(lua, 2, badTypeErrorMessage); }

    lua_rawgeti(lua, 2, 4);
    auto multiplier = lua_tonumber(lua, -1);

    lua_createtable(lua, 4, 0);
    luaL_getmetatable(lua, metatable<RGBAColor>::name);
    lua_setmetatable(lua, -2);

    for (int i = 1; i <= 3; ++i) {
        lua_rawgeti(lua, 1, i);
        lua_rawgeti(lua, 2, i);
        lua_pushnumber(lua, lua_tonumber(lua, -2) - multiplier * lua_tonumber(lua, -1));
        lua_rawseti(lua, -4, i);
        lua_pop(lua, 2);
    }
    lua_rawgeti(lua, 1, 4);
    lua_rawseti(lua, -2, 4);
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
    SAVE_TOP(lua);
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
    CHECK_TOP(lua, +1);
}

RGBAColor lua_tocolor(lua_State * lua, int index)
{
    SAVE_TOP(lua);
    lua_rawgeti(lua, index, 1);
    lua_rawgeti(lua, index, 2);
    lua_rawgeti(lua, index, 3);
    lua_rawgeti(lua, index, 4);

    if (!lua_isnumber(lua, -4) || !lua_isnumber(lua, -3) ||
        !lua_isnumber(lua, -2) || !lua_isnumber(lua, -1)) {
        luaL_argerror(lua, 2, badTypeErrorMessage);
    }

    static constexpr unsigned channel_max = std::numeric_limits<RGBAColor::channel_type>::max();
    auto result = RGBAColor(
        RGBAColor::channel_type(std::min(channel_max, unsigned(256.0 * lua_tonumber(lua, -4)))),
        RGBAColor::channel_type(std::min(channel_max, unsigned(256.0 * lua_tonumber(lua, -3)))),
        RGBAColor::channel_type(std::min(channel_max, unsigned(256.0 * lua_tonumber(lua, -2)))),
        RGBAColor::channel_type(std::min(channel_max, unsigned(256.0 * lua_tonumber(lua, -1))))
    );
    lua_pop(lua, 4);
    CHECK_TOP(lua, 0);
    return result;
}

RGBAColor lua_checkcolor(lua_State * lua, int index)
{
    if (!lua_is<RGBAColor>(lua, index)) {
        luaL_argerror(lua, index, badTypeErrorMessage);
        // does not return
    }
    return lua_tocolor(lua, index);
}


/****************************************************************************/

const char * const metatable<RGBAColor>::name = "LRGBAColor";
const struct luaL_Reg metatable<RGBAColor>::meta_methods[] = {
    { "__add",      add },
    { "__div",      div },
    { "__eq",       equal },
    { "__index",    index },
    { "__mul",      mul },
    { "__newindex", newIndex },
    { "__sub",      sub },
    { "__tostring", toString },
    { nullptr,      nullptr}
};

} } // namespace keyleds::lua
