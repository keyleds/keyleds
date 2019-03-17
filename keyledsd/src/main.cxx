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
#include "config.h"
#include "keyledsd/logging.h"
#ifndef NO_DBUS
#include "keyledsd/service/dbus/Service.h"
#endif
#include "keyledsd/service/Configuration.h"
#include "keyledsd/service/EffectManager.h"
#include "keyledsd/service/Service.h"
#include "keyledsd/service/StaticModuleRegistry.h"
#include "keyledsd/tools/Event.h"
#include "keyledsd/tools/FileWatcher.h"
#include "keyledsd/tools/XWindow.h"
#include <clocale>
#include <csignal>
#include <cstring>
#include <exception>
#include <fcntl.h>
#ifdef _GNU_SOURCE
#include <getopt.h>
#endif
#include <iostream>
#include <optional>
#include <sys/socket.h>
#include <sys/types.h>
#ifndef NO_DBUS
#include <systemd/sd-bus.h>
#endif
#include <unistd.h>
#include <uv.h>

LOGGING("main");

using keyleds::service::Configuration;

static uv_loop_t main_loop;

/****************************************************************************/
// Command line parsing

class Options final
{
public:
    const char *                configPath = KEYLEDSD_CONFIG_FILE;
    std::vector<std::string>    modulePaths;
    keyleds::logging::level_t   logLevel = keyleds::logging::warning::value;
    bool                        autoQuit = false;
    bool                        noDBus = false;

public:
    static std::optional<Options> parse(int & argc, char * argv[])
    {
        Options options;
        int opt;
        ::opterr = 0;
#ifdef _GNU_SOURCE
        static constexpr struct option optionDescriptions[] = {
            {"config",      1, nullptr, 'c' },
            {"help",        0, nullptr, 'h' },
            {"module-path", 1, nullptr, 'm' },
            {"quiet",       0, nullptr, 'q' },
            {"single",      0, nullptr, 's' },
            {"verbose",     0, nullptr, 'v' },
            {"no-dbus",     0, nullptr, 'D' },
            {nullptr, 0, nullptr, 0}
        };
        while ((opt = ::getopt_long(argc, argv, ":c:hm:qsvD", optionDescriptions, nullptr)) >= 0) {
#else
        while ((opt = ::getopt(argc, argv, ":c:hm:qsvD")) >= 0) {
#endif
            switch(opt) {
            case 'c': options.configPath = optarg; break;
            case 'm': options.modulePaths.emplace_back(optarg); break;
            case 'q': options.logLevel = keyleds::logging::critical::value; break;
            case 's': options.autoQuit = true; break;
            case 'v': options.logLevel += 1; break;
            case 'D': options.noDBus = true; break;
            case 'h':
                std::cout <<"Usage: " <<argv[0] <<" [-c path] [-h] [-m path] [-q] [-s] [-v] [-D]\n";
                return std::nullopt;
            case ':':
                std::cerr <<argv[0] <<": option -- '" <<char(::optopt) <<"' requires an argument\n";
                return std::nullopt;
            default:
                std::cerr <<argv[0] <<": invalid option -- '" <<char(::optopt) <<"'\n";
                return std::nullopt;
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
static int openSignalSocket(const std::vector<int> & sigs)
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
static void handleSignalEvent(const Options & options, keyleds::service::Service & service, int fd)
{
    int sig;
    while (read(fd, &sig, sizeof(sig)) == static_cast<ssize_t>(sizeof(sig))) {
        switch(sig) {
            case SIGINT:
            case SIGQUIT:
            case SIGTERM:
                uv_stop(&main_loop);
                break;
            case SIGHUP:
                NOTICE("reloading ", options.configPath);
                try {
                    service.setConfiguration(Configuration::loadFile(options.configPath));
                } catch (std::exception & error) {
                    CRITICAL("reloading failed: ", error.what());
                    break;
                }
                break;
            case SIGUSR1:
                NOTICE("forcing device refresh");
                service.forceRefreshDevices();
                break;
        }
    }
}

/****************************************************************************/

int main(int argc, char * argv[])
{
    using namespace keyleds;

    ::setlocale(LC_NUMERIC, "C"); // we deal with system stuff and config files

    // Parse command line
    const auto options = Options::parse(argc, argv);
    if (!options) { return 1; }

    // Configure logging - intentionally leak policy in case some destructor logs stuff
    auto logPolicy = new logging::FilePolicy(STDERR_FILENO, options->logLevel);
    logging::Configuration::instance().setPolicy(logPolicy);

    INFO("keyledsd v" KEYLEDSD_VERSION_STR " starting up");

    // Load configuration
    auto configuration = Configuration();
    try {
        configuration = Configuration::loadFile(options->configPath);
        INFO("using ", configuration.path);
    } catch (std::exception & error) {
        CRITICAL("Could not load configuration: ", error.what());
        return 1;
    }

    // Register modules
    auto effectManager = service::EffectManager();
    {
        std::copy(options->modulePaths.cbegin(), options->modulePaths.cend(),
                std::back_inserter(effectManager.searchPaths()));
        std::copy(configuration.pluginPaths.begin(), configuration.pluginPaths.end(),
                std::back_inserter(effectManager.searchPaths()));
        effectManager.searchPaths().push_back(SYS_CONFIG_LIBDIR "/" KEYLEDSD_MODULE_PREFIX);

        std::string error;
        for (const auto & module : service::StaticModuleRegistry::instance().modules()) {
            if (!effectManager.add(module.first, module.second, &error)) {
                ERROR("static module <", module.first, ">: ", error);
            }
        }
        for (const auto & name : configuration.plugins) {
            if (!effectManager.load(name, &error)) {
                WARNING("loading module <", name, ">: ", error);
            }
        }
    }

#ifndef NO_DBUS
    sd_bus * bus = nullptr;
    if (int err = sd_bus_open_user(&bus); err < 0) {
        CRITICAL("Could not connect to session bus: ", strerror(-err));
        return 2;
    }
    if (int err = sd_bus_request_name(bus, "org.etherdream.KeyledsService", 0); err < 0) {
        CRITICAL("Could not reserve name on session bus: ", strerror(-err));
        return 2;
    }
#endif

    // Create event loop
    uv_loop_init(&main_loop);
    {
        // Setup application components
        auto watcher = tools::FileWatcher(main_loop);
        auto service = service::Service(
            effectManager, watcher, std::move(configuration), main_loop
        );
        service.setAutoQuit(options->autoQuit);

        try {
            auto display = std::make_unique<tools::xlib::Display>();
            NOTICE("connected to display ", display->name());
            service.addDisplay(std::move(display));
        } catch (tools::xlib::Error & err) {
            CRITICAL("X display initialization failed: ", err.what());
            return 2;
        }

#ifndef NO_DBUS
        auto serviceAdapter = service::dbus::ServiceAdapter(bus, service);
        auto dbusFdWatcher = tools::FDWatcher(
            sd_bus_get_fd(bus), tools::FDWatcher::Read,
            [&](auto){ while (sd_bus_process(bus, nullptr) > 0) { /* empty */ } },
            main_loop
        );
#endif

        // Register signals and go
        auto sigFdWatcher = std::unique_ptr<tools::FDWatcher>();
        int sigFd = openSignalSocket({SIGINT, SIGTERM, SIGQUIT, SIGHUP, SIGUSR1});
        if (sigFd >= 0) {
            sigFdWatcher = std::make_unique<tools::FDWatcher>(
                sigFd, tools::FDWatcher::Read,
                [&](auto){ handleSignalEvent(*options, service, sigFd); },
                main_loop
            );
        }

        uv_run(&main_loop, UV_RUN_DEFAULT);
    }
    uv_run(&main_loop, UV_RUN_NOWAIT);  // let closed handles cleanup
    uv_loop_close(&main_loop);

#ifndef NO_DBUS
    sd_bus_flush(bus);
    sd_bus_close(bus);
    sd_bus_unref(bus);
#endif

    return 0;
}
