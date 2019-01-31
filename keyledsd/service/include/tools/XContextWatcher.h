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

#include "tools/Event.h"
#include "tools/XWindow.h"
#include <X11/Xlib.h>
#undef CursorShape
#undef Bool
#include <memory>
#include <utility>
#include <vector>

namespace xlib {

/****************************************************************************/

/** X-Window - based context watcher
 *
 * Generates a Context from a X display connection, by tracking active window.
 * The generated context contains active window's class, instance, title and id
 * and gets updated as the watcher receives events through the X display connection.
 *
 * Every context update generates a contextChanged signal.
 */
class XContextWatcher final
{
public:
    using context_map = std::vector<std::pair<std::string, std::string>>;
public:
                    XContextWatcher(Display & display);
                    ~XContextWatcher();

    const xlib::Display & display() const { return m_display; }

    const context_map & current() const noexcept { return m_context; }

    // signals
    tools::Callback<const context_map &>    contextChanged;

private:
    /// Invoked from the main X display event loop for Xinput events
    virtual void    handleEvent(const XEvent &);

    /// Invoked from handleEvent when it detects the active window has changed
    /// Passed window is the new active window; m_activeWindow still has the old one.
    /// Both may be nullptr.
    virtual void    onActiveWindowChanged(Window *, bool silent = false);

    /// Builds a context and emit a contextChanged signal for given window
    /// Signal is debounced: if built context is equal to current, signal is not sent.
    void            setContext(Window *);

private:
    Display &               m_display;          ///< X display connection
    Display::subscription   m_displayReg;       ///< callback registration for X events
    std::unique_ptr<Window> m_activeWindow;     ///< Currently active window, or nullptr if none
    context_map             m_context;          ///< Current context
};

/****************************************************************************/

};

#endif
