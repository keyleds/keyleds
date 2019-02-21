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
#ifndef KEYLEDSD_DISPLAYMANAGER_H_2ADCBC2A
#define KEYLEDSD_DISPLAYMANAGER_H_2ADCBC2A
#ifndef KEYLEDSD_INTERNAL
#   error "Internal header - must not be pulled into plugins"
#endif

#include "keyledsd/tools/Event.h"
#include "keyledsd/tools/XContextWatcher.h"
#include "keyledsd/tools/XInputWatcher.h"
#include <memory>
#include <string>
#include <utility>
#include <vector>

namespace keyleds::tools::xlib {
    class Display;
    class XInputWatcher;
}

namespace keyleds::service {

/****************************************************************************/

/** Main display manager
 *
 * Centralizes all operations and information for a specific display.
 */
class DisplayManager final
{
    using Display = tools::xlib::Display;
    using XContextWatcher = tools::xlib::XContextWatcher;
    using XInputWatcher = tools::xlib::XInputWatcher;
    using context_map = std::vector<std::pair<std::string, std::string>>;
public:
                    DisplayManager(std::unique_ptr<Display>, uv_loop_t &);
                    ~DisplayManager();

    Display &       display() { return *m_display; }

    void            scanDevices();
    const context_map & currentContext() const { return m_context; }

    // signals
    tools::Callback<const context_map &>            contextChanged;
    tools::Callback<const std::string &, int, bool> keyEventReceived;

private:
    /// Receives notifications from m_contextWatcher. Forwards them through contextChanged signal.
    void            onContextChanged(const XContextWatcher::context_map &);

    /// Receives notifications from m_inputWatcher. Forwards them through keyEventReceived signal.
    void            onKeyEventReceived(const std::string & devNode, int key, bool press);

private:
    std::unique_ptr<Display>    m_display;          ///< Connection to X display
    XContextWatcher             m_contextWatcher;   ///< Watches window events
    XInputWatcher               m_inputWatcher;     ///< Watches keypresses
    tools::FDWatcher            m_fdWatcher;        ///< Monitors display socket
    context_map                 m_context;          ///< Current context values
};

/****************************************************************************/

} // namespace keyleds::service

#endif
