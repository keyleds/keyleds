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
#include "plugins/lua/lua_KeyGroup.h"

#include <lua.hpp>
#include <sstream>
#include "plugins/lua/lua_common.h"
#include "plugins/lua/lua_Key.h"

using keyleds::device::KeyDatabase;

namespace keyleds { namespace lua {

/****************************************************************************/

static int index(lua_State * lua)
{
    const auto * group = lua_to<const KeyDatabase::KeyGroup *>(lua, 1);

    auto idx = luaL_checkinteger(lua, 2);
    if (static_cast<size_t>(std::abs(idx)) > group->size()) {
        return luaL_error(lua, badIndexErrorMessage, idx);
    }
    idx = idx > 0 ? idx - 1 : group->size() + idx;
    lua_push(lua, &(*group)[idx]);
    return 1;
}

static int len(lua_State * lua)
{
    const auto * group = lua_to<const KeyDatabase::KeyGroup *>(lua, 1);
    lua_pushinteger(lua, group->size());
    return 1;
}

static int toString(lua_State * lua)
{
    const auto * keyGroup = lua_to<const KeyDatabase::KeyGroup *>(lua, 1);

    bool isFirst = true;
    std::ostringstream buffer;
    buffer <<"[";
    for (const auto & key : *keyGroup) {
        if (isFirst) { isFirst = false; }
                else { buffer <<", "; }
        buffer <<key.name;
    }
    buffer <<"]";
    lua_pushstring(lua, buffer.str().c_str());
    return 1;
}

/****************************************************************************/

const char * metatable<const KeyDatabase::KeyGroup *>::name = "LKeyGroup";
const struct luaL_Reg metatable<const KeyDatabase::KeyGroup *>::meta_methods[] = {
    { "__index",    index},
    { "__len",      len},
    { "__tostring", toString },
    { nullptr,      nullptr}
};

} } // namespace keyleds::lua
