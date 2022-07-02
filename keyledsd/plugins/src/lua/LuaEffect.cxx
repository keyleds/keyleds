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
#include "lua/LuaEffect.h"

#include "lua/Environment.h"
#include "lua/lua_common.h"
#include <algorithm>
#include <array>
#include <cassert>
#include <cstring>
#include <lua.hpp>
#include <sstream>

using keyleds::plugin::lua::LuaEffect;
using namespace keyleds::lua;

/****************************************************************************/
// Constants defining LUA environment

// LUA libraries to load
static constexpr std::array<lua_CFunction, 4> loadModules = {{
    luaopen_base, luaopen_math, luaopen_string, luaopen_table,
}};
static_assert(loadModules.back() == luaopen_table,
              "unexpected last element, is size correct?");

// Symbols not in this list get removed once libraries are loaded
static constexpr std::array<const char *, 25> globalWhitelist = {{
    // Libraries
    "coroutine", "math", "string", "table",
    // Functions
    "assert", "error", "getmetatable", "ipairs",
    "next", "pairs", "pcall", "print",
    "rawequal", "rawget", "rawset", "select",
    "setmetatable", "tonumber", "tostring", "type",
    "unpack", "wait", "xpcall",
    // Values
    "_G", "_VERSION"
}};

static void * const threadToken = const_cast<void **>(&threadToken);

/****************************************************************************/
// Helper functions

static int luaPanicHandler(lua_State *);
static int luaErrorHandler(lua_State *);

/****************************************************************************/
// Lifecycle management

LuaEffect::LuaEffect(std::string name, EffectService & service, state_ptr state)
 : m_name(std::move(name)),
   m_service(service),
   m_state(std::move(state)),
   m_enabled(true)
{}

LuaEffect::~LuaEffect() = default;

std::unique_ptr<LuaEffect> LuaEffect::create(const std::string & name, EffectService & service,
                                             const std::string & code)
{
    // Create a LUA state
    state_ptr state(luaL_newstate());
    auto * lua = state.get();
    lua_atpanic(lua, luaPanicHandler);

    SAVE_TOP(lua);

    // Load script
    if (luaL_loadbuffer(lua, code.data(), code.size(), name.c_str()) != 0) {
        service.log(logging::error::value, lua_tostring(lua, -1));
        return nullptr;
    }                                       // ^push (script)

    auto effect = std::make_unique<LuaEffect>(name, service, std::move(state));
    effect->setupState();

    // Run script to let it build its environment
    lua_pushcfunction(lua, luaErrorHandler);// push (errhandler)
    lua_insert(lua, -2);                    // swap (script, errhandler) => (errhandler, script)
    if (!handleError(lua, service, lua_pcall(lua, 0, 0, -2))) { // pop (errhandler, script)
        return nullptr;
    }

    CHECK_TOP(lua, 0);

    // Let the effect run init hook
    effect->init();
    return effect;
}

