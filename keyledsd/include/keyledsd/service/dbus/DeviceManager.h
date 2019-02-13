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
#ifndef KEYLEDSD_DBUS_DEVICEMANAGERADAPTOR_H_A3B0E2B4
#define KEYLEDSD_DBUS_DEVICEMANAGERADAPTOR_H_A3B0E2B4

#include <string>

struct sd_bus;
struct sd_bus_slot;
namespace keyleds::service { class DeviceManager; }

namespace keyleds::service::dbus {

/****************************************************************************/

class DeviceManagerAdapter final
{
public:
                        DeviceManagerAdapter(sd_bus *, DeviceManager &);
                        DeviceManagerAdapter(const DeviceManagerAdapter &) = delete;
    DeviceManagerAdapter & operator=(const DeviceManagerAdapter &) = delete;
                        ~DeviceManagerAdapter();

    DeviceManager &     device() noexcept { return m_device; }

    static std::string  pathFor(const DeviceManager & manager);

private:
    sd_bus *        m_bus;
    sd_bus_slot *   m_slot;
    DeviceManager & m_device;
};

/****************************************************************************/

} // namespace keyleds::service::dbus

#endif
