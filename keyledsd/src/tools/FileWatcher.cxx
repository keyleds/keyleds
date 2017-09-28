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
#include <unistd.h>
#include <QSocketNotifier>
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <system_error>
#include "tools/FileWatcher.h"
#include "logging.h"

LOGGING("filewatcher");

/****************************************************************************/

struct FileWatcher::Watch final
{
    watch_id    id;
    Listener    callback;
    void *      userData;
};

/****************************************************************************/

FileWatcher::FileWatcher(QObject * parent)
 : QObject(parent),
   m_fd(-1)
{
    if ((m_fd = inotify_init1(IN_NONBLOCK | IN_CLOEXEC)) < 0) {
        throw std::system_error(errno, std::generic_category());
    }
    // this QObject takes automatic ownership of QSocketNotifier
    auto notifier = new QSocketNotifier(m_fd, QSocketNotifier::Read, this);
    QObject::connect(notifier, &QSocketNotifier::activated,
                     this, &FileWatcher::onNotifyReady);
}

FileWatcher::~FileWatcher()
{
    assert(m_listeners.empty());
    if (m_fd >= 0) { close(m_fd); }
}


FileWatcher::watch_id FileWatcher::subscribe(const std::string & path, event events,
                                             Listener listener, void * data)
{
    watch_id wd = inotify_add_watch(m_fd, path.c_str(), static_cast<uint32_t>(events));
    if (wd < 0) {
        throw std::system_error(errno, std::generic_category());
    }
    m_listeners.insert(
        std::upper_bound(m_listeners.begin(), m_listeners.end(), wd,
                         [](watch_id id, const auto & listener) { return id < listener.id; }),
        {wd, listener, data}
    );
    return wd;
}

void FileWatcher::unsubscribe(watch_id wd)
{
    auto it = std::find_if(m_listeners.begin(), m_listeners.end(),
                           [wd](const auto & listener) { return listener.id == wd; });
    if (it == m_listeners.end()) {
        throw std::invalid_argument("Unknown subscription");
    }
    inotify_rm_watch(m_fd, wd);
    m_listeners.erase(it);
}

void FileWatcher::unsubscribe(Listener callback, void * data)
{
    auto it = std::stable_partition(
        m_listeners.begin(), m_listeners.end(),
        [callback, data](const auto & listener) {
            return !(listener.callback == callback && listener.userData == data);
        }
    );

    std::for_each(it, m_listeners.end(),
                  [this](auto & listener) { inotify_rm_watch(m_fd, listener.id); });
    m_listeners.erase(it, m_listeners.end());
}

void FileWatcher::onNotifyReady(int)
{
    // We use a union to allocate the buffer on the stack
    union {
        struct inotify_event    event;
        char                    reserve[sizeof(struct inotify_event) + NAME_MAX + 1];
    } buffer;

    ssize_t nread;
    while ((nread = read(m_fd, &buffer, sizeof(buffer))) >= 0) {
        auto it = std::lower_bound(
            m_listeners.begin(), m_listeners.end(), buffer.event.wd,
            [](const auto & listener, watch_id id) { return listener.id < id; }
        );
        if (it == m_listeners.end()) { continue; }
        assert(it->id == buffer.event.wd);
        (*it->callback)(it->userData,
                        static_cast<enum event>(buffer.event.mask),
                        buffer.event.cookie,
                        std::string(buffer.event.name, buffer.event.len));
    }

    if (errno != EAGAIN) {
        ERROR("read on inotify fd returned error ", errno);
    }
}
