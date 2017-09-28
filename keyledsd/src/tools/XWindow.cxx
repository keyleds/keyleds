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
#include <algorithm>
#include <cassert>
#include <cstddef>
#include <limits>
#include <sstream>
#include <stdexcept>
#include <X11/Xatom.h>
#include <X11/extensions/XInput2.h>
#include "tools/XWindow.h"

using xlib::Device;
using xlib::ErrorCatcher;
using xlib::X11Display;

static constexpr char activeWindowAtom[] = "_NET_ACTIVE_WINDOW";
static constexpr char nameAtom[] = "_NET_WM_NAME";
static constexpr char deviceNodeAtom[] = "Device Node";
static constexpr char utf8Atom[] = "UTF8_STRING";

void std::default_delete<X11Display>::operator()(X11Display *ptr) const { XCloseDisplay(ptr); }

/****************************************************************************/

xlib::Display::Display(std::string name)
 : m_display(openDisplay(name)),
   m_name(DisplayString(m_display.get())),
   m_root(*this, DefaultRootWindow(m_display.get()))
{
}

xlib::Display::~Display()
{
    assert(m_handlers.empty());
}

int xlib::Display::connection() const
{
    return ConnectionNumber(m_display.get());
}

Atom xlib::Display::atom(const std::string & name) const
{
    const auto it = std::lower_bound(
        m_atomCache.cbegin(), m_atomCache.cend(), name,
        [](const auto & entry, const auto & name) { return entry.first < name; }
    );
    if (it != m_atomCache.cend() && it->first == name) {
        return it->second;
    }

    Atom atom = XInternAtom(m_display.get(), name.c_str(), True);
    m_atomCache.emplace(it, name, atom);
    return atom;
}

std::unique_ptr<xlib::Window> xlib::Display::getActiveWindow()
{
    std::string data = m_root.getProperty(atom(activeWindowAtom), XA_WINDOW);
    ::Window handle = *reinterpret_cast<const ::Window *>(data.data());
    if (handle == 0) { return nullptr; }
    return std::make_unique<Window>(*this, handle);
}

std::unique_ptr<X11Display> xlib::Display::openDisplay(const std::string & name)
{
    handle_type display = XOpenDisplay(name.empty() ? nullptr : name.c_str());
    if (display == nullptr) {
        throw Error(name.empty() ? "failed to open default display"
                                 : ("failed to open display " + name));
    }
    return std::unique_ptr<X11Display>(display);
}

void xlib::Display::processEvents()
{
    while (XPending(m_display.get())) {
        XEvent event;
        XNextEvent(m_display.get(), &event);
        XGetEventData(m_display.get(), &event.xcookie);
        for (const auto & item : m_handlers) {
            if (item.event == event.type || item.event == 0) {
                (*item.handler)(event, item.data);
            }
        }
        XFreeEventData(m_display.get(), &event.xcookie);
    }
}

void xlib::Display::registerHandler(event_type type, event_handler handler, void * data)
{
    m_handlers.push_back({type, handler, data});
}

void xlib::Display::unregisterHandler(event_handler handler)
{
    m_handlers.erase(std::remove_if(m_handlers.begin(), m_handlers.end(),
                                    [handler](const auto & item){ return item.handler == handler; }));
}

/****************************************************************************/

xlib::Window::Window(Display & display, handle_type window)
 : m_display(display), m_window(window), m_classLoaded(false)
{}

void xlib::Window::changeAttributes(unsigned long mask, const XSetWindowAttributes & attrs)
{
    XChangeWindowAttributes(m_display.handle(), m_window, mask,
                            const_cast<XSetWindowAttributes*>(&attrs));
}

std::string xlib::Window::name() const
{
    std::string name = getProperty(
        m_display.atom(nameAtom),
        m_display.atom(utf8Atom)
    );
    if (name.empty()) {
        name = getProperty(XA_WM_NAME, XA_STRING);
    }
    return name;
}

std::string xlib::Window::iconName() const
{
    return getProperty(XA_WM_ICON_NAME, XA_STRING);
}

std::string xlib::Window::getProperty(Atom atom, Atom type) const
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char * value;
    if (XGetWindowProperty(m_display.handle(), m_window, atom,
                           0, std::numeric_limits<long>::max() / 4,
                           False, type,
                           &actualType, &actualFormat,
                           &nItems, &bytesAfter, &value) != Success ||
        actualType != type) {
        return std::string();
    }

    // Wrap value in a unique_ptr in case something throws after that point
    auto valptr = std::unique_ptr<unsigned char, int(*)(void*)>(value, XFree);

    std::size_t itemBytes;
    switch (actualFormat) {
        case 8: itemBytes = sizeof(char); break;
        case 16: itemBytes = sizeof(short); break;
        case 32: itemBytes = sizeof(long); break;
        default: throw std::logic_error("Invalid actualFormat returned by XGetWindowProperty");
    }
    return std::string(reinterpret_cast<char *>(valptr.get()), nItems * itemBytes);
}

