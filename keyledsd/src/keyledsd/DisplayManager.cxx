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

#include <QSocketNotifier>
#include <cassert>
#include "tools/XContextWatcher.h"
#include "tools/XInputWatcher.h"

using xlib::XContextWatcher;
using xlib::XInputWatcher;
using keyleds::DisplayManager;

/****************************************************************************/

DisplayManager::DisplayManager(std::unique_ptr<xlib::Display> display, QObject *parent)
 : QObject(parent),
   m_display(std::move(display)),
   m_contextWatcher(std::make_unique<XContextWatcher>(*m_display)),
   m_inputWatcher(std::make_unique<XInputWatcher>(*m_display))
{
    assert(m_display != nullptr);
    auto * notifier = new QSocketNotifier(m_display->connection(), QSocketNotifier::Read, this);
    QObject::connect(notifier, &QSocketNotifier::activated,
                     [this](int){ m_display->processEvents(); });
    QObject::connect(m_contextWatcher.get(), &XContextWatcher::contextChanged,
                     this, &DisplayManager::onContextChanged);
    QObject::connect(m_inputWatcher.get(), &XInputWatcher::keyEventReceived,
                     this, &DisplayManager::onKeyEventReceived);
}

DisplayManager::~DisplayManager()
{}

void DisplayManager::scanDevices()
{
    m_inputWatcher->scan();
}

void DisplayManager::onContextChanged(const xlib::XContextWatcher::context_map & context)
{
    m_context = context;
    emit contextChanged(m_context);
}

void DisplayManager::onKeyEventReceived(const std::string & devNode, int key, bool press)
{
    emit keyEventReceived(devNode, key, press);
}
