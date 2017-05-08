#include <signal.h>
#include <QCoreApplication>
#include <QTimer>
#include <cstdlib>
#include <iostream>
#include <locale.h>
#include "config.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"

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
    const char * confFilePath = ::getenv("CONFIG");
    if (confFilePath == NULL) { confFilePath = KEYLEDSD_CONFIG_PATH; }
    try {
        configuration = keyleds::Configuration::loadFile(confFilePath);
    } catch (std::exception & error) {
        std::cerr <<"Error reading " <<confFilePath <<": " <<error.what() <<std::endl;
        return 1;
    }

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
