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
#include "tools/FileWatcher.h"

#include "logging.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <dirent.h>
#include <system_error>
#include <unistd.h>

LOGGING("filewatcher");

using tools::FileWatcher;

/****************************************************************************/

struct FileWatcher::Watch final
{
    watch_id    id;
    Listener    callback;
};

/****************************************************************************/

static int inotify_init_throw(int flags)
{
    int fd = inotify_init1(flags);
    if (fd < 0) { throw std::system_error(errno, std::generic_category()); }
    return fd;
}


FileWatcher::FileWatcher(uv_loop_t & loop)
 : m_fd(inotify_init_throw(IN_NONBLOCK | IN_CLOEXEC)),
   m_fdWatcher(m_fd, tools::FDWatcher::Read,
               std::bind(&FileWatcher::onNotifyReady, this), loop)
{}

FileWatcher::~FileWatcher()
{
    assert(m_listeners.empty());
    if (m_fd >= 0) { close(m_fd); }
}


FileWatcher::subscription FileWatcher::subscribe(const std::string & path,
                                                 Event events, Listener listener)
{
    watch_id wd = inotify_add_watch(m_fd, path.c_str(), static_cast<uint32_t>(events));
    if (wd < 0) {
        throw std::system_error(errno, std::generic_category());
    }
    m_listeners.push_back({wd, std::move(listener)});
    DEBUG("subscribed to events on ", path, " => ", wd);
    return subscription(*this, wd);
}

void FileWatcher::unsubscribe(watch_id wd)
{
    auto it = std::find_if(m_listeners.begin(), m_listeners.end(),
                           [wd](const auto & listener) { return listener.id == wd; });
    assert(it != m_listeners.end());

    inotify_rm_watch(m_fd, wd);

    if (it != m_listeners.end() - 1) { *it = std::move(m_listeners.back()); }
    m_listeners.pop_back();
    DEBUG("unsubscribed from events => ", wd);
}

void FileWatcher::onNotifyReady()
{
    // We use a union to allocate the buffer on the stack
    union {
        struct inotify_event    event;
        // cppcheck-suppress unusedStructMember
        char                    reserve[sizeof(struct inotify_event) + NAME_MAX + 1];
    } buffer;
    auto & event = buffer.event;

    ssize_t nread;
    while ((nread = read(m_fd, &event, sizeof(buffer))) >= 0) {
        auto it = std::find_if(
            m_listeners.begin(), m_listeners.end(),
            [&event](const auto & listener) { return listener.id == event.wd; }
        );
        if (it == m_listeners.end()) { continue; }
        INFO("Got event for ", it->id, ": ", std::string(event.name, event.len));
        it->callback(static_cast<Event>(event.mask), event.cookie,
                     std::string(event.name, event.len));
        //NOTE at that point `it` is invalid, because callback is allowed to unsubscribe
    }

    if (errno != EAGAIN) {
        ERROR("read on inotify fd returned error ", errno);
    }
}

FileWatcher::subscription & FileWatcher::subscription::operator=(subscription && other) noexcept
{
    if (m_id != invalid_watch) { m_watcher->unsubscribe(m_id); }
    m_watcher = other.m_watcher;
    m_id = other.m_id;
    other.m_id = invalid_watch;
    return *this;
}

FileWatcher::subscription::~subscription()
{
    if (m_id != invalid_watch) { m_watcher->unsubscribe(m_id); }
}
