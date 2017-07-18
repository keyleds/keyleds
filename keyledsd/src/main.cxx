#include <QCoreApplication>
#include <QTimer>
#include <csignal>
#include <iostream>
#include <locale.h>
#include <keyleds.h>
#include "config.h"
#ifndef NO_DBUS
#include "dbus/ServiceAdaptor.h"
#endif
#include "keyledsd/Configuration.h"
#include "keyledsd/ContextWatcher.h"
#include "keyledsd/Service.h"
#include "logging.h"

LOGGING("main");

static void quit_handler(int) { QCoreApplication::quit(); }

int main(int argc, char * argv[])
{
    keyleds::Configuration configuration;

    // Create event loop
    QCoreApplication app(argc, argv);
    app.setOrganizationDomain("etherdream.org");
    app.setApplicationName("keyledsd");
    app.setApplicationVersion(KEYLEDSD_VERSION_STR);

    ::setlocale(LC_NUMERIC, "C"); // we deal with system stuff and config files

    // Load configuration
    try {
        configuration = std::move(keyleds::Configuration::loadArguments(argc, argv));
        g_keyleds_debug_level = configuration.logLevel();
    } catch (std::exception & error) {
        std::cerr <<"Could not load configuration: " <<error.what() <<std::endl;
        return 1;
    }

    // Setup application components
    auto service = new keyleds::Service(configuration, &app);

    try {
        auto contextWatcher = new keyleds::XContextWatcher(service);
        service->setContext(contextWatcher->current());
        QObject::connect(contextWatcher, SIGNAL(contextChanged(const keyleds::Context &)),
                         service, SLOT(setContext(const keyleds::Context &)));
        INFO("using display ", contextWatcher->display().name(), " for events");
    } catch (xlib::Error & error) {
        ERROR("skipping display events: ", error.what());
    }

#ifndef NO_DBUS
    if (!configuration.noDBus()) {
        auto connection = QDBusConnection::sessionBus();
        new dbus::ServiceAdaptor(service);   // service takes ownership
        if (!connection.registerObject("/Service", service) ||
            !connection.registerService("org.etherdream.KeyledsService")) {
            CRITICAL("DBus registration failed");
            return 2;
        }
    }
#endif

    // Register signals and go
    std::signal(SIGINT, quit_handler);
    std::signal(SIGQUIT, quit_handler);
    std::signal(SIGTERM, quit_handler);
    std::signal(SIGHUP, SIG_IGN);
    QTimer::singleShot(0, service, SLOT(init()));

    return app.exec();
}
