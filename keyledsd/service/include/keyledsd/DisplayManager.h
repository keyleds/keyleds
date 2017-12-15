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

#include <QObject>
#include <memory>
#include <string>
#include <utility>
#include <vector>
#include "tools/XContextWatcher.h"

namespace xlib {
    class Display;
    class XInputWatcher;
}

namespace keyleds {

/****************************************************************************/

/** Main display manager
 *
 * Centralizes all operations and information for a specific display.
 */
class DisplayManager final : public QObject
{
    Q_OBJECT
    using Display = xlib::Display;
    using XContextWatcher = xlib::XContextWatcher;
    using XInputWatcher = xlib::XInputWatcher;
    using context_map = std::vector<std::pair<std::string, std::string>>;
public:
                    DisplayManager(std::unique_ptr<Display>, QObject *parent = nullptr);
                    ~DisplayManager() override;

    Display &       display() { return *m_display; }

    void            scanDevices();
    const context_map & currentContext() const { return m_context; }

signals:
    void            contextChanged(const context_map &);
    void            keyEventReceived(const std::string & devNode, int key, bool press);

private:
    /// Receives notifications from m_contextWatcher. Forwards them through contextChanged signal.
    void            onContextChanged(const XContextWatcher::context_map &);

    /// Receives notifications from m_inputWatcher. Forwards them through keyEventReceived signal.
    void            onKeyEventReceived(const std::string & devNode, int key, bool press);

private:
    std::unique_ptr<Display>            m_display;          ///< Connection to X display
    std::unique_ptr<XContextWatcher>    m_contextWatcher;   ///< Watches window events
    std::unique_ptr<XInputWatcher>      m_inputWatcher;     ///< Watches keypresses
    context_map                         m_context;          ///< Current context values
};

/****************************************************************************/

} // namespace keyleds

#endif
