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
#include "lua/lua_Interpolator.h"

#include <cassert>
#include <lua.hpp>
#include "lua/Environment.h"
#include "lua/lua_common.h"

static constexpr lua_Number maximumDuration = 3600000;  // One hour

namespace keyleds { namespace lua {

static const void * const interpolatorToken = &interpolatorToken;
static const char targetLink[] = "target";

/****************************************************************************/

static void pushRegistry(lua_State * lua)
{
    SAVE_TOP(lua);
    lua_pushlightuserdata(lua, const_cast<void *>(interpolatorToken));
    lua_rawget(lua, LUA_REGISTRYINDEX);
    if (!lua_istable(lua, -1)) {
        lua_pop(lua, 1);
        lua_createtable(lua, 1, 1);
        lua_pushlightuserdata(lua, const_cast<void *>(interpolatorToken));
        lua_pushvalue(lua, -2);
        lua_rawset(lua, LUA_REGISTRYINDEX);
    }
    CHECK_TOP(lua, +1);
}

/****************************************************************************/

int luaNewInterpolator(lua_State * lua)
{
    // Analyze arguments
    if (lua_gettop(lua) > 3) { return luaL_error(lua, tooManyArgumentsErrorMessage); }

    int flags = 0;

    auto duration = luaL_checknumber(lua, 1) * 1000.0;
    if (duration <= 0.0 || duration > maximumDuration) {
        return luaL_argerror(lua, 1, "invalid duration");
    }

    RGBAColor startValue, finishValue;
    if (lua_gettop(lua) == 3) {
        startValue = lua_checkcolor(lua, 2);
        finishValue = lua_checkcolor(lua, 3);
        flags |= Interpolator::hasStartValueFlag;
    } else {
        finishValue = lua_checkcolor(lua, 2);
    }

    // Create object
    lua_push(lua, Interpolator{
        LUA_NOREF, flags, 0, unsigned(duration), 0, startValue, finishValue
    });                                                         // push(interpol)
    lua_createtable(lua, 0, 1);                                 // push(fenv)
    lua_setfenv(lua, -2);                                       // pop(fenv)

    return 1;
}

static int stop(lua_State * lua)
{
    lua_check<Interpolator>(lua, 1);
    if (lua_gettop(lua) > 1) { return luaL_error(lua, tooManyArgumentsErrorMessage); }

    Interpolator::stop(lua);
    return 0;
}

static const struct luaL_Reg methods[] = {
    { "stop",       stop },
    { nullptr,      nullptr }
};

static int index(lua_State * lua)
{
    if (lua_handleMethodIndex(lua, 2, methods)) { return 1; }
    return lua_keyError(lua, 2);
}

/****************************************************************************/

void Interpolator::start(lua_State * lua, unsigned keyIndex)
{
    assert(lua_gettop(lua) >= 2);
    assert(lua_is<Interpolator>(lua, -2));
    assert(lua_is<RenderTarget *>(lua, -1));

    auto & interpolator = lua_to<Interpolator>(lua, -2);
    auto * target = lua_to<RenderTarget *>(lua, -1);

    if (interpolator.id != LUA_NOREF) {
        luaL_error(lua, "interpolator already active");
        // does not return
    }

    SAVE_TOP(lua);
    pushRegistry(lua);                                              // push(registry)

    // see whether another interpolator already runs on this key
    size_t size = lua_objlen(lua, -1);
    for (size_t index = 1; index <= size; ++index) {
        lua_rawgeti(lua, -1, index);                                // push(otherinter)
        if (lua_is<Interpolator>(lua, -1)) {
            auto & other = lua_to<Interpolator>(lua, -1);
            if (other.index == keyIndex) {
                lua_getfenv(lua, -1);                               // push(otherfenv)
                lua_getfield(lua, -1, targetLink);                  // push(othertarget)
                auto * otherTarget = lua_to<RenderTarget *>(lua, -1);
                lua_pop(lua, 2);                                    // pop(otherfenv, othertarget)
                if (otherTarget == target) {
                    Interpolator::stop(lua);    // (pop(otherinter))
                    continue;
                }
            }
        }
        lua_pop(lua, 1);                                            // pop(otherinter)
    }

    lua_pushvalue(lua, -3);                                         // push(interpolator)
    interpolator.id = luaL_ref(lua, -2);                            // pop(interpolator)

    // fill in missing values
    interpolator.index = keyIndex;
    interpolator.elapsed = 0;
    if ((interpolator.flags & Interpolator::hasStartValueFlag) == 0) {
        interpolator.startValue = (*lua_to<RenderTarget *>(lua, -2))[keyIndex];
    } else {
        (*target)[keyIndex] = interpolator.startValue;
    }

    // create a reference to the render target
    lua_getfenv(lua, -3);                                           // push(fenv)
    lua_pushvalue(lua, -3);                                         // push(target)
    lua_setfield(lua, -2, targetLink);                              // pop(target)

    lua_pop(lua, 4);                                                // pop(arg1, arg2, registry, fenv)
    CHECK_TOP(lua, -2);
}

void Interpolator::stop(lua_State * lua)
{
    SAVE_TOP(lua);
    assert(lua_gettop(lua) >= 1);
    assert(lua_is<Interpolator>(lua, -1));

    auto & interpolator = lua_to<Interpolator>(lua, -1);

    lua_getfenv(lua, -1);                                   // push(fenv)
    pushRegistry(lua);                                      // push(registry)

    // unregister interpolator from running list
    luaL_unref(lua, -1, interpolator.id);
    interpolator.id = LUA_NOREF;

    // remove reference to the render target
    lua_pushnil(lua);                                       // push(nil)
    lua_setfield(lua, -3, targetLink);                      // pop(nil)

    lua_pop(lua, 3);                                        // pop(interpolator, fenv, registry)
    CHECK_TOP(lua, -1);
}

void Interpolator::stepAll(lua_State * lua, unsigned ms)
{
    SAVE_TOP(lua);
    pushRegistry(lua);                                              // push(registry)
    size_t size = lua_objlen(lua, -1);

    for (size_t index = 1; index <= size; ++index) {
        lua_rawgeti(lua, -1, index);                                // push(interpolator)
        if (lua_is<Interpolator>(lua, -1)) {
            auto & interpolator = lua_to<Interpolator>(lua, -1);

            lua_getfenv(lua, -1);                                   // push(fenv)
            lua_getfield(lua, -1, targetLink);                      // push(target)
            auto * target = lua_to<RenderTarget *>(lua, -1);
            assert(target);
            lua_pop(lua, 2);                                        // pop(fenv, target)

            interpolator.elapsed += ms;
            if (interpolator.elapsed >= interpolator.duration) {
                (*target)[interpolator.index] = interpolator.finishValue;
                Interpolator::stop(lua);                            // pop(interpolator)
                continue;
            }

            (*target)[interpolator.index] = interpolator.value();
        }
        lua_pop(lua, 1);                                            // pop(interpolator)
    }

    lua_pop(lua, 1);                                                // pop(registry)
    CHECK_TOP(lua, 0);
}

RGBAColor Interpolator::value() const
{
    using ct = RGBAColor::channel_type;
    return {
        ct(int(startValue.red) +
           (int(finishValue.red) - int(startValue.red)) * int(elapsed) / int(duration)),
        ct(int(startValue.green) +
           (int(finishValue.green) - int(startValue.green)) * int(elapsed) / int(duration)),
        ct(int(startValue.blue) +
           (int(finishValue.blue) - int(startValue.blue)) * int(elapsed) / int(duration)),
        ct(int(startValue.alpha) +
           (int(finishValue.alpha) - int(startValue.alpha)) * int(elapsed) / int(duration))
    };
}

/****************************************************************************/

const char * metatable<Interpolator>::name = "Interpolator";
const struct luaL_Reg metatable<Interpolator>::meta_methods[] = {
    { "__index",        index },
    { nullptr,          nullptr}
};

} } // namespace keyleds::lua
