#include <QCoreApplication>
#include <iostream>
#include "keyledsd/Configuration.h"
#include "keyledsd/ContextWatcher.h"
#include "keyledsd/Device.h"
#include "keyledsd/Service.h"
#include "config.h"
#include "logging.h"

LOGGING("service");

using keyleds::Context;
using keyleds::Service;

/****************************************************************************/

Service::Service(Configuration & configuration, QObject * parent)
    : QObject(parent),
      m_configuration(configuration),
      m_deviceWatcher(nullptr, this)
{
    m_active = false;
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceAdded(const device::Description &)),
                     this, SLOT(onDeviceAdded(const device::Description &)));
    QObject::connect(&m_deviceWatcher, SIGNAL(deviceRemoved(const device::Description &)),
                     this, SLOT(onDeviceRemoved(const device::Description &)));
    DEBUG("created");
}

Service::~Service()
{
    setActive(false);
    m_devices.clear();
    DEBUG("destroyed");
}

void Service::init()
{
    setActive(true);
}

void Service::setActive(bool active)
{
    DEBUG("switching to ", active ? "active" : "inactive", " mode");
    m_deviceWatcher.setActive(active);
    m_active = active;
}

void Service::setContext(const Context & context)
{
    DEBUG("setContext ", context);
    m_context = context;
    for (auto & deviceEntry : m_devices) { deviceEntry.second->setContext(m_context); }
}

void Service::onDeviceAdded(const device::Description & description)
{
    DEBUG("device added: ", description.devNode());
    try {
        auto device = Device(description.devNode());
        auto manager = std::make_unique<DeviceManager>(
            device::Description(description), std::move(device), m_configuration, m_context
        );
        emit deviceManagerAdded(*manager);

        auto dit = m_configuration.devices().find(manager->serial());
        INFO("opened device ", description.devNode(),
             ": serial ", manager->serial(),
             " [", dit != m_configuration.devices().end() ? dit->second : std::string(), ']',
             ", model ", manager->device().model(),
             " firmware ", manager->device().firmware(),
             ", <", manager->device().name(), ">");

        m_devices[description.devPath()] = std::move(manager);

    } catch (Device::error & error) {
        // Suppress hid version error, it just means it's not the kind of device we want
        if (error.code() != KEYLEDS_ERROR_HIDVERSION) {
            ERROR("not opening device ", description.devNode(), ": ", error.what());
        }
    }
}

void Service::onDeviceRemoved(const device::Description & description)
{
    auto it = m_devices.find(description.devPath());
    if (it != m_devices.end()) {
        auto manager = std::move(it->second);
        m_devices.erase(it);

        INFO("removing device ", manager->serial());
        emit deviceManagerRemoved(*manager);

        if (m_devices.empty() && m_configuration.autoQuit()) {
            QCoreApplication::quit();
        }
    }
}
