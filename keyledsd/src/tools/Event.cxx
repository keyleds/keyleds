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
#include "keyledsd/tools/Event.h"

#include <memory>
#include <uv.h>

using keyleds::tools::FDWatcher;

/****************************************************************************/

static void handleCloseCallback(uv_handle_t * ptr) { operator delete(ptr); }

FDWatcher::FDWatcher(int fd, events ev, Callback<events>::function_type onReady, uv_loop_t & loop)
{
    ready.connect(std::move(onReady));
    m_handle = reinterpret_cast<uv_poll_t *>(operator new(sizeof(uv_poll_t)));
    uv_poll_init(&loop, m_handle, fd);
    m_handle->data = this;

    auto mask = 0;
    if (ev & Read) { mask |= UV_READABLE; }
    if (ev & Write) { mask |= UV_WRITABLE; }
    uv_poll_start(m_handle, mask, fdNotifierCallback);
}

FDWatcher::~FDWatcher()
{
    uv_poll_stop(m_handle);
    uv_close(reinterpret_cast<uv_handle_t *>(m_handle), handleCloseCallback);
}

void FDWatcher::fdNotifierCallback(uv_poll_t * handle, int, int ev)
{
    auto mask = 0u;
    if (ev & UV_READABLE) { mask |= Read; }
    if (ev & UV_WRITABLE) { mask |= Write; }
    static_cast<FDWatcher *>(handle->data)->ready.emit(static_cast<events>(mask));
}