void xlib::Window::loadClass() const
{
    m_className = getProperty(XA_WM_CLASS, XA_STRING);
    auto sep = m_className.find('\0');
    if (sep != std::string::npos) {
        m_instanceName = m_className.substr(0, sep);
        m_className.erase(0, sep + 1);
        sep = m_className.find('\0');
        if (sep != std::string::npos) {
            m_className.erase(sep);
        }
    }
    m_classLoaded = true;
}

/****************************************************************************/

constexpr Device::handle_type Device::invalid_device;

Device::Device(Display & display, handle_type device)
 : m_display(display), m_device(device)
{
    m_devNode = getProperty(m_display.atom(deviceNodeAtom), XA_STRING);
}

Device::Device(Device && other) noexcept
 : m_display(other.m_display), m_device(invalid_device)
{
    std::swap(m_device, other.m_device);
    std::swap(m_devNode, other.m_devNode);
}

Device & Device::operator=(Device && other)
{
    assert(&m_display == &other.m_display);
    if (m_device != invalid_device) { setEventMask({}); m_device = invalid_device; }
    std::swap(m_device, other.m_device);
    m_devNode = std::move(other.m_devNode);
    return *this;
}

Device::~Device()
{
    if (m_device != invalid_device) { setEventMask({}); }
}

void Device::setEventMask(const std::vector<int> & events)
{
    std::vector<unsigned char> mask(XIMaskLen(XI_LASTEVENT), 0);
    XIEventMask eventMask = { m_device, (int)mask.size(), mask.data() };
    for (auto event : events) {
        XISetMask(mask.data(), event);
    }
    XISelectEvents(m_display.handle(), m_display.root().handle(), &eventMask, 1);
}

std::string Device::getProperty(Atom atom, Atom type) const
{
    Atom actualType;
    int actualFormat;
    unsigned long nItems, bytesAfter;
    unsigned char * value;
    if (XIGetProperty(m_display.handle(), m_device, atom,
                      0, std::numeric_limits<long>::max() / 4,
                      False, type,
                      &actualType, &actualFormat,
                      &nItems, &bytesAfter, &value) != Success ||
        actualType != type) {
        return std::string();
    }

    // Wrap value in a unique_ptr in case something throws after that point
    auto valptr = std::unique_ptr<unsigned char, int(*)(void*)>(value, XFree);

    std::size_t itemBytes;
    switch (actualFormat) {
        case 8: itemBytes = sizeof(char); break;
        case 16: itemBytes = sizeof(short); break;
        case 32: itemBytes = sizeof(long); break;
        default: throw Error("invalid actualFormat returned by XGetDeviceProperty");
    }
    return std::string(reinterpret_cast<char *>(valptr.get()), nItems * itemBytes);
}

/****************************************************************************/

std::string xlib::Error::makeMessage(X11Display *display, XErrorEvent *event)
{
    std::ostringstream msg;

    char buffer[256];
    if (XGetErrorText(display, event->error_code, buffer, sizeof(buffer)) == Success) {
        msg <<buffer;
    } else {
        msg <<"error code " <<event->error_code;
    }
    msg <<" on display " <<DisplayString(display)
        <<" while performing request " <<int(event->request_code) <<"." <<int(event->minor_code);

    return msg.str();
}

/****************************************************************************/

ErrorCatcher::ErrorCatcher()
{
    m_oldCatcher = s_current;
    s_current = this;
    m_oldHandler = XSetErrorHandler(errorHandler);
}

ErrorCatcher::~ErrorCatcher()
{
#ifdef NDEBUG
    XSetErrorHandler(m_oldHandler);
    s_current = m_oldCatcher;
#else
    auto prevHandler = XSetErrorHandler(m_oldHandler);
    auto prevCatcher = s_current;

    s_current = m_oldCatcher;

    assert(prevHandler == errorHandler);
    assert(prevCatcher == this);
#endif
}

void ErrorCatcher::synchronize(Display & display) const
{
    XSync(display.handle(), False);
}

int ErrorCatcher::errorHandler(X11Display * display, XErrorEvent * error)
{
    assert(s_current != nullptr);
    s_current->m_errors.emplace_back(display, error);
    return 0;
}

ErrorCatcher * ErrorCatcher::s_current = nullptr;
