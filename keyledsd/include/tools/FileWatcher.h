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

/****************************************************************************/

#include <sys/inotify.h>
#include <QObject>
#include <functional>
#include <vector>

class FileWatcher final : public QObject
{
    Q_OBJECT
public:
    enum event : uint32_t {
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

    using watch_id = int;
    using Listener = std::function<void(event mask, uint32_t cookie, std::string path)>;
    static constexpr watch_id invalid_watch = -1;

    class subscription final
    {
        FileWatcher *   m_watcher;
        watch_id        m_id;
    public:
                    subscription()
                     : m_watcher(nullptr), m_id(invalid_watch) {}
                    subscription(FileWatcher & watcher, watch_id id)
                     : m_watcher(&watcher), m_id(id) {}
                    subscription(subscription && other) noexcept
                     : m_watcher(other.m_watcher), m_id(invalid_watch)
                     { std::swap(m_id, other.m_id); }
        subscription& operator=(subscription &&);
                    ~subscription();
    };
private:
    struct Watch;
    using listener_list = std::vector<Watch>;

public:
                        FileWatcher(QObject *parent = nullptr);
                        ~FileWatcher() override;
                        FileWatcher(const FileWatcher &) = delete;

    subscription        subscribe(const std::string & path, event events, Listener);

private:
    /// Invoked whenever system notifications from udev become available
    void                onNotifyReady(int socket);
    /// Invoked by subscription destructors
    void                unsubscribe(watch_id);
private:
    int                 m_fd;           ///< file descriptor of inotify device
    listener_list       m_listeners;    ///< list of registered watches
};

/****************************************************************************/

#endif
