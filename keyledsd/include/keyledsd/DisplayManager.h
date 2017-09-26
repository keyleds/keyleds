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

namespace xlib {
    class Display;
    class XInputWatcher;
}

namespace keyleds {

class Context;
class XContextWatcher;

/** Main display manager
 *
 * Centralizes all operations and information for a specific display.
 */
class DisplayManager final : public QObject
{
    Q_OBJECT
public:
                    DisplayManager(std::unique_ptr<xlib::Display>, QObject *parent = nullptr);
                    ~DisplayManager() override;

    xlib::Display & display() { return *m_display; }

    void            scanDevices();
    const Context & currentContext() const;

signals:
    void            contextChanged(const keyleds::Context &);
    void            keyEventReceived(const std::string & devNode, int key, bool press);

private slots:
    void            onContextChanged(const keyleds::Context &);
    void            onKeyEventReceived(const std::string & devNode, int key, bool press);

private:
    std::unique_ptr<xlib::Display>          m_display;
    std::unique_ptr<XContextWatcher>        m_contextWatcher;
    std::unique_ptr<xlib::XInputWatcher>    m_inputWatcher;
};

};

#endif