void LuaEffect::setupState()
{
    auto lua = m_state.get();
    SAVE_TOP(lua);

    // Load libraries in default environment
    for (const auto & module : loadModules) {
        lua_pushcfunction(lua, module);
        lua_call(lua, 0, 0);
    }

    // Remove global symbols not in whitelist
    lua_pushnil(lua);
    while (lua_next(lua, LUA_GLOBALSINDEX) != 0) {
        lua_pop(lua, 1);
        if (lua_isstring(lua, -1)) {
            const char * key = lua_tostring(lua, -1);
            auto it = std::find_if(globalWhitelist.begin(), globalWhitelist.end(),
                                   [key](const auto * item) { return std::strcmp(key, item) == 0; });
            if (it == globalWhitelist.end()) {
                lua_pushnil(lua);
                lua_setglobal(lua, key);
            }
        }
    }

    // Load keyleds library, passing ourselves as controller
    Environment(lua).openKeyleds(this);

    // Add debug module if configuration requests it
    if (getConfig<bool>(m_service, "debug").value_or(false)) {
        lua_pushcfunction(lua, luaopen_debug);
        lua_call(lua, 0, 0);
    }

    // Insert thread list
    lua_pushlightuserdata(lua, threadToken);
    lua_newtable(lua);
    lua_rawset(lua, LUA_REGISTRYINDEX);

    // Set keyleds members
    lua_createtable(lua, 0, 6);
    lua_pushvalue(lua, -1);
    lua_setglobal(lua, "keyleds");
    {
        lua_pushlstring(lua, m_service.deviceName().data(), m_service.deviceName().size());
        lua_setfield(lua, -2, "deviceName");
        lua_pushlstring(lua, m_service.deviceModel().data(), m_service.deviceModel().size());
        lua_setfield(lua, -2, "deviceModel");
        lua_pushlstring(lua, m_service.deviceSerial().data(), m_service.deviceSerial().size());
        lua_setfield(lua, -2, "deviceSerial");

        auto & config = m_service.configuration();
        lua_createtable(lua, 0, static_cast<int>(config.size()));
        for (const auto & item : config) {
            lua_pushlstring(lua, item.first.data(), item.first.size());
            if (std::holds_alternative<std::string>(item.second)) {
                const auto & value = std::get<std::string>(item.second);
                lua_pushlstring(lua, value.data(), value.size());
            } else if (std::holds_alternative<std::vector<std::string>>(item.second)) {
                const auto & values = std::get<std::vector<std::string>>(item.second);
                lua_createtable(lua, int(values.size()), 0);
                for (std::size_t idx = 0; idx < values.size(); ++idx) {
                    lua_pushlstring(lua, values[idx].data(), values[idx].size());
                    lua_rawseti(lua, -2, int(idx));
                }
            } else {
                assert(false);
            }
            lua_rawset(lua, -3);
        }
        lua_setfield(lua, -2, "config");

        auto & groups = m_service.keyGroups();
        lua_createtable(lua, 0, static_cast<int>(groups.size()));
        for (const auto & group : groups) {
            lua_pushlstring(lua, group.name().data(), group.name().size());
            lua_push(lua, &group);
            lua_rawset(lua, -3);
        }
        lua_setfield(lua, -2, "groups");

        lua_push(lua, &m_service.keyDB());
        lua_setfield(lua, -2, "db");
    }
    lua_pop(lua, 1);        // pop(keyleds)

    CHECK_TOP(lua, 0);
}

/****************************************************************************/
// Hooks

void LuaEffect::init()
{
    if (!m_enabled) { return; }
    auto lua = m_state.get();
    SAVE_TOP(lua);

    if (pushHook(lua, "init")) {                    // push(init)
        lua_pushcfunction(lua, luaErrorHandler);    // push(errhandler)
        lua_insert(lua, -2);                        // swap(init, errhandler) => (errhandler, init)
        if (!handleError(lua, m_service,
                         lua_pcall(lua, 0, 0, -2))) {// pop(errhandler, render)
            m_enabled = false;
        }
    }
    CHECK_TOP(lua, 0);
}

void LuaEffect::render(milliseconds elapsed, RenderTarget & target)
{
    if (!m_enabled) { return; }
    auto lua = m_state.get();

    Environment(lua).stepInterpolators(elapsed);
    stepThreads(elapsed);

    SAVE_TOP(lua);
    lua_push(lua, &target);                         // push(rendertarget)

    lua_pushcfunction(lua, luaErrorHandler);        // push(errhandler)
    if (pushHook(lua, "render")) {                  // push(render)
        lua_pushinteger(lua, lua_Integer(elapsed.count())); // push(arg1)
        lua_pushvalue(lua, -4);                     // push(arg2)
        if (!handleError(lua, m_service,
                         lua_pcall(lua, 2, 0, -4))) {// pop(errhandler, render, arg1, arg2)
            m_enabled = false;
        }
    } else {
        lua_pop(lua, 1);                            // pop(errhandler)
    }

    lua_to<RenderTarget *>(lua, -1) = nullptr;      // mark target as gone
    lua_pop(lua, 1);
    CHECK_TOP(lua, 0);
}

void LuaEffect::handleContextChange(const string_map & data)
{
    if (!m_enabled) { return; }
    auto lua = m_state.get();
    SAVE_TOP(lua);
    lua_pushcfunction(lua, luaErrorHandler);        // push(errhandler)
    if (pushHook(lua, "onContextChange")) {         // push(hook)
        lua_createtable(lua, 0, static_cast<int>(data.size())); // push table
        for (const auto & item : data) {
            lua_pushlstring(lua, item.first.c_str(), item.first.size());
            lua_pushlstring(lua, item.second.c_str(), item.second.size());
            lua_rawset(lua, -3);
        }
        if (!handleError(lua, m_service, lua_pcall(lua, 1, 0, -3))) { // pop(errhandler, hook, table)
            m_enabled = false;
        }
    } else {
        lua_pop(lua, 1);                            // pop(errhandler)
    }
    CHECK_TOP(lua, 0);
}

