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
#ifndef KEYLEDS_PLUGINS_LUA_LUA_THREAD_H_761E3512
#define KEYLEDS_PLUGINS_LUA_LUA_THREAD_H_761E3512

#include <chrono>
#include "lua/lua_types.h"

namespace keyleds::lua {

/****************************************************************************/

/** Lua coroutine-based thread.
 */
struct Thread
{
    using milliseconds = std::chrono::duration<unsigned, std::milli>;

    int             id;         ///< unique identifier
    bool            running;    ///< whether the thread is currently running (schedulable)
    milliseconds    sleepTime;  ///< time until thread should be awoken
};

int luaNewThread(lua_State *);

/// Registration of thread as a lua object
template <> struct metatable<Thread>
    { static const char * const name; static const struct luaL_Reg methods[];
      static const struct luaL_Reg meta_methods[]; struct weak_table : std::false_type{}; };

/****************************************************************************/

} // namespace keyleds::lua

#endif
