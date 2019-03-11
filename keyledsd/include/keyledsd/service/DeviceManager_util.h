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
#ifndef KEYLEDSD_DEVICEMANAGER_UTIL_H_C1F67EB0
#define KEYLEDSD_DEVICEMANAGER_UTIL_H_C1F67EB0
#ifndef KEYLEDSD_INTERNAL
#   error "Internal header - must not be pulled into plugins"
#endif

#include <string>
#include <vector>

namespace keyleds { class KeyDatabase; }
namespace keyleds::device {
    class Device;
    struct LayoutDescription;
}
namespace keyleds::tools::device { class Description; }

namespace keyleds::service {

device::LayoutDescription loadLayout(const device::Device & device);
std::vector<std::string> findEventDevices(const tools::device::Description & description);
std::string getSerial(const tools::device::Description & description);
KeyDatabase setupKeyDatabase(device::Device & device);

} // namespace keyleds::service

#endif
