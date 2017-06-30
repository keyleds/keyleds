#ifndef KEYLEDSD_CONTEXTWATCHER_H
#define KEYLEDSD_CONTEXTWATCHER_H

#include <QObject>
#include <memory>
#include <string>
#include "keyledsd/Context.h"
#include "tools/XWindow.h"

namespace keyleds {

class XContextWatcher : public QObject
{
    Q_OBJECT
public:
                    XContextWatcher(QObject *parent = nullptr);
                    XContextWatcher(std::string display, QObject *parent = nullptr);
                    XContextWatcher(xlib::Display && display, QObject *parent = nullptr);

    const Context & current() const noexcept { return m_context; }

    const xlib::Display & display() const { return m_display; }

signals:
    void            contextChanged(const keyleds::Context &);

protected:
    virtual void    setupDisplay(xlib::Display &);
    virtual void    handleEvent(XEvent &);
    virtual void    onActiveWindowChanged(xlib::Window *);
    void            setContext(xlib::Window *);

private slots:
    void            onDisplayEvent(int);

protected:
    xlib::Display                   m_display;
    std::unique_ptr<xlib::Window>   m_activeWindow;
    Context                         m_context;
};


};

#endif
