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
#ifndef KEYLEDS_PLUGINS_LUA_LUA_KEYLEDS_H_3EAF7EA0
#define KEYLEDS_PLUGINS_LUA_LUA_KEYLEDS_H_3EAF7EA0

#include <optional>
#include <string>
#include "lua/lua_Interpolator.h"
#include "lua/lua_Key.h"
#include "lua/lua_KeyDatabase.h"
#include "lua/lua_KeyGroup.h"
#include "lua/lua_RGBAColor.h"
#include "lua/lua_RenderTarget.h"
#include "lua/lua_Thread.h"

struct lua_State;
namespace keyleds { struct RGBAColor; }
namespace keyleds::device { class RenderTarget; }

namespace keyleds::lua {

struct Thread;

/****************************************************************************/

/// Synctactical sugar for manipulating lua_State: Environment(lua).controller()
class Environment final
{
public:
    class Controller
    {
    protected:
        using Thread = keyleds::lua::Thread;
    public:
        virtual void            print(const std::string &) const = 0;
        virtual std::optional<RGBAColor> parseColor(const std::string &) const = 0;

        virtual RenderTarget *  createRenderTarget() = 0;
        virtual void            destroyRenderTarget(RenderTarget *) = 0;

        virtual int             createThread(lua_State * lua, int nargs) = 0;
        virtual void            destroyThread(lua_State * lua, Thread &) = 0;
    protected:
        ~Controller() {}
    };

public:
                    Environment(lua_State * lua) : m_lua(lua) {}

    void            openKeyleds(Controller *);
    Controller *    controller() const;

    void            stepInterpolators(Interpolator::milliseconds elapsed)
        { Interpolator::stepAll(m_lua, elapsed); }

    static const void * const waitToken;
private:
    lua_State *     m_lua;
};

/****************************************************************************/

} // namespace keyleds::lua

#endif
