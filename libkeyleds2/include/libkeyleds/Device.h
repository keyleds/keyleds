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
#ifndef LIBKEYLEDS_DEVICE_H_A35EF9C0
#define LIBKEYLEDS_DEVICE_H_A35EF9C0

#include "libkeyleds/Feature.h"
#include <string>
#include <vector>

namespace libkeyleds {

/****************************************************************************/

class Device
{
public:
    enum class Type { Keyboard, Remote, NumPad, Mouse, TouchPad, TrackBall, Presenter, Receiver };

public:
                Device(const Device &) = delete;
    Device &    operator=(Device &&) noexcept = delete;
    virtual     ~Device() = 0;

    [[nodiscard]] auto &    path() const noexcept { return m_path; }
    [[nodiscard]] Type      type() const noexcept { return m_type; }
    [[nodiscard]] auto &    name() const noexcept { return m_name; }
    [[nodiscard]] auto &    model() const noexcept { return m_model; }
    [[nodiscard]] auto &    serial() const noexcept { return m_serial; }

    virtual bool        resync() = 0;

    [[nodiscard]] virtual const std::vector<Feature::type_id_t> & features() const = 0;
    [[nodiscard]] virtual Feature *   getFeature(Feature::type_id_t) = 0;

    template <typename T>
    [[nodiscard]] T * getFeature() { return static_cast<T*>(this->getFeature(T::type_id)); }

protected:
    Device(std::string path, Type type, std::string name, std::string model, std::string serial)
     : m_path(std::move(path)), m_type(type),
       m_name(std::move(name)), m_model(std::move(model)), m_serial(std::move(serial))
       {}

private:
    std::string m_path;     ///< Device node path
    Type        m_type;     ///< The kind of libkeyleds device
    std::string m_name;     ///< User-friendly name of the device, eg "Logitech G410"
    std::string m_model;    ///< Technical model identification string, eg "c3300000"
    std::string m_serial;   ///< Serial number, not required to be unique
};

/****************************************************************************/

}

#endif
