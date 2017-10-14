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
#ifndef KEYLEDS_PLUGINS_LUA_LUA_COMMON_H_924F07E9
#define KEYLEDS_PLUGINS_LUA_LUA_COMMON_H_924F07E9

struct lua_State;
struct luaL_Reg;

namespace keyleds { namespace lua {

/****************************************************************************/

extern const char * const badKeyErrorMessage;
extern const char * const badTypeErrorMessage;
extern const char * const badIndexErrorMessage;
extern const char * const noLongerExistsErrorMessage;
extern const char * const noEffectTokenErrorMessage;

bool lua_handleMethodIndex(lua_State * lua, int index, const luaL_Reg[]);
int lua_keyError(lua_State * lua, int index);

/****************************************************************************/

#ifndef NDEBUG
#define SAVE_TOP(lua)           int saved_top_ = lua_gettop(lua)
#define CHECK_TOP(lua, depth)   assert(lua_gettop(lua) == saved_top_ + depth)
#else
#define SAVE_TOP(lua)
#define CHECK_TOP(lua, depth)
#endif

/****************************************************************************/

} } // namespace keyleds::lua

#endif
