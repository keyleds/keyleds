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
#include "keyledsd/utils.h"

#include <cstdlib>

/** Parse an unsigned integer.
 * Simple wrapper around std::strtoul() with a more convenient interface.
 * @param str String representation of number, is base 10.
 * @param [out] value Variable to write result into.
 * @return `true` on success, `false` if `str` does not represent an unsigned integer.
 */
std::optional<unsigned long> keyleds::parseNumber(const std::string & str)
{
    if (str.empty()) { return {}; }
    char * end;
    auto result = std::strtoul(str.c_str(), &end, 10);
    if (*end != '\0') { return {}; }
    return result;
}
