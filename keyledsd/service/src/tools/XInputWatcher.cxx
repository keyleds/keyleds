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
#include "tools/XInputWatcher.h"

#include "logging.h"
#define Bool int
#include <X11/extensions/XInput2.h>
#undef Bool
#include <algorithm>
#include <cassert>

#define MIN_KEYCODE 8

LOGGING("xinput-watcher");

using xlib::XInputWatcher;

/****************************************************************************/

static constexpr char XInputExtensionName[] = "XInputExtension";

static int queryOpcode(xlib::Display & display, const char * name)
{
    int event, error, opcode;
    if (!XQueryExtension(display.handle(), name, &opcode, &event, &error)) {
        throw std::runtime_error("XInput extension not available");
    }
    return opcode;
}

/****************************************************************************/

XInputWatcher::XInputWatcher(Display & display)
 : m_display(display),
   m_displayReg(m_display.registerHandler(GenericEvent, std::bind(
                &XInputWatcher::handleEvent, this, std::placeholders::_1))),
   m_XIopcode(queryOpcode(display, XInputExtensionName))
{
    std::vector<unsigned char> mask(XIMaskLen(XI_LASTEVENT), 0);
    XIEventMask eventMask = { XIAllDevices, int(mask.size()), mask.data() };
    XISetMask(mask.data(), XI_HierarchyChanged);
    XISelectEvents(display.handle(), display.root().handle(), &eventMask, 1);
}

XInputWatcher::~XInputWatcher() = default;

void XInputWatcher::scan()
{
    int nInfo;
    std::unique_ptr<XIDeviceInfo[], void(*)(XIDeviceInfo*)> info(
        XIQueryDevice(m_display.handle(), XIAllDevices, &nInfo),
        XIFreeDeviceInfo
    );
    assert(info != nullptr);
    assert(nInfo >= 0);

    for (std::size_t i = 0; i < static_cast<unsigned>(nInfo); ++i) {
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
        const auto * data = static_cast<XIHierarchyEvent *>(event.xcookie.data);
        for (int i = 0; i < data->num_info; ++i) {
            if (data->info[i].flags & XIDeviceEnabled) {
                onInputEnabled(data->info[i].deviceid, data->info[i].use);
            }
            if (data->info[i].flags & XIDeviceDisabled) {
                onInputDisabled(data->info[i].deviceid, data->info[i].use);
            }
        }
        } break;
    case XI_RawKeyPress:
    case XI_RawKeyRelease: {
        const auto * data = static_cast<XIRawEvent *>(event.xcookie.data);
        DEBUG("key ", data->detail - MIN_KEYCODE, " ",
              event.xcookie.evtype == XI_RawKeyPress ? "pressed" : "released",
              " on device ", data->deviceid);
        auto it = std::find_if(
            m_devices.begin(), m_devices.end(),
            [&](const auto & device) { return device.handle() == data->deviceid; }
        );
        if (it != m_devices.end()) {
            keyEventReceived.emit(it->devNode(), data->detail - MIN_KEYCODE,
                                  event.xcookie.evtype == XI_RawKeyPress);
        }
        } break;
    }
}

void XInputWatcher::onInputEnabled(int deviceId, int use)
{
    if (use != XISlaveKeyboard) { return; }
    auto it = std::find_if(
        m_devices.begin(), m_devices.end(),
        [&](const auto & device) { return device.handle() == deviceId; }
    );
    if (it != m_devices.end()) { return; }

    auto device = Device(m_display, deviceId);
    if (device.devNode().empty()) { return; }

    ErrorCatcher errors;
    device.setEventMask({ XI_RawKeyPress, XI_RawKeyRelease });

    errors.synchronize(m_display);
    if (errors) {
        ERROR("failed to set events on device ", deviceId, ": ",
              errors.errors().size(), " errors");
    } else {
        VERBOSE("xinput keyboard ", deviceId, " enabled for device ", device.devNode());
        m_devices.emplace_back(std::move(device));
    }
}

void XInputWatcher::onInputDisabled(int deviceId, int use)
{
    if (use != XISlaveKeyboard) { return; }
    auto it = std::find_if(
        m_devices.begin(), m_devices.end(),
        [&](const auto & device) { return device.handle() == deviceId; }
    );
    if (it == m_devices.end()) { return; }

    ErrorCatcher errors;
    if (it != m_devices.end() - 1) { *it = std::move(m_devices.back()); }
    m_devices.pop_back();
    VERBOSE("xinput keyboard ", deviceId, " disabled");

    errors.synchronize(m_display);
    if (errors) {
        DEBUG("onInputDisabled, ignoring ", errors.errors().size(), " errors");
    }
}
