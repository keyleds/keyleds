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
#ifndef KEYLEDSD_COMMON_H_D31E341B
#define KEYLEDSD_COMMON_H_D31E341B

#include <iosfwd>
#include <limits>

namespace keyleds {

/****************************************************************************/

/** RGB color POD
 *
 * Holds a single R8G8B8 value. Not intended to be tightly packed.
 */
struct RGBColor final {
    typedef unsigned char channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;

    RGBColor() = default;
    RGBColor(channel_type r, channel_type g, channel_type b)
     : red(r), green(g), blue(b) {}

    static RGBColor parse(const std::string &);
    void print(std::ostream &) const;
};

static inline bool operator==(const RGBColor & a, const RGBColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBColor & a, const RGBColor & b) { return !(a == b); }

static inline std::ostream & operator<<(std::ostream & out, const RGBColor & obj)
{
    obj.print(out);
    return out;
}

/****************************************************************************/

/** RGBA color POD
 *
 * Holds a single R8G8B8A8 value. Should not generate any padding, but remember
 * to static_assert sizeof(RGBAColor) == 4 if using that fact.
 */
struct RGBAColor final {
    typedef unsigned char channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;
    channel_type alpha;

    RGBAColor() = default;
    RGBAColor(channel_type r, channel_type g, channel_type b, channel_type a)
     : red(r), green(g), blue(b), alpha(a) {}
    explicit RGBAColor(const RGBColor & c, channel_type a = std::numeric_limits<channel_type>::max())
     : red(c.red), green(c.green), blue(c.blue), alpha(a) {}

    static RGBAColor parse(const std::string &);
    void print(std::ostream &) const;
};

static inline bool operator==(const RGBAColor & a, const RGBAColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBAColor & a, const RGBAColor & b) { return !(a == b); }

static inline std::ostream & operator<<(std::ostream & out, const RGBAColor & obj)
{
    obj.print(out);
    return out;
}

/****************************************************************************/

};

#endif
