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
#include "plugins/lua/lua_common.h"

#include <cstring>
#include <lua.hpp>

namespace keyleds { namespace lua {

/****************************************************************************/

const char * const badKeyErrorMessage = "bad key '%s'";
const char * const badTypeErrorMessage = "bad type";
const char * const badIndexErrorMessage = "index out of bounds '%d'";
const char * const noLongerExistsErrorMessage = "object no longer exists";
const char * const noEffectTokenErrorMessage = "no effect token in environment";

/****************************************************************************/

bool lua_handleMethodIndex(lua_State * lua, int index, const luaL_Reg methods[])
{
    const char * name = lua_tostring(lua, index);
    if (!name) { return false; }

    for (size_t idx = 0; methods[idx].name; ++idx) {
        if (std::strcmp(methods[idx].name, name) == 0) {
            lua_pushcfunction(lua, methods[idx].func);
            return true;
        }
    }
    return false;
}

int lua_keyError(lua_State * lua, int index)
{
    lua_getglobal(lua, "tostring");
    lua_pushvalue(lua, index);
    lua_call(lua, 1, 1);
    return luaL_error(lua, badKeyErrorMessage, lua_tostring(lua, -1));
}

/****************************************************************************/

} }
