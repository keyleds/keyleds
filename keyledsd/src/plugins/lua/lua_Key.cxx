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
#include "plugins/lua/lua_Key.h"

#include <cstring>
#include <lua.hpp>
#include "plugins/lua/lua_common.h"

using keyleds::device::KeyDatabase;

namespace keyleds { namespace lua {

/****************************************************************************/

static int index(lua_State * lua)
{
    const auto * key = lua_to<const KeyDatabase::Key *>(lua, 1);
    const char * field = luaL_checkstring(lua, 2);

    if (std::strcmp(field, "index") == 0) {
        lua_pushnumber(lua, key->index);
    } else if (std::strcmp(field, "keyCode") == 0 ) {
        lua_pushnumber(lua, key->keyCode);
    } else if (std::strcmp(field, "name") == 0) {
        lua_pushlstring(lua, key->name.c_str(), key->name.size());
    } else if (std::strcmp(field, "x0") == 0) {
        lua_pushnumber(lua, key->position.x0);
    } else if (std::strcmp(field, "y0") == 0) {
        lua_pushnumber(lua, key->position.y0);
    } else if (std::strcmp(field, "x1") == 0) {
        lua_pushnumber(lua, key->position.x1);
    } else if (std::strcmp(field, "y1") == 0) {
        lua_pushnumber(lua, key->position.y1);
    } else {
        return luaL_error(lua, badKeyErrorMessage, field);
    }
    return 1;
}

static int toString(lua_State * lua)
{
    const auto * key = lua_to<const KeyDatabase::Key *>(lua, 1);
    lua_pushfstring(lua, "Key(%d, %d, %s)", key->index, key->keyCode, key->name.c_str());
    return 1;
}

/****************************************************************************/

const char * metatable<const KeyDatabase::Key *>::name = "LKey";
const struct luaL_Reg metatable<const KeyDatabase::Key *>::meta_methods[] = {
    { "__index",    index},
    { "__tostring", toString },
    { nullptr,      nullptr}
};

} } // namespace keyleds::lua
