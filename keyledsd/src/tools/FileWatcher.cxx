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
#include <QSocketNotifier>
#include <unistd.h>
#include <cerrno>
#include <system_error>
#include "tools/FileWatcher.h"
#include "logging.h"

LOGGING("filewatcher");

/****************************************************************************/

struct FileWatcher::Watch final
{
    Listener        callback;
    void * const    userData;
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
    if (m_fd >= 0) { close(m_fd); }
}


FileWatcher::watch_id FileWatcher::subscribe(const std::string & path, event events,
                                             Listener listener, void * data)
{
    watch_id wd = inotify_add_watch(m_fd, path.c_str(), static_cast<uint32_t>(events));
    if (wd < 0) {
        throw std::system_error(errno, std::generic_category());
    }
    m_listeners.insert(std::make_pair(wd, Watch{listener, data}));
    return wd;
}

void FileWatcher::unsubscribe(watch_id wd)
{
    if (m_listeners.erase(wd) == 0) {
        throw std::logic_error("Unknown subscription");
    }
    inotify_rm_watch(m_fd, wd);
}

void FileWatcher::unsubscribe(Listener listener, void * data)
{
    for (auto it = m_listeners.begin(); it != m_listeners.end(); ++it) {
        if (it->second.callback == listener && it->second.userData == data) {
            inotify_rm_watch(m_fd, it->first);
            m_listeners.erase(it);
            return;
        }
    }
    throw std::logic_error("Unknown subscription");
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
        auto it = m_listeners.find(buffer.event.wd);
        if (it == m_listeners.end()) { continue; }
        const auto & watch = it->second;
        (*watch.callback)(watch.userData, static_cast<enum event>(buffer.event.mask),
                          buffer.event.cookie, std::string(buffer.event.name, buffer.event.len));
    }

    if (errno != EAGAIN) {
        ERROR("read on inotify fd returned error ", errno);
    }
}
