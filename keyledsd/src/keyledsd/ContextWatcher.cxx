#include <QtCore>
#include "keyledsd/Context.h"
#include "keyledsd/ContextWatcher.h"

using keyleds::XContextWatcher;
const std::string activeWindowAtom = "_NET_ACTIVE_WINDOW";

/****************************************************************************/

XContextWatcher::XContextWatcher(QObject *parent)
 : XContextWatcher(std::string(), parent) {}

XContextWatcher::XContextWatcher(std::string display, QObject *parent)
 : QObject(parent),
   m_display(display)
{
    setupDisplay(m_display);
    // this QObject takes automatic ownership of QSocketNotifier
    auto notifier = new QSocketNotifier(m_display.connection(), QSocketNotifier::Read, this);
    QObject::connect(notifier, SIGNAL(activated(int)),
                     this, SLOT(onDisplayEvent(int)));
}

XContextWatcher::XContextWatcher(xlib::Display && display, QObject *parent)
 : QObject(parent),
   m_display(std::move(display))
{
    setupDisplay(m_display);
    // this QObject takes automatic ownership of QSocketNotifier
    auto notifier = new QSocketNotifier(m_display.connection(), QSocketNotifier::Read, this);
    QObject::connect(notifier, SIGNAL(activated(int)),
                     this, SLOT(onDisplayEvent(int)));
}

void XContextWatcher::setupDisplay(xlib::Display & display)
{
    XSetWindowAttributes attributes;
    attributes.event_mask = PropertyChangeMask;
    display.root().changeAttributes(CWEventMask, attributes);

    m_activeWindow = display.getActiveWindow();
    onActiveWindowChanged(m_activeWindow.get());
}

void XContextWatcher::onDisplayEvent(int)
{
    while (XPending(m_display.handle())) {
        XEvent event;
        XNextEvent(m_display.handle(), &event);
        handleEvent(event);
    }
}

void XContextWatcher::handleEvent(XEvent & event)
{
    switch (event.type) {
    case PropertyNotify:
        if (event.xproperty.atom == m_display.atom(activeWindowAtom)) {
            auto active = m_display.getActiveWindow();
            if ((active == nullptr) != (m_activeWindow == nullptr) ||
                (active != nullptr && active->handle() != m_activeWindow->handle())) {
                // Event must fire before the property is updated
                onActiveWindowChanged(active.get());
                m_activeWindow = std::move(active);
            }
        }
        if (m_activeWindow != nullptr && (
                event.xproperty.atom == m_display.atom("_NET_WM_NAME") ||
                event.xproperty.atom == m_display.atom("WM_NAME"))) {
            setContext(m_activeWindow.get());
        }
        break;
    }
}

void XContextWatcher::onActiveWindowChanged(xlib::Window * window)
{
    if (m_activeWindow != nullptr) {
        XSetWindowAttributes attributes;
        attributes.event_mask = NoEventMask;
        m_activeWindow->changeAttributes(CWEventMask, attributes);
    }
    if (window != nullptr) {
        XSetWindowAttributes attributes;
        attributes.event_mask = PropertyChangeMask;
        window->changeAttributes(CWEventMask, attributes);
    }

    setContext(window);
}

void XContextWatcher::setContext(xlib::Window * window)
{
    Context context;
    if (window == nullptr) {
        context = {};
    } else {
        context = {
            { "id", std::to_string(window->handle()) },
            { "title", window->name() },
            { "class", window->className() },
            { "instance", window->instanceName() }
        };
    }

    if (context != m_context) {
        m_context = std::move(context);
        emit contextChanged(m_context);
    }
}
