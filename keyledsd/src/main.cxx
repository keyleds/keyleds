#include <signal.h>
#include <QCoreApplication>
#include <QTimer>
#include <iostream>
#include "config.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"

static void quit_handler(int) { QCoreApplication::quit(); }

int main(int argc, char * argv[])
{
    // Load configuration
    keyleds::Configuration configuration;
    try {
        configuration = keyleds::Configuration::loadFile("keyledsd.conf");
    } catch (std::exception & error) {
        std::cerr <<"Configuration error: " <<error.what() <<std::endl;
        return 1;
    }

    // Create event loop
    QCoreApplication app(argc, argv);
    app.setOrganizationDomain("etherdream.org");
    app.setApplicationName("keyledsd");
    app.setApplicationVersion(KEYLEDSD_VERSION_STR);

    // Setup application components
    auto service = new keyleds::Service(configuration, &app);

    // Register signals and go
    signal(SIGINT, quit_handler);
    signal(SIGQUIT, quit_handler);
    signal(SIGTERM, quit_handler);
    signal(SIGHUP, quit_handler);
    QTimer::singleShot(0, service, SLOT(init()));

    return app.exec();
}
