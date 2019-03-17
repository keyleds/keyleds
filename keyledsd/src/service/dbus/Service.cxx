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
#include "keyledsd/service/dbus/Service.h"

#include "keyledsd/service/DeviceManager.h"
#include "keyledsd/service/Service.h"
#include "keyledsd/service/dbus/DeviceManager.h"
#include <algorithm>
#include <systemd/sd-bus.h>

using keyleds::service::dbus::DeviceManagerAdapter;
using keyleds::service::dbus::ServiceAdapter;

/****************************************************************************/

static constexpr char objectPath[] = "/Service";
static constexpr char interfaceName[] = "org.etherdream.keyleds.Service";

/****************************************************************************/

static int getConfigurationPath(sd_bus *, const char *, const char *, const char *,
                                sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = static_cast<ServiceAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's',
        adapter->service().configuration().path.c_str()
    );
}

static int getContext(sd_bus *, const char *, const char *, const char *,
                      sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    ret = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "{ss}");
    if (ret < 0) { return ret; }
    for (const auto & entry : adapter->service().context()) {
        ret = sd_bus_message_append(reply, "{ss}", entry.first.c_str(), entry.second.c_str());
        if (ret < 0) { return ret; }
    }
    return sd_bus_message_close_container(reply);
}

static int getAutoQuit(sd_bus *, const char *, const char *, const char *,
                       sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = static_cast<ServiceAdapter *>(userdata);
    return sd_bus_message_append(reply, "b", adapter->service().autoQuit());
}

static int setAutoQuit(sd_bus *, const char *, const char *, const char *,
                       sd_bus_message * value, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    int autoQuit;
    ret = sd_bus_message_read_basic(value, 'b', &autoQuit);
    if (ret < 0) { return ret; }
    adapter->service().setAutoQuit(bool(autoQuit));
    return 0;}

static int getDevices(sd_bus *, const char *, const char *, const char *,
                      sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    ret = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "o");
    if (ret < 0) { return ret; }
    for (const auto & device : adapter->service().devices()) {
        ret = sd_bus_message_append_basic(reply, 'o', DeviceManagerAdapter::pathFor(*device).c_str());
        if (ret < 0) { return ret; }
    }
    return sd_bus_message_close_container(reply);
}

static int getPlugins(sd_bus *, const char *, const char *, const char *,
                      sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    ret = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "s");
    if (ret < 0) { return ret; }
    for (const auto & name : adapter->service().effectManager().pluginNames()) {
        ret = sd_bus_message_append_basic(reply, 's', name.c_str());
        if (ret < 0) { return ret; }
    }
    return sd_bus_message_close_container(reply);
}

static int setContextValues(sd_bus_message * message, void *userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);
    auto context = std::vector<std::pair<std::string, std::string>>();

    ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, "{ss}");
    if (ret < 0) { return ret; }
    while ((ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_DICT_ENTRY, "ss")) > 0) {
        const char * key, * value;
        ret = sd_bus_message_read(message, "ss", &key, &value);
        if (ret < 0) { return ret; }
        context.emplace_back(key, value);
    }
    ret = sd_bus_message_close_container(message);
    if (ret < 0) { return ret; }

    adapter->service().setContext(context);
    return 0;
}

static int setContextValue(sd_bus_message * message, void *userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    const char * key, * value;
    ret = sd_bus_message_read(message, "ss", &key, &value);
    if (ret < 0) { return ret; }

    adapter->service().setContext({{ key, value }});
    return 0;
}

static int sendGenericEvent(sd_bus_message * message, void *userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);
    auto data = std::vector<std::pair<std::string, std::string>>();

    ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_ARRAY, "{ss}");
    if (ret < 0) { return ret; }
    while ((ret = sd_bus_message_enter_container(message, SD_BUS_TYPE_DICT_ENTRY, "ss")) > 0) {
        const char * key, * value;
        ret = sd_bus_message_read(message, "ss", &key, &value);
        if (ret < 0) { return ret; }
        data.emplace_back(key, value);
    }
    ret = sd_bus_message_close_container(message);
    if (ret < 0) { return ret; }

    adapter->service().handleGenericEvent(data);
    return 0;
}

