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
#include "lua/lua_RenderTarget.h"

#include "keyledsd/KeyDatabase.h"
#include "lua/Environment.h"
#include "lua/lua_common.h"
#include <algorithm>
#include <cassert>
#include <lua.hpp>

using keyleds::KeyDatabase;
using keyleds::RenderTarget;

namespace keyleds::lua {

/****************************************************************************/

static int toTargetIndex(lua_State * lua, int idx) // 0-based
{
    if (lua_is<const KeyDatabase::Key *>(lua, idx)) {
        return static_cast<int>(lua_to<const KeyDatabase::Key *>(lua, idx)->index);
    }
    if (lua_isnumber(lua, idx)) {
        return static_cast<int>(lua_tointeger(lua, idx) - 1);
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

        auto it = db->findName(keyName);
        if (it != db->end()) {
            return static_cast<int>(it->index);
        }
        return -1;
    }
    return luaL_argerror(lua, idx, badTypeErrorMessage);
}

/****************************************************************************/

static int blend(lua_State * lua)
{
    using keyleds::blend;

    auto * to = lua_check<RenderTarget *>(lua, 1);
    if (!to) { return luaL_argerror(lua, 1, noLongerExistsErrorMessage); }
    auto * from = lua_check<RenderTarget *>(lua, 2);
    if (!from) { return luaL_argerror(lua, 2, noLongerExistsErrorMessage); }

    blend(*to, *from);
    return 0;
}

static int copy(lua_State * lua)
{
    auto * to = lua_check<RenderTarget *>(lua, 1);
    if (!to) { return luaL_argerror(lua, 1, noLongerExistsErrorMessage); }
    auto * from = lua_check<RenderTarget *>(lua, 2);
    if (!from) { return luaL_argerror(lua, 2, noLongerExistsErrorMessage); }

    std::copy(from->begin(), from->end(), to->begin());
    return 0;
}

static int fill(lua_State * lua)
{
    auto * to = lua_check<RenderTarget *>(lua, 1);
    if (!to) { return luaL_argerror(lua, 1, noLongerExistsErrorMessage); }

    std::fill(to->begin(), to->end(), lua_checkcolor(lua, 2));
    return 0;
}

static int multiply(lua_State * lua)
{
    using keyleds::multiply;

    auto * to = lua_check<RenderTarget *>(lua, 1);
    if (!to) { return luaL_argerror(lua, 1, noLongerExistsErrorMessage); }
    auto * from = lua_check<RenderTarget *>(lua, 2);
    if (!from) { return luaL_argerror(lua, 2, noLongerExistsErrorMessage); }

    multiply(*to, *from);
    return 0;
}

static int create(lua_State * lua)
{
    auto * controller = Environment(lua).controller();
    if (!controller) { return luaL_error(lua, noEffectTokenErrorMessage); }

    auto * target = controller->createRenderTarget();
    std::fill(target->begin(), target->end(), RGBAColor(0, 0, 0, 0));

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
    if (index < 0 || static_cast<unsigned>(index) >= target->size()) {
        lua_pushnil(lua);
        return 1;
    }

    // Extract color data
    lua_push(lua, (*target)[static_cast<unsigned>(index)]);
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
    if (index < 0 || static_cast<unsigned>(index) >= target->size()) {
        return 0;   /// invalid indices and key names are silently ignored, to allow
                    /// generic code that works on different keyboards
    }

    if (lua_is<Interpolator>(lua, 3)) {
        lua_pushvalue(lua, 3);
        lua_pushvalue(lua, 1);
        Interpolator::start(lua, static_cast<unsigned>(index));
        return 0;
    }
    (*target)[static_cast<unsigned>(index)] = lua_checkcolor(lua, 3);
    return 0;
}

/****************************************************************************/

const char * const metatable<RenderTarget *>::name = "RenderTarget";
const struct luaL_Reg metatable<RenderTarget *>::methods[] = {
    { "blend",      blend },
    { "copy",       copy },
    { "fill",       fill },
    { "multiply",   multiply },
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

} // namespace keyleds::lua
