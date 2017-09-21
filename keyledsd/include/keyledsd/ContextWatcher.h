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
#ifndef KEYLEDSD_CONTEXTWATCHER_H_1CF5FE0A
#define KEYLEDSD_CONTEXTWATCHER_H_1CF5FE0A

#include <QObject>
#include <memory>
#include <string>
#include "keyledsd/Context.h"
#include "tools/XWindow.h"

namespace keyleds {

/** X-Window - based context watcher
 *
 * Generates a Context from a X display connection, by tracking active window.
 * The generated context contains active window's class, instance, title and id
 * and gets updated as the watcher receives events through the X display connection.
 *
 * Every context update generates a contextChanged signal.
 */
class XContextWatcher : public QObject
{
    Q_OBJECT
public:
                    XContextWatcher(xlib::Display & display, QObject *parent = nullptr);
                    ~XContextWatcher() override;

    const Context & current() const noexcept { return m_context; }

    const xlib::Display & display() const { return m_display; }

signals:
    void            contextChanged(const keyleds::Context &);

protected:
    virtual void    handleEvent(const XEvent &);
    virtual void    onActiveWindowChanged(xlib::Window *);
    void            setContext(xlib::Window *);

private:
    static void     displayEventCallback(const XEvent &, void*);

protected:
    xlib::Display &                 m_display;          ///< X display connection
    std::unique_ptr<xlib::Window>   m_activeWindow;     ///< Currently active window, or nullptr if none
    Context                         m_context;          ///< Current context
};


};

#endif
