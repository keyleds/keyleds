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
#include "libkeyleds/HIDParser.h"

#include <cstdint>
#include <iostream>

static constexpr std::size_t maxSize = 256;

int main()
{
    std::ios_base::sync_with_stdio(false);

    uint8_t descriptor[maxSize];
    std::cin.read(reinterpret_cast<char *>(descriptor), maxSize);

    std::cout <<"Parsing " <<std::cin.gcount() <<" bytes" <<std::endl;

    auto description = libkeyleds::hid::parse(descriptor, std::size_t(std::cin.gcount()));
    if (description) {
        std::cout <<description->collections.size() <<" collections, "
                  <<description->reports.size() <<" reports\n";
    } else {
        std::cout <<"invalid\n";
    }
    return 0;
}
