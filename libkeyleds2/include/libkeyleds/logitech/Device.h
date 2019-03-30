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
#ifndef LIBKEYLEDS_LOGITECH_DEVICE_H_0EEAF81D
#define LIBKEYLEDS_LOGITECH_DEVICE_H_0EEAF81D

#include "libkeyleds/Device.h"
#include "libkeyleds/Feature.h"

#include <memory>
#include <system_error>

namespace libkeyleds::logitech {

/****************************************************************************/

class Device : public libkeyleds::Device
{
public:
    ~Device() override;

    static void open(const char * path, uint8_t appId,
                     std::function<void(std::unique_ptr<Device>)> onOpen,
                     std::function<void(std::error_code)> onError);

    bool    resync() override;
    const std::vector<Feature::type_id_t> & features() const override;
    Feature * getFeature(Feature::type_id_t) override;
};

/****************************************************************************/

} // namespace libkeyleds::logitech

#endif