void LuaEffect::handleGenericEvent(const string_map & data)
{
    if (!m_enabled) { return; }
    auto lua = m_state.get();
    SAVE_TOP(lua);
    lua_pushcfunction(lua, luaErrorHandler);        // push(errhandler)
    if (pushHook(lua, "onGenericEvent")) {          // push(hook)
        lua_createtable(lua, 0, static_cast<int>(data.size())); // push table
        for (const auto & item : data) {
            lua_pushlstring(lua, item.first.c_str(), item.first.size());
            lua_pushlstring(lua, item.second.c_str(), item.second.size());
            lua_rawset(lua, -3);
        }
        if (!handleError(lua, m_service, lua_pcall(lua, 1, 0, -3))) { // pop(errhandler, hook, table)
            m_enabled = false;
        }
    } else {
        lua_pop(lua, 1);                            // pop(errhandler)
    }
    CHECK_TOP(lua, 0);
}

void LuaEffect::handleKeyEvent(const KeyDatabase::Key & key, bool press)
{
    if (!m_enabled) { return; }
    auto lua = m_state.get();
    SAVE_TOP(lua);
    lua_pushcfunction(lua, luaErrorHandler);        // push(errhandler)
    if (pushHook(lua, "onKeyEvent")) {              // push(hook)
        lua_push(lua, &key);                        // push(arg1)
        lua_pushboolean(lua, press);                // push(arg2)
        if (!handleError(lua, m_service,
                         lua_pcall(lua, 2, 0, -4))) {// pop(errhandler, hook, arg1, arg2)
            m_enabled = false;
        }
    } else {
        lua_pop(lua, 1);                            // pop(errhandler)
    }
    CHECK_TOP(lua, 0);
}

/****************************************************************************/
// Lua interface

void LuaEffect::print(const std::string & msg) const
{
    m_service.log(logging::info::value, msg.c_str());
}

std::optional<keyleds::RGBAColor> LuaEffect::parseColor(const std::string & str) const
{
    const auto & colors = m_service.colors();
    auto it = std::find_if(colors.begin(), colors.end(),
                           [&str](auto & item) { return item.first == str; });
    if (it != colors.end()) { return it->second; }
    return RGBAColor::parse(str);
}

keyleds::RenderTarget * LuaEffect::createRenderTarget()
{
    return m_service.createRenderTarget();
}

void LuaEffect::destroyRenderTarget(RenderTarget * target)
{
    m_service.destroyRenderTarget(target);
}

/// pushes the thread onto the stack - [-nargs-1 ; +1]
int LuaEffect::createThread(lua_State * lua, int nargs)
{
    SAVE_TOP(lua);
    lua_push(lua, Thread{0, true, Thread::milliseconds::zero()});   // push(thread)

    lua_createtable(lua, 0, 1);                     // push(fenv)
    auto * thread = lua_newthread(m_state.get());   // push(thread)
    lua_setfield(lua, -2, "thread");                // pop(thread)
    lua_setfenv(lua, -2);                           // pop(fenv)

    lua_pushlightuserdata(lua, threadToken); // push(token)
    lua_rawget(lua, LUA_REGISTRYINDEX);             // pop(token) push(threadlist)
    lua_pushvalue(lua, -2);                         // push(thread)
    auto id = luaL_ref(lua, -2);                    // pop(thread)
    lua_to<Thread>(lua, -2).id = id;
    lua_pop(lua, 1);                                // pop(threadlist)

    int itemsToMove = 1 + nargs;
    lua_insert(lua, -1 - itemsToMove);              // pop(thread) => (thread, args...)
    lua_xmove(lua, thread, itemsToMove);            // pop(args...)
    runThread(lua_to<Thread>(lua, -1), thread, nargs);

    CHECK_TOP(lua, -1 - nargs + 1);
    return 1;
}

void LuaEffect::destroyThread(lua_State * lua, Thread & thread)
{
    SAVE_TOP(lua);
    lua_pushlightuserdata(lua, threadToken);
    lua_rawget(lua, LUA_REGISTRYINDEX);

    luaL_unref(lua, -1, thread.id);
    lua_pop(lua, 1);
    thread.running = false;
    CHECK_TOP(lua, 0);
}

