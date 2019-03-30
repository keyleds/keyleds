/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017-2019 Julien Hartmann, juli1.hartmann@gmail.com
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
#include "libkeyleds/logitech/Device.h"
#include "libkeyleds/HIDParser.h"

#include "libkeyleds/utils.h"
#include <cerrno>
#include <fcntl.h>
#include <linux/hidraw.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <system_error>

namespace libkeyleds::logitech {

void Device::open(const char * path, uint8_t appId,
                  std::function<void(std::unique_ptr<Device>)> onOpen,
                  std::function<void(std::error_code)> onError)
{
#if _POSIX_C_SOURCE >= 200809L
    auto fd = FileDescriptor(open(path, O_RDWR | O_CLOEXEC));
    if (!fd) { onError(std::error_code(errno, std::system_category)); return; }
#else
    auto fd = FileDescriptor(open(path, O_RDWR));
    if (!fd) { onError(std::error_code(errno, std::system_category)); return; }

    fcntl(fd.get(), F_SETFD, FD_CLOEXEC);
#endif

    struct hidraw_devinfo devinfo;
    if (ioctl(fd.get(), HIDIOCGRAWINFO, &devinfo) < 0) {
        onError(std::error_code(errno, std::system_category));
        return;
    }
    if (devinfo.vendor != logitechVendorId) {
        onError(std::error_code(InvalidDevice, libkeyleds_category));
        return;
    }

    struct hidraw_report_descriptor descriptorData;
    if (ioctl(fd.get(), HIDIOCGRDESCSIZE, &descriptor.size) < 0 ||
        ioctl(fd.get(), HIDIOCGRDESC, &descriptor) < 0) {
        onError(std::error_code(errno, std::system_category));
        return;
    }
    auto descriptor = hid::parse(descriptorData.value, descriptorData.size);
    if (!descriptor) {
        onError(std::error_code(InvalidDevice, libkeyleds_category));
        return;
    }

    auto endpoint = hid::Endpoint(loop, std::move(fd), maxReportSize);

    // get protocol
    // ping
    // query features
}
