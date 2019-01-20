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
#ifndef KEYLEDS_PLUGINS_LUA_LUA_INTERPOLATOR_H_BCD195FC
#define KEYLEDS_PLUGINS_LUA_LUA_INTERPOLATOR_H_BCD195FC

#include <chrono>
#include "lua/lua_types.h"
#include "keyledsd/PluginHelper.h"

namespace keyleds { namespace lua {

/****************************************************************************/

/** Color interpolator for animating keys.
 * This is a lua userdata-based object created using `fade()` from lua.
 */
struct Interpolator
{
    enum {
        hasStartValueFlag = (1 << 1)
    };
    using milliseconds = std::chrono::duration<unsigned, std::milli>;

    int             id;             ///< luaL_ref id in registry
    int             flags;          ///< See flags_type above
    unsigned        index;          ///< Key index withing render target
    milliseconds    duration;       ///< Animation duration in ms
    milliseconds    elapsed;        ///< Elapsed time in ms
    RGBAColor       startValue;     ///< Color when elapsed == 0
    RGBAColor       finishValue;    ///< Color when elapsed >= duration

    static void start(lua_State *, unsigned index); // on stack: (interpolator, rendertarget) [-2, 0]
    static void stop(lua_State *);                  // on stack: (interpolator) [-1, 0]
    static void stepAll(lua_State *, milliseconds);

    RGBAColor   value() const;
};

int luaNewInterpolator(lua_State *);

/// Registration of Interpolator as lua object
template <> struct metatable<Interpolator>
    { static const char * name; static constexpr struct luaL_Reg * methods = nullptr;
      static const struct luaL_Reg meta_methods[]; struct weak_table : std::false_type{}; };

/****************************************************************************/

} } // namespace keyleds::lua

#endif
