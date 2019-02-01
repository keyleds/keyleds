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
#include "keyledsd/dbus/DeviceManager.h"

#include "keyledsd/DeviceManager.h"
#include "keyledsd/Service.h"
#include <systemd/sd-bus.h>

using keyleds::dbus::DeviceManagerAdapter;

/****************************************************************************/

static constexpr char interfaceName[] = "org.etherdream.keyleds.DeviceManager";

/****************************************************************************/

static int getSysPath(sd_bus *, const char *, const char *, const char *,
                      sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().sysPath().c_str());
}

static int getSerial(sd_bus *, const char *, const char *, const char *,
                     sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().serial().c_str());
}

static int getDevNode(sd_bus *, const char *, const char *, const char *,
                      sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().device().path().c_str());
}

static int getEventDevices(sd_bus *, const char *, const char *, const char *,
                           sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);

    ret = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "s");
    if (ret < 0) { return ret; }
    for (const auto & path : adapter->device().eventDevices()) {
        ret = sd_bus_message_append_basic(reply, 's', path.c_str());
        if (ret < 0) { return ret; }
    }
    return sd_bus_message_close_container(reply);
}

static int getName(sd_bus *, const char *, const char *, const char *,
                   sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().device().name().c_str());
}

static int getModel(sd_bus *, const char *, const char *, const char *,
                    sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().device().model().c_str());
}

static int getFirmware(sd_bus *, const char *, const char *, const char *,
                       sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append_basic(reply, 's', adapter->device().device().firmware().c_str());
}

static int getKeys(sd_bus *, const char *, const char *, const char *,
                   sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);

    ret = sd_bus_message_open_container(reply, SD_BUS_TYPE_ARRAY, "(qs(qqqq))");
    if (ret < 0) { return ret; }
    for (const auto & key : adapter->device().keyDB()) {
        ret = sd_bus_message_append(reply, "(qs(qqqq))",
            key.keyCode,
            key.name.c_str(),
            key.position.x0, key.position.y0,
            key.position.x1, key.position.y1
        );
        if (ret < 0) { return ret; }
    }
    return sd_bus_message_close_container(reply);
}

static int getPaused(sd_bus *, const char *, const char *, const char *,
                     sd_bus_message * reply, void * userdata, sd_bus_error *)
{
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    return sd_bus_message_append(reply, "b", adapter->device().paused());
}

static int setPaused(sd_bus *, const char *, const char *, const char *,
                     sd_bus_message * value, void * userdata, sd_bus_error *)
{
    int ret;
    auto adapter = reinterpret_cast<DeviceManagerAdapter *>(userdata);
    int paused;

    ret = sd_bus_message_read_basic(value, 'b', &paused);
    if (ret < 0) { return ret; }
    adapter->device().setPaused(bool(paused));
    return 0;
}

static constexpr sd_bus_vtable interfaceVtable[] = {
    SD_BUS_VTABLE_START(0),
    SD_BUS_PROPERTY("sysPath", "s", getSysPath, 0, 0),
    SD_BUS_PROPERTY("serial", "s", getSerial, 0, 0),
    SD_BUS_PROPERTY("devNode", "s", getDevNode, 0, 0),
    SD_BUS_PROPERTY("eventDevices", "as", getEventDevices, 0, 0),
    SD_BUS_PROPERTY("name", "s", getName, 0, 0),
    SD_BUS_PROPERTY("model", "s", getModel, 0, 0),
    SD_BUS_PROPERTY("firmware", "s", getFirmware, 0, 0),
    SD_BUS_PROPERTY("keys", "a(qs(qqqq))", getKeys, 0, 0),
    SD_BUS_WRITABLE_PROPERTY("paused", "b", getPaused, setPaused, 0, 0),
    SD_BUS_VTABLE_END
};

/****************************************************************************/

DeviceManagerAdapter::DeviceManagerAdapter(sd_bus * bus, DeviceManager & device)
 : m_bus(bus), m_device(device)
{
    sd_bus_ref(bus);
    sd_bus_add_object_vtable(m_bus, &m_slot, pathFor(device).c_str(), interfaceName,
                             interfaceVtable, this);
}

DeviceManagerAdapter::~DeviceManagerAdapter()
{
    sd_bus_slot_unref(m_slot);
    sd_bus_unref(m_bus);
}

std::string DeviceManagerAdapter::pathFor(const DeviceManager & manager)
{
    return "/Device/" + manager.serial();
}
