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
#ifndef KEYLEDSD_UTILS_H_9ADABC7C
#define KEYLEDSD_UTILS_H_9ADABC7C

#include <chrono>
#include <optional>
#include <string>
#include "keyledsd_config.h"

namespace keyleds {

KEYLEDSD_EXPORT std::optional<unsigned long> parseNumber(const std::string & str);

template <typename T>
std::optional<T> parseDuration(const std::string & str)
{
    auto value = parseNumber(str);
    if (!value) { return {}; }
    return std::chrono::duration_cast<T>(std::chrono::milliseconds(*value));
}

}

#endif
