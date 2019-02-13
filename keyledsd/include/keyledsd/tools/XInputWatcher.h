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
#ifndef TOOLS_XINPUTWATCHER_H_51CB4EAC
#define TOOLS_XINPUTWATCHER_H_51CB4EAC

#include "keyledsd/tools/Event.h"
#include "keyledsd/tools/XWindow.h"
#include <string>
#include <vector>

namespace keyleds::tools::xlib {

/****************************************************************************/

/** X-Input - device events watcher
 *
 * Watches device hierarchy changes and emits signals when keys are pressed.
 */
class XInputWatcher final
{
    using device_list = std::vector<Device>;
public:
                    XInputWatcher(Display & display);
                    ~XInputWatcher();

    const Display & display() const { return m_display; }

    void            scan();                 ///< Rescans Xinput devices actively

    // signals
    /// Emitted whenever a key event happens on any slave keyboard
    /// @param devNode the path to kernel device that the event originates from.
    /// @param key the key code, as sent by the kernel device
    /// @param pressed true if this indicates a keypress, otherwise it's a key release
    tools::Callback<const std::string &, int, bool> keyEventReceived;

protected:
    /// Invoked from the main X display event loop for Xinput events
    virtual void    handleEvent(const XEvent &);

    /// Invoked by handleEvent when a Xinput device becomes ready.
    virtual void    onInputEnabled(int deviceId, int use);

    /// Invoked by handleEvent when a Xinput device disappears.
    virtual void    onInputDisabled(int deviceId, int use);

protected:
    Display &       m_display;          ///< X display connection
    Display::subscription m_displayReg; ///< callback registration for X events
    int             m_XIopcode;         ///< XInput extension code
    device_list     m_devices;          ///< List of enabled devices
};

/****************************************************************************/

} // namespace keyleds::tools::xlib

#endif