static int sendKeyEvents(sd_bus_message * message, void *userdata, sd_bus_error *)
{
    int ret;
    auto adapter = static_cast<ServiceAdapter *>(userdata);

    const char * serial;
    uint16_t     key;
    ret = sd_bus_message_read(message, "sq", &serial, &key);
    if (ret < 0) { return ret; }

    auto it = std::find_if(
        adapter->service().devices().begin(),
        adapter->service().devices().end(),
        [=](const auto & device){ return device->serial() == serial; }
    );
    if (it != adapter->service().devices().end()) {
        (*it)->handleKeyEvent(key, true);
        (*it)->handleKeyEvent(key, false);
    }
    return 0;
}

static constexpr sd_bus_vtable interfaceVtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_PROPERTY("configurationPath", "s", getConfigurationPath, 0, 0),
    SD_BUS_PROPERTY("context", "a{ss}", getContext, 0, 0),
    SD_BUS_WRITABLE_PROPERTY("autoQuit", "b", getAutoQuit, setAutoQuit, 0, 0),
    SD_BUS_PROPERTY("devices", "ao", getDevices, 0, 0),
    SD_BUS_PROPERTY("plugins", "as", getPlugins, 0, 0),
    SD_BUS_SIGNAL("deviceAdded", "o", 0),
    SD_BUS_SIGNAL("deviceRemoved", "o", 0),
    SD_BUS_METHOD("setContextValues", "a{ss}", "", setContextValues,
                  SD_BUS_VTABLE_METHOD_NO_REPLY | SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("setContextValue", "ss", "", setContextValue,
                  SD_BUS_VTABLE_METHOD_NO_REPLY | SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("sendGenericEvent", "a{ss}", "", sendGenericEvent,
                  SD_BUS_VTABLE_METHOD_NO_REPLY | SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_METHOD("sendKeyEvent", "sq", "", sendKeyEvents,   // serial, key
                  SD_BUS_VTABLE_METHOD_NO_REPLY | SD_BUS_VTABLE_UNPRIVILEGED),
    SD_BUS_VTABLE_END
};

/****************************************************************************/

ServiceAdapter::ServiceAdapter(sd_bus * bus, Service & service)
 : m_bus(bus), m_service(service), m_slot(nullptr)
{
    sd_bus_ref(bus);
    sd_bus_add_object_vtable(m_bus, &m_slot, objectPath, interfaceName,
                             interfaceVtable, this);

    using namespace std::placeholders;
    connect(m_service.deviceManagerAdded, this,
            std::bind(&ServiceAdapter::onDeviceAdded, this, _1));
    connect(m_service.deviceManagerRemoved, this,
            std::bind(&ServiceAdapter::onDeviceRemoved, this, _1));
}

ServiceAdapter::~ServiceAdapter()
{
    disconnect(m_service.deviceManagerAdded, this);
    disconnect(m_service.deviceManagerRemoved, this);

    sd_bus_slot_unref(m_slot);
    sd_bus_unref(m_bus);
}

void ServiceAdapter::onDeviceAdded(DeviceManager & device)
{
    m_devices.emplace_back(std::make_unique<DeviceManagerAdapter>(m_bus, device));
    sd_bus_emit_signal(m_bus, objectPath, interfaceName, "deviceAdded",
                       "o", DeviceManagerAdapter::pathFor(device).c_str());
}

void ServiceAdapter::onDeviceRemoved(DeviceManager & device)
{
    using std::swap;
    sd_bus_emit_signal(m_bus, objectPath, interfaceName, "deviceRemoved",
                       "o", DeviceManagerAdapter::pathFor(device).c_str());
    auto it = std::find_if(m_devices.begin(), m_devices.end(),
                           [&](const auto & item){ return &item->device() == &device; });
    assert(it != m_devices.end());

    if (it != m_devices.end() - 1) { std::swap(*it, m_devices.back()); }
    m_devices.pop_back();
}