void LuaEffect::stepThreads(milliseconds elapsed)
{
    auto * lua = m_state.get();
    SAVE_TOP(lua);
    lua_pushlightuserdata(lua, threadToken);
    lua_rawget(lua, LUA_REGISTRYINDEX);

    auto size = lua_objlen(lua, -1);
    assert(size <= std::numeric_limits<int>::max());
    for (int index = 1; index <= int(size); ++index) {
        lua_rawgeti(lua, -1, index);                    // push(threadInfo)
        if (!lua_is<Thread>(lua, -1)) {
            lua_pop(lua, 1);                            // (pop(threadInfo))
            continue;
        }

        auto & threadInfo = lua_to<Thread>(lua, -1);

        if (threadInfo.running) {
            if (threadInfo.sleepTime <= elapsed) {
                lua_getfenv(lua, -1);                   // push(fenv)
                lua_getfield(lua, -1, "thread");        // push(thread)
                auto * thread = static_cast<lua_State *>(const_cast<void *>(lua_topointer(lua, -1)));

                while (threadInfo.running && threadInfo.sleepTime <= elapsed) {
                    runThread(threadInfo, thread, 0);
                }

                lua_pop(lua, 2);                        // pop(fenv, thread)
            }
            if (threadInfo.sleepTime > elapsed) {
                threadInfo.sleepTime -= elapsed;
            } else {
                threadInfo.sleepTime = Thread::milliseconds::zero();
            }
        }
        lua_pop(lua, 1);                                // pop(threadInfo)
    }
    lua_pop(lua, 1);                                    // pop(animlist)
    CHECK_TOP(lua, 0);
}

void LuaEffect::runThread(Thread & threadInfo, lua_State * thread, int nargs)
{
    auto * lua = m_state.get();
    SAVE_TOP(lua);

    bool terminate = true;
    switch (lua_resume(thread, nargs)) {
        case 0:
            break;
        case LUA_YIELD:
            if (lua_topointer(thread, 1) != const_cast<void *>(Environment::waitToken)) {
                luaL_traceback(lua, thread, "invalid yield", 0);
                m_service.log(logging::error::value, lua_tostring(lua, -1));
                lua_pop(lua, 1);
                break;
            }
            threadInfo.sleepTime += Thread::milliseconds(unsigned(1000.0 * lua_tonumber(thread, 2)));
            terminate = false;
            break;
        case LUA_ERRRUN:
            luaL_traceback(lua, thread, lua_tostring(thread, -1), 0);
            m_service.log(logging::error::value, lua_tostring(lua, -1));
            lua_pop(lua, 1);
            break;
        case LUA_ERRMEM:
            m_service.log(logging::error::value, "out of memory");
            break;
        default:
            m_service.log(logging::critical::value, "unexpected error");
    }
    if (terminate) {
        destroyThread(m_state.get(), threadInfo);
    }
    CHECK_TOP(lua, 0);
}

/****************************************************************************/
// Static Helper methods

bool LuaEffect::pushHook(lua_State * lua, const char * name)
{
    SAVE_TOP(lua);
    lua_getglobal(lua, name);               // push(hook)
    if (!lua_isfunction(lua, -1)) {
        lua_pop(lua, 1);                    // pop(hook)
        CHECK_TOP(lua, 0);
        return false;
    }
    CHECK_TOP(lua, +1);
    return true;
}

bool LuaEffect::handleError(lua_State * lua, EffectService & service, int code)
{
    bool ok = false;
    switch(code)
    {
    case 0:
        ok = true;
        break;  // no error
    case LUA_ERRRUN:
        service.log(1, lua_tostring(lua, -1));
        lua_pop(lua, 1);    // pop error message
        break;
    case LUA_ERRMEM:
        service.log(1, "out of memory");
        break;
    case LUA_ERRERR:
        service.log(0, "unexpected error");
        break;
    }
    lua_pop(lua, 1);        // pop error handler
    return ok;
}

void LuaEffect::lua_state_deleter::operator()(lua_State *p) const { lua_close(p); }

/****************************************************************************/

/// Convert a lua panic into abort - gives better messages than letting lua exit().
static int luaPanicHandler(lua_State *) { abort(); }

/// Builds the error message for script errors
static int luaErrorHandler(lua_State * lua)
{
    luaL_traceback(lua, lua, lua_tostring(lua, -1), 1);
    return 1;
}
