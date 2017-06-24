#ifndef KEYLEDSD_CONTEXTWATCHER_H
#define KEYLEDSD_CONTEXTWATCHER_H

#include <QObject>
#include <memory>
#include <string>
#include "tools/XWindow.h"

namespace keyleds {

class Context;

class XContextWatcher : public QObject
{
    Q_OBJECT
public:
                    XContextWatcher(QObject *parent = nullptr);
                    XContextWatcher(std::string display, QObject *parent = nullptr);
                    XContextWatcher(xlib::Display && display, QObject *parent = nullptr);

    const xlib::Display & display() const { return m_display; }

signals:
    void            contextChanged(const keyleds::Context &);

protected:
    virtual void    setupDisplay(xlib::Display &);
    virtual void    handleEvent(XEvent &);
    virtual void    onActiveWindowChanged(xlib::Window *);

private slots:
    void            onDisplayEvent(int);

protected:
    xlib::Display                   m_display;
    std::unique_ptr<xlib::Window>   m_activeWindow;
};


};

#endif
