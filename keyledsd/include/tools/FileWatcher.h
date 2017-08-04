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

#include <QObject>
#include <sys/inotify.h>
#include <map>

class FileWatcher : public QObject
{
    Q_OBJECT
public:
    enum class event : uint32_t {
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
        Unmounted = IN_UNMOUNT
    };

    typedef int watch_id;
    typedef void (*Listener)(void * userData, event mask, uint32_t cookie, std::string path);

private:
    struct Watch final
    {
        Listener        callback;
        void * const    userData;
    };

    typedef std::map<watch_id, Watch> listener_map;

public:
                        FileWatcher(QObject *parent = nullptr);
                        ~FileWatcher() override;

    watch_id            subscribe(const std::string & path, event events, Listener, void * data);
    void                unsubscribe(watch_id);
    void                unsubscribe(Listener, void * data);

private slots:
    void                onNotifyReady(int socket);

private:
    int                 m_fd;
    listener_map        m_listeners;
};

/****************************************************************************/

#endif
