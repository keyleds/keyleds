#include <signal.h>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include <locale.h>
#include "config.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"
#include "keyledsd/ServiceAdaptor.h"

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
        configuration = keyleds::Configuration::loadArguments(argc, argv);
    } catch (std::exception & error) {
        std::cerr <<error.what() <<std::endl;
        return 1;
    }

    // Setup application components
    auto connection = QDBusConnection::sessionBus();
    auto service = new keyleds::Service(configuration, &app);

    new keyleds::ServiceAdaptor(service);
    if (!connection.registerObject("/Service", service) ||
        !connection.registerService("org.etherdream.KeyledsService")) {
        std::cerr <<"DBus registration failed" <<std::endl;
        return 2;
    }

    // Register signals and go
    signal(SIGINT, quit_handler);
    signal(SIGQUIT, quit_handler);
    signal(SIGTERM, quit_handler);
    signal(SIGHUP, quit_handler);
    QTimer::singleShot(0, service, SLOT(init()));

    return app.exec();
}
