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
#include <fcntl.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif
#include <locale.h>
#include <sys/types.h>
#include <sys/socket.h>
#include <unistd.h>
#include <QCoreApplication>
#include <QDBusConnection>
#include <QSocketNotifier>
#include <QTimer>
#include <csignal>
#include <exception>
#include <iostream>
#include "config.h"
#ifndef NO_DBUS
#include "keyledsd/dbus/ServiceAdaptor.h"
#endif
#include "keyledsd/effect/EffectManager.h"
#include "keyledsd/effect/StaticModuleRegistry.h"
#include "keyledsd/Configuration.h"
#include "keyledsd/Service.h"
#include "config.h"
#include "logging.h"

LOGGING("main");

using keyleds::Configuration;

/****************************************************************************/
// Command line parsing

#ifdef _GNU_SOURCE
static const struct option optionDescriptions[] = {
    {"config",      1, nullptr, 'c' },
    {"help",        0, nullptr, 'h' },
    {"module-path", 1, nullptr, 'm' },
    {"quiet",       0, nullptr, 'q' },
    {"single",      0, nullptr, 's' },
    {"verbose",     0, nullptr, 'v' },
    {"no-dbus",     0, nullptr, 'D' },
    {nullptr, 0, nullptr, 0}
};
#endif

class Options final
{
public:
    const char *                configPath;
    std::vector<std::string>    modulePaths;
    logging::level_t            logLevel;
    bool                        autoQuit;
    bool                        noDBus;

public:
    Options() : configPath(KEYLEDSD_CONFIG_FILE),
                logLevel(logging::warning::value),
                autoQuit(false),
                noDBus(false) {}

    static Options parse(int & argc, char * argv[])
    {
        Options options;
        int opt;
        std::ostringstream msgBuf;
        ::opterr = 0;
#ifdef _GNU_SOURCE
        while ((opt = ::getopt_long(argc, argv, ":c:hm:qsvD", optionDescriptions, nullptr)) >= 0) {
#else
        while ((opt = ::getopt(argc, argv, ":c:hm:qsvD")) >= 0) {
#endif
            switch(opt) {
            case 'c': options.configPath = optarg; break;
            case 'm': options.modulePaths.push_back(optarg); break;
            case 'q': options.logLevel = logging::critical::value; break;
            case 's': options.autoQuit = true; break;
            case 'v': options.logLevel += 1; break;
            case 'D': options.noDBus = true; break;
            case 'h':
                std::cout <<"Usage: " <<argv[0] <<" [-c path] [-h] [-q] [-s] [-v] [-D]" <<std::endl;
                ::exit(EXIT_SUCCESS);
            case ':':
                msgBuf <<argv[0] <<": option -- '" <<(char)::optopt <<"' requires an argument";
                throw std::runtime_error(msgBuf.str());
            default:
                msgBuf <<argv[0] <<": invalid option -- '" <<(char)::optopt <<"'";
                throw std::runtime_error(msgBuf.str());
            }
        }
        return options;
    }
};


/****************************************************************************/
// POSIX signal management

// This defines a signal handler that simply write signal number to signalSendFd
static int signalSendFd;
static void sigHandler(int sig)
{
    if (write(signalSendFd, &sig, sizeof(sig)) < 0) {
        CRITICAL("ignoring signal ", sig, " due to failed write");
    }
}

/// Returns a socket that get an int everytime sigHandler catches a signal
static int openSignalSocket(std::vector<int> sigs)
{
    int signalSockets[2];
    if (socketpair(AF_UNIX, SOCK_STREAM, 0, signalSockets) < 0) { return -1; }
    if (fcntl(signalSockets[1], F_SETFL, O_NONBLOCK) < 0) {
        close(signalSockets[0]);
        close(signalSockets[1]);
        return -1;
    }
    signalSendFd = signalSockets[0];
    for (int sigNum : sigs) { std::signal(sigNum, sigHandler); }
    return signalSockets[1];
}

/// Invoked asynchronously to convert POSIX signal into intent
static void handleSignalEvent(const Options & options, keyleds::Service * service, int fd)
{
    int sig;
    while (read(fd, &sig, sizeof(sig)) == sizeof(sig)) {
        switch(sig) {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                QCoreApplication::quit();
                break;
            case SIGHUP:
                INFO("reloading ", options.configPath);
                try {
                    auto conf = Configuration::loadFile(options.configPath);
                    service->setConfiguration(std::move(conf));
                } catch (std::exception & error) {
                    CRITICAL("reloading failed: ", error.what());
                }
                break;
        }
    }
}

/****************************************************************************/

int main(int argc, char * argv[])
{
    // Must be before app, so its destructor runs after, since Service holds a ref
    keyleds::effect::EffectManager effectManager;

    // Create event loop
    QCoreApplication app(argc, argv);
    app.setOrganizationDomain("etherdream.org");
    app.setApplicationName("keyledsd");
    app.setApplicationVersion(KEYLEDSD_VERSION_STR);
    ::setlocale(LC_NUMERIC, "C"); // we deal with system stuff and config files

    // Parse command line
    auto options = Options::parse(argc, argv);
    logging::Configuration::instance().setPolicy(
        new logging::FilePolicy(STDERR_FILENO, options.logLevel)
    );

    INFO("keyledsd v" KEYLEDSD_VERSION_STR " starting up");

    // Load configuration
    std::unique_ptr<Configuration> configuration;
    try {
        configuration = Configuration::loadFile(options.configPath);
        VERBOSE("using ", configuration->path());
    } catch (std::exception & error) {
        CRITICAL("Could not load configuration: ", error.what());
        return 1;
    }

    // Register modules
    std::copy(options.modulePaths.cbegin(), options.modulePaths.cend(),
              std::back_inserter(effectManager.searchPaths()));
    std::copy(configuration->pluginPaths().begin(), configuration->pluginPaths().end(),
              std::back_inserter(effectManager.searchPaths()));
    effectManager.searchPaths().push_back(SYS_CONFIG_LIBDIR "/" KEYLEDSD_MODULE_PREFIX);

    {
        std::string error;
        for (const auto & module : keyleds::effect::StaticModuleRegistry::instance().modules()) {
            if (!effectManager.add(module.first, module.second, &error)) {
                ERROR("static module <", module.first, ">: ", error);
            }
        }
        for (const auto & name : configuration->plugins()) {
            if (!effectManager.load(name, &error)) {
                WARNING("loading module <", name, ">: ", error);
            }
        }
    }

    // Setup application components
    auto service = new keyleds::Service(effectManager, std::move(configuration), &app);
    service->setAutoQuit(options.autoQuit);
    QTimer::singleShot(0, service, &keyleds::Service::init);

#ifndef NO_DBUS
    if (!options.noDBus) {
        auto connection = QDBusConnection::sessionBus();
        new keyleds::dbus::ServiceAdaptor(service);   // service takes ownership
        if (!connection.registerObject("/Service", service) ||
            !connection.registerService("org.etherdream.KeyledsService")) {
            CRITICAL("DBus registration failed");
            return 2;
        }
    }
#endif

    // Register signals and go
    int sigFd = openSignalSocket({SIGINT, SIGTERM, SIGQUIT, SIGHUP});
    if (sigFd >= 0) {
        auto * notifier = new QSocketNotifier(sigFd, QSocketNotifier::Read, service);
        QObject::connect(notifier, &QSocketNotifier::activated,
                         [options, service](int fd){ handleSignalEvent(options, service, fd); });
    }

    return app.exec();
}
