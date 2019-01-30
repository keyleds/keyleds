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
#ifndef TOOLS_FILE_WATCHER_H_3F146693
#define TOOLS_FILE_WATCHER_H_3F146693

#include <sys/inotify.h>
#include <QObject>
#include <functional>
#include <vector>

namespace tools {

/****************************************************************************/

class FileWatcher final : public QObject
{
    Q_OBJECT
public:
    enum Event : uint32_t {
        Access = IN_ACCESS,
        Attrib = IN_ATTRIB,
        CloseWrite = IN_CLOSE_WRITE,
        CloseNoWrite = IN_CLOSE_NOWRITE,
        Create = IN_CREATE,
        Delete = IN_DELETE,
        DeleteSelf = IN_DELETE_SELF,
        Modify = IN_MODIFY,
        MoveSelf = IN_MOVE_SELF,
        MovedFrom = IN_MOVED_FROM,
        MovedTo = IN_MOVED_TO,
        Open = IN_OPEN,
        ExcludeUnlinked = IN_EXCL_UNLINK,
        Unmounted = IN_UNMOUNT,

        Ignored = IN_IGNORED,
        IsDirectory = IN_ISDIR
    };

private:
    using watch_id = int;
    using Listener = std::function<void(Event mask, uint32_t cookie, std::string path)>;
    static constexpr watch_id invalid_watch = -1;

    struct Watch;
    using listener_list = std::vector<Watch>;
public:
    class subscription final
    {
        FileWatcher *   m_watcher = nullptr;
        watch_id        m_id = invalid_watch;
    public:
                    subscription() = default;
                    subscription(FileWatcher & watcher, watch_id id)
                     : m_watcher(&watcher), m_id(id) {}
                    subscription(subscription && other) noexcept
                     : m_watcher(other.m_watcher)
                     { std::swap(m_id, other.m_id); }
        subscription& operator=(subscription &&) noexcept;
                    ~subscription();
    };

public:
    explicit            FileWatcher(QObject *parent = nullptr);
                        FileWatcher(const FileWatcher &) = delete;
    FileWatcher &       operator=(const FileWatcher &) = delete;
                        ~FileWatcher() override;

    subscription        subscribe(const std::string & path, Event events, Listener);

private:
    /// Invoked whenever system notifications from udev become available
    void                onNotifyReady(int socket);
    /// Invoked by subscription destructors
    void                unsubscribe(watch_id);
private:
    int                 m_fd = -1;      ///< file descriptor of inotify device
    listener_list       m_listeners;    ///< list of registered watches
};

/****************************************************************************/

} // namespace tools

#endif
