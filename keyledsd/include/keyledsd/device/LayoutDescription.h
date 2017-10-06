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

namespace keyleds { namespace device {

/****************************************************************************/

/** Keyboard layout description
 *
 * Describes the physical layout of a keyboard: which keys are available, where
 * exactly they are on the keyboard, and what size the whole keyboard is.
 */
class LayoutDescription final
{
public:
    struct Rect { unsigned x0, y0, x1, y1; };
    class Key;
    class ParseError;
    using key_list = std::vector<Key>;
public:
                        LayoutDescription() = default;
                        LayoutDescription(std::string name, key_list keys);
                        ~LayoutDescription();

    const std::string & name() const { return m_name; }
    const key_list &    keys() const { return m_keys; }

    static LayoutDescription parse(std::istream &);
    static LayoutDescription loadFile(const std::string & path);

private:
    std::string         m_name;     ///< Layout name, indicating its country code
    key_list            m_keys;     ///< All keys from all blocks
};

/****************************************************************************/

/** Key layout description
 *
 * Describes the physical characteristics of a single key.
 */
class LayoutDescription::Key final
{
    using block_type = unsigned int;
    using code_type = unsigned int;
public:
                Key(block_type block, code_type code, Rect position, std::string name);
                ~Key();
public:
    block_type  block;          ///< Block identifier, eg: 0 for normal keys, 64 for game/light keys, ...
    code_type   code;           ///< Key identifier within block
    Rect        position;       ///< Physical key bounds. [0, 0] is upper left corner.
    std::string name;           ///< User-readable key name
};

/****************************************************************************/

class LayoutDescription::ParseError : public std::runtime_error
{
public:
                ParseError(const std::string & what, int line);
                ~ParseError();

    int         line() const noexcept { return m_line; }
private:
    int         m_line; ///< Line of the parsing error, as reported by xml lib
};

/****************************************************************************/

} } // namespace keyleds::device

#endif
