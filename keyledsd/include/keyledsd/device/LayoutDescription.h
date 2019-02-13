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
#ifndef KEYLEDSD_LAYOUTDESCRIPTION_H_FF3532D2
#define KEYLEDSD_LAYOUTDESCRIPTION_H_FF3532D2

#include <iosfwd>
#include <stdexcept>
#include <string>
#include <vector>

namespace keyleds::device {

/****************************************************************************/

/** Keyboard layout description
 *
 * Describes the physical layout of a keyboard: which keys are available, where
 * exactly they are on the keyboard, and what size the whole keyboard is.
 */
struct LayoutDescription final
{
    using block_type = unsigned int;
    using code_type = unsigned int;

    struct Key;
    struct Rect final { unsigned x0, y0, x1, y1; };

    class ParseError : public std::runtime_error { using runtime_error::runtime_error; };
    using key_list = std::vector<Key>;
    using pos_list = std::vector<std::pair<block_type, code_type>>;

    static LayoutDescription parse(std::istream &);
    static LayoutDescription loadFile(const std::string & path);

    std::string name;       ///< Layout name, indicating its country code
    key_list    keys;       ///< All keys from all blocks
    pos_list    spurious;   ///< Position of blacklisted keys
};

/****************************************************************************/

struct LayoutDescription::Key final
{
    block_type  block;      ///< Block identifier, eg: normal keys, game/light keys, ...
    code_type   code;       ///< Key identifier within block
    Rect        position;   ///< Physical key bounds. [0, 0] is upper left corner.
    std::string name;       ///< User-readable key name
};

/****************************************************************************/

} // namespace keyleds::device

#endif
