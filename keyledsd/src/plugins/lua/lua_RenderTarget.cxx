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
#include "plugins/lua/lua_RenderTarget.h"

#include <cassert>
#include <lua.hpp>
#include "keyledsd/device/KeyDatabase.h"
#include "plugins/lua/Environment.h"
#include "plugins/lua/lua_common.h"

using keyleds::device::KeyDatabase;
using keyleds::device::RenderTarget;

namespace keyleds { namespace lua {

/****************************************************************************/

static int toTargetIndex(lua_State * lua, int idx) // 0-based
{
    if (lua_is<const KeyDatabase::Key *>(lua, idx)) {
        return lua_to<const KeyDatabase::Key *>(lua, idx)->index;
    }
    if (lua_isnumber(lua, idx)) {
        return lua_tointeger(lua, idx) - 1;
    }
    if (lua_isstring(lua, idx)) {
        size_t size;
        const char * keyName = lua_tolstring(lua, idx, &size);

        lua_getglobal(lua, "keyleds");
        lua_getfield(lua, -1, "db");
        if (!lua_is<const KeyDatabase *>(lua, -1)) {
            return luaL_error(lua, "keyleds.db is not a valid database");
        }

        auto * db = lua_to<const KeyDatabase *>(lua, -1);
        lua_pop(lua, 2);

        auto it = db->findName(std::string(keyName, size));
        if (it != db->end()) {
            return it->index;
        }
        return -1;
    }
    return luaL_argerror(lua, idx, badTypeErrorMessage);
}

/****************************************************************************/

static int blend(lua_State * lua)
{
    using keyleds::device::blend;

    auto * to = lua_check<RenderTarget *>(lua, 1);
    if (!to) { return luaL_argerror(lua, 1, noLongerExistsErrorMessage); }
    auto * from = lua_check<RenderTarget *>(lua, 2);
    if (!from) { return luaL_argerror(lua, 2, noLongerExistsErrorMessage); }

    blend(*to, *from);
    return 0;
}


static int create(lua_State * lua)
{
    auto * controller = Environment(lua).controller();
    if (!controller) { return luaL_error(lua, noEffectTokenErrorMessage); }

    auto * target = controller->createRenderTarget();
    for (auto & entry : *target) { entry = RGBAColor(0, 0, 0, 0); }

    lua_push(lua, target);
    return 1;
}

/****************************************************************************/

static int destroy(lua_State * lua)
{
    auto * target = lua_to<RenderTarget *>(lua, 1);
    if (!target) { return 0; }                  // object marked as gone already

    auto * controller = Environment(lua).controller();
    assert(controller);

    controller->destroyRenderTarget(target);

    lua_to<RenderTarget *>(lua, 1) = nullptr;   // mark object as gone
    return 0;
}

static int index(lua_State * lua)
{
    auto * target = lua_to<RenderTarget *>(lua, 1);
    if (!target) { return luaL_error(lua, noLongerExistsErrorMessage); }

    // Handle method retrieval
    if (lua_handleMethodIndex(lua, 2, metatable<RenderTarget *>::methods)) { return 1; }

    // Handle table-like access
    int index = toTargetIndex(lua, 2);
    if (index < 0 || unsigned(index) >= target->size()) {
        lua_pushnil(lua);
        return 1;
    }

    // Extract color data
    lua_push(lua, (*target)[index]);
    return 1;
}

static int len(lua_State * lua)
{
    auto * target = lua_to<RenderTarget *>(lua, 1);
    if (!target) { return luaL_error(lua, noLongerExistsErrorMessage); }
    lua_pushinteger(lua, target->size());
    return 1;
}

static int newIndex(lua_State * lua)
{
    auto * target = lua_to<RenderTarget *>(lua, 1);
    if (!target) { return luaL_error(lua, noLongerExistsErrorMessage); }

    int index = toTargetIndex(lua, 2);
    if (index < 0 || unsigned(index) >= target->size()) {
        return 0;   /// invalid indices and key names are silently ignored, to allow
                    /// generic code that works on different keyboards
    }

    if (!lua_is<RGBAColor>(lua, 3)) {
        return luaL_argerror(lua, 3, badTypeErrorMessage);
    }
    lua_rawgeti(lua, 3, 1);
    lua_rawgeti(lua, 3, 2);
    lua_rawgeti(lua, 3, 3);
    lua_rawgeti(lua, 3, 4);

    if (!lua_isnumber(lua, -4) || !lua_isnumber(lua, -3) ||
        !lua_isnumber(lua, -2) || !lua_isnumber(lua, -1)) {
        return luaL_argerror(lua, 3, badTypeErrorMessage);
    }

    (*target)[index] = RGBAColor(
        std::min(255, int(256.0 * lua_tonumber(lua, -4))),
        std::min(255, int(256.0 * lua_tonumber(lua, -3))),
        std::min(255, int(256.0 * lua_tonumber(lua, -2))),
        std::min(255, int(256.0 * lua_tonumber(lua, -1)))
    );
    return 0;
}

/****************************************************************************/

const char * metatable<RenderTarget *>::name = "RenderTarget";
const struct luaL_Reg metatable<RenderTarget *>::methods[] = {
    { "blend",      blend },
    { "new",        create },
    { nullptr,      nullptr }
};
const struct luaL_Reg metatable<RenderTarget *>::meta_methods[] = {
    { "__gc",       destroy },
    { "__index",    index },
    { "__len",      len },
    { "__newindex", newIndex },
    { nullptr,      nullptr}
};

} } // namespace keyleds::lua
