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
#include <X11/extensions/XInput2.h>
#undef CursorShape
#undef Bool
#include <cassert>
#include <vector>
#include "tools/XInputWatcher.h"
#include "logging.h"

#define MIN_KEYCODE 8

LOGGING("xinput-watcher");

using xlib::XInputWatcher;
static const char XInputExtensionName[] = "XInputExtension";

/****************************************************************************/

XInputWatcher::XInputWatcher(xlib::Display & display, QObject *parent)
 : QObject(parent),
   m_display(display)
{
    int event, error;
    if (!XQueryExtension(display.handle(), XInputExtensionName, &m_XIopcode, &event, &error)) {
        throw std::runtime_error("XInput extension not available");
    }

    std::vector<unsigned char> mask(XIMaskLen(XI_LASTEVENT), 0);
    XIEventMask eventMask = { XIAllDevices, (int)mask.size(), mask.data() };
    XISetMask(mask.data(), XI_HierarchyChanged);
    XISelectEvents(display.handle(), display.root().handle(), &eventMask, 1);

    m_display.registerHandler(GenericEvent, displayEventCallback, this);
}

XInputWatcher::~XInputWatcher()
{
    m_display.unregisterHandler(displayEventCallback);
}

void XInputWatcher::scan()
{
    int nInfo;
    std::unique_ptr<XIDeviceInfo[], void(*)(XIDeviceInfo*)> info(
        XIQueryDevice(m_display.handle(), XIAllDevices, &nInfo),
        XIFreeDeviceInfo
    );
    assert(info != nullptr);

    for (int i = 0; i < nInfo; ++i) {
        if (info[i].enabled) {
            onInputEnabled(info[i].deviceid, info[i].use);
        } else {
            onInputDisabled(info[i].deviceid, info[i].use);
        }
    }
}

void XInputWatcher::handleEvent(const XEvent & event)
{
    if (event.type != GenericEvent || event.xcookie.extension != m_XIopcode) { return; }

    switch (event.xcookie.evtype) {
    case XI_HierarchyChanged: {
        const auto & data = *reinterpret_cast<XIHierarchyEvent *>(event.xcookie.data);
        for (int i = 0; i < data.num_info; ++i) {
            if (data.info[i].flags & XIDeviceEnabled) {
                onInputEnabled(data.info[i].deviceid, data.info[i].use);
            }
            if (data.info[i].flags & XIDeviceDisabled) {
                onInputDisabled(data.info[i].deviceid, data.info[i].use);
            }
        }
        } break;
    case XI_RawKeyPress:
    case XI_RawKeyRelease: {
        const auto & data = *reinterpret_cast<XIRawEvent *>(event.xcookie.data);
        auto it = m_devices.find(data.deviceid);
        DEBUG("key ", data.detail - MIN_KEYCODE, " ",
              event.xcookie.evtype == XI_RawKeyPress ? "pressed" : "released",
              " on device ", data.deviceid);
        if (it != m_devices.end()) {
            emit keyEventReceived(it->second.devNode(), data.detail - MIN_KEYCODE,
                                  event.xcookie.evtype == XI_RawKeyPress);
        }
        } break;
    }
}

void XInputWatcher::onInputEnabled(xlib::Device::handle_type id, int type)
{
    if (type != XISlaveKeyboard) { return; }
    if (m_devices.find(id) != m_devices.end()) { return; }

    auto device = xlib::Device(m_display, id);
    if (device.devNode().empty()) { return; }

    xlib::ErrorCatcher errors;
    device.setEventMask({ XI_RawKeyPress, XI_RawKeyRelease });

    errors.synchronize(m_display);
    if (errors) {
        ERROR("failed to set events on device ", id, ": ", errors.errors().size(), " errors");
    } else {
        VERBOSE("xinput keyboard ", id, " enabled for device ", device.devNode());
        m_devices.emplace(id, std::move(device));
    }
}

void XInputWatcher::onInputDisabled(xlib::Device::handle_type id, int)
{
    auto it = m_devices.find(id);
    if (it == m_devices.end()) { return; }

    xlib::ErrorCatcher errors;
    m_devices.erase(it);
    VERBOSE("xinput keyboard ", id, " disabled");

    errors.synchronize(m_display);
    if (errors) {
        DEBUG("onInputDisabled, ignoring ", errors.errors().size(), " errors");
    }
}

void XInputWatcher::displayEventCallback(const XEvent & event, void * ptr)
{
    reinterpret_cast<XInputWatcher *>(ptr)->handleEvent(event);
}
