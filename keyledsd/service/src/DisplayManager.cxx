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
#include "keyledsd/DisplayManager.h"

#include <functional>

using keyleds::DisplayManager;

/****************************************************************************/

DisplayManager::DisplayManager(std::unique_ptr<Display> display, uv_loop_t & loop)
 : m_display(std::move(display)),
   m_contextWatcher(*m_display),
   m_inputWatcher(*m_display),
   m_fdWatcher(m_display->connection(), tools::FDWatcher::Read,
               [this](auto){ m_display->processEvents(); }, loop),
   m_context(m_contextWatcher.current())
{
    using namespace std::placeholders;
    m_contextWatcher.contextChanged.connect(std::bind(
        &DisplayManager::onContextChanged, this, _1
    ));
    m_inputWatcher.keyEventReceived.connect(std::bind(
        &DisplayManager::onKeyEventReceived, this, _1, _2, _3
    ));
}

DisplayManager::~DisplayManager()
{
    m_contextWatcher.contextChanged.disconnect();
    m_inputWatcher.keyEventReceived.disconnect();
}

void DisplayManager::scanDevices()
{
    m_inputWatcher.scan();
}

void DisplayManager::onContextChanged(const XContextWatcher::context_map & context)
{
    m_context = context;
    contextChanged.emit(m_context);
}

void DisplayManager::onKeyEventReceived(const std::string & devNode, int key, bool press)
{
    keyEventReceived.emit(devNode, key, press);
}
