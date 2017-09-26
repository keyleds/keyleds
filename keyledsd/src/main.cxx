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
#include <locale.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QTimer>
#include <csignal>
#include <exception>
#include <iostream>
#include "config.h"
#ifndef NO_DBUS
#include "dbus/ServiceAdaptor.h"
#endif
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"
#include "config.h"
#include "logging.h"

LOGGING("main");

static void quit_handler(int) { QCoreApplication::quit(); }

int main(int argc, char * argv[])
{
    keyleds::Configuration configuration;
    INFO("keyledsd v" KEYLEDSD_VERSION_STR " starting up");

    // Create event loop
    QCoreApplication app(argc, argv);
    app.setOrganizationDomain("etherdream.org");
    app.setApplicationName("keyledsd");
    app.setApplicationVersion(KEYLEDSD_VERSION_STR);

    ::setlocale(LC_NUMERIC, "C"); // we deal with system stuff and config files

    // Load configuration
    try {
        configuration = keyleds::Configuration::loadArguments(argc, argv);
        logging::Configuration::instance().setPolicy(
            new logging::FilePolicy(STDERR_FILENO, configuration.logLevel())
        );
    } catch (std::exception & error) {
        std::cerr <<"Could not load configuration: " <<error.what() <<std::endl;
        return 1;
    }

    // Setup application components
    auto service = new keyleds::Service(configuration, &app);
    QTimer::singleShot(0, service, &keyleds::Service::init);

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

    return app.exec();
}
