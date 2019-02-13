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
#ifndef TOOLS_EVENT_H_4FBEBA6D
#define TOOLS_EVENT_H_4FBEBA6D

#include <cassert>
#include <functional>
#include <utility>
#include <type_traits>

struct uv_loop_s;
struct uv_poll_s;
using uv_loop_t = struct uv_loop_s;
using uv_poll_t = struct uv_poll_s;

namespace keyleds::tools {

/****************************************************************************/

template <typename ...Args>
class Callback final
{
public:
    using function_type = std::function<void(Args...)>;
public:
    template <typename T>
    void connect(T && listener)
    {
        static_assert(std::is_convertible_v<T, function_type>, "incorrect listener signature");
        assert(!m_value);
        m_value = std::forward<T>(listener);
    }

    void disconnect() { m_value = nullptr; }

    void emit(Args... args)
    {
        if (m_value) { m_value(std::forward<Args>(args)...); }
    }

private:
    function_type   m_value;
};

template <typename ...Args, typename T, typename Listener>
void connect(Callback<Args...> & event, const T *, Listener && listener)
{
    event.connect(std::forward<Listener>(listener));
}

template <typename ...Args, typename T>
void disconnect(Callback<Args...> & event, const T *)
{
    event.disconnect();
}


/****************************************************************************/

class FDWatcher final
{
public:
    enum events { Read = 1u, Write = 2u };

    FDWatcher(int fd, events, Callback<events>::function_type onReady, uv_loop_t &);
                FDWatcher(const FDWatcher &) = delete;
    FDWatcher & operator=(const FDWatcher &) = delete;
                ~FDWatcher();

    Callback<events>    ready;
private:
    static void fdNotifierCallback(uv_poll_t * handle, int status, int ev);
private:
    uv_poll_t * m_handle;   // lifecycle has to be deccorelated for async callback
};


/****************************************************************************/

} // namespace keyleds::tools

#endif
