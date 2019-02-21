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
#ifndef KEYLEDSD_DBUS_SERVICEADAPTOR_H_A616A05A
#define KEYLEDSD_DBUS_SERVICEADAPTOR_H_A616A05A
#ifndef KEYLEDSD_INTERNAL
#   error "Internal header - must not be pulled into plugins"
#endif

#include <memory>
#include <vector>

struct sd_bus;
struct sd_bus_slot;
namespace keyleds::service {
    class DeviceManager;
    class Service;
}

namespace keyleds::service::dbus {

class DeviceManagerAdapter;

/****************************************************************************/

class ServiceAdapter final
{
public:
                        ServiceAdapter(sd_bus *, Service &);
                        ServiceAdapter(const ServiceAdapter &) = delete;
    ServiceAdapter &    operator=(const ServiceAdapter &) = delete;
                        ~ServiceAdapter();

    Service &           service() noexcept { return m_service; }

private:
    void                onDeviceAdded(DeviceManager &);
    void                onDeviceRemoved(DeviceManager &);

private:
    sd_bus *        m_bus;
    Service &       m_service;
    sd_bus_slot *   m_slot;
    std::vector<std::unique_ptr<DeviceManagerAdapter>>  m_devices;
};

/****************************************************************************/

} // namespace keyleds::service::dbus

#endif
