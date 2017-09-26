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

#include <QObject>
#include <string>
#include "tools/XWindow.h"

namespace xlib {

/** X-Input - device events watcher
 *
 * Watches device hierarchy changes and emits signals when keys are pressed.
 */
class XInputWatcher final : public QObject
{
    Q_OBJECT
    typedef std::map<xlib::Device::handle_type, xlib::Device> device_map;
public:
                    XInputWatcher(xlib::Display & display, QObject *parent = nullptr);
                    ~XInputWatcher() override;

    const xlib::Display & display() const { return m_display; }

public slots:
    void            scan();

signals:
    void            keyEventReceived(const std::string & devNode, int key, bool pressed);

protected:
    virtual void    handleEvent(const XEvent &);
    virtual void    onInputEnabled(xlib::Device::handle_type, int);
    virtual void    onInputDisabled(xlib::Device::handle_type, int);

private:
    static void     displayEventCallback(const XEvent &, void*);

protected:
    xlib::Display & m_display;          ///< X display connection
    int             m_XIopcode;         ///< XInput extension code
    device_map      m_devices;          ///< List of enabled devices
};


};

#endif
