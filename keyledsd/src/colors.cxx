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
#include "keyledsd/colors.h"

#include "config.h"
#include <algorithm>
#include <array>
#include <cstdint>
#include <cstdlib>
#include <iomanip>
#include <ostream>
#include <stdexcept>
#include <string>
#include <utility>

using keyleds::RGBColor;
using keyleds::RGBAColor;

/** Table of color names. Must be sorted alphabetically so it can use bisect search.
 * Those colors are those defined in CSS standard.
 */
static constexpr std::array<std::pair<const char *, RGBColor>, 149> predefinedColors = {{
    { "aliceblue", { 0xF0, 0xF8, 0xFF } },
    { "antiquewhite", { 0xFA, 0xEB, 0xD7 } },
    { "aqua", { 0x00, 0xFF, 0xFF } },
    { "aquamarine", { 0x7F, 0xFF, 0xD4 } },
    { "azure", { 0xF0, 0xFF, 0xFF } },
    { "beige", { 0xF5, 0xF5, 0xDC } },
    { "bisque", { 0xFF, 0xE4, 0xC4 } },
    { "black", { 0x00, 0x00, 0x00 } },
    { "blanchedalmond", { 0xFF, 0xEB, 0xCD } },
    { "blue", { 0x00, 0x00, 0xFF } },
    { "blueviolet", { 0x8A, 0x2B, 0xE2 } },
    { "brown", { 0xA5, 0x2A, 0x2A } },
    { "burlywood", { 0xDE, 0xB8, 0x87 } },
    { "cadetblue", { 0x5F, 0x9E, 0xA0 } },
    { "chartreuse", { 0x7F, 0xFF, 0x00 } },
    { "chocolate", { 0xD2, 0x69, 0x1E } },
    { "coral", { 0xFF, 0x7F, 0x50 } },
    { "cornflowerblue", { 0x64, 0x95, 0xED } },
    { "cornsilk", { 0xFF, 0xF8, 0xDC } },
    { "crimson", { 0xDC, 0x14, 0x3C } },
    { "cyan", { 0x00, 0xFF, 0xFF } },
    { "darkblue", { 0x00, 0x00, 0x8B } },
    { "darkcyan", { 0x00, 0x8B, 0x8B } },
    { "darkgoldenrod", { 0xB8, 0x86, 0x0B } },
    { "darkgray", { 0xA9, 0xA9, 0xA9 } },
    { "darkgreen", { 0x00, 0x64, 0x00 } },
    { "darkgrey", { 0xA9, 0xA9, 0xA9 } },
    { "darkkhaki", { 0xBD, 0xB7, 0x6B } },
    { "darkmagenta", { 0x8B, 0x00, 0x8B } },
    { "darkolivegreen", { 0x55, 0x6B, 0x2F } },
    { "darkorange", { 0xFF, 0x8C, 0x00 } },
    { "darkorchid", { 0x99, 0x32, 0xCC } },
    { "darkred", { 0x8B, 0x00, 0x00 } },
    { "darksalmon", { 0xE9, 0x96, 0x7A } },
    { "darkseagreen", { 0x8F, 0xBC, 0x8F } },
    { "darkslateblue", { 0x48, 0x3D, 0x8B } },
    { "darkslategray", { 0x2F, 0x4F, 0x4F } },
    { "darkslategrey", { 0x2F, 0x4F, 0x4F } },
    { "darkturquoise", { 0x00, 0xCE, 0xD1 } },
    { "darkviolet", { 0x94, 0x00, 0xD3 } },
    { "deeppink", { 0xFF, 0x14, 0x93 } },
    { "deepskyblue", { 0x00, 0xBF, 0xFF } },
    { "dimgray", { 0x69, 0x69, 0x69 } },
    { "dimgrey", { 0x69, 0x69, 0x69 } },
    { "dodgerblue", { 0x1E, 0x90, 0xFF } },
    { "firebrick", { 0xB2, 0x22, 0x22 } },
    { "floralwhite", { 0xFF, 0xFA, 0xF0 } },
    { "forestgreen", { 0x22, 0x8B, 0x22 } },
    { "fuchsia", { 0xFF, 0x00, 0xFF } },
    { "gainsboro", { 0xDC, 0xDC, 0xDC } },
    { "ghostwhite", { 0xF8, 0xF8, 0xFF } },
    { "gold", { 0xFF, 0xD7, 0x00 } },
    { "goldenrod", { 0xDA, 0xA5, 0x20 } },
    { "gray", { 0x80, 0x80, 0x80 } },
    { "green", { 0x00, 0x80, 0x00 } },
    { "greenyellow", { 0xAD, 0xFF, 0x2F } },
    { "grey", { 0x80, 0x80, 0x80 } },
    { "honeydew", { 0xF0, 0xFF, 0xF0 } },
    { "hotpink", { 0xFF, 0x69, 0xB4 } },
    { "indianred", { 0xCD, 0x5C, 0x5C } },
    { "indigo", { 0x4B, 0x00, 0x82 } },
    { "ivory", { 0xFF, 0xFF, 0xF0 } },
    { "khaki", { 0xF0, 0xE6, 0x8C } },
    { "lavender", { 0xE6, 0xE6, 0xFA } },
    { "lavenderblush", { 0xFF, 0xF0, 0xF5 } },
    { "lawngreen", { 0x7C, 0xFC, 0x00 } },
    { "lemonchiffon", { 0xFF, 0xFA, 0xCD } },
    { "lightblue", { 0xAD, 0xD8, 0xE6 } },
    { "lightcoral", { 0xF0, 0x80, 0x80 } },
    { "lightcyan", { 0xE0, 0xFF, 0xFF } },
    { "lightgoldenrodyellow", { 0xFA, 0xFA, 0xD2 } },
    { "lightgray", { 0xD3, 0xD3, 0xD3 } },
    { "lightgreen", { 0x90, 0xEE, 0x90 } },
    { "lightgrey", { 0xD3, 0xD3, 0xD3 } },
    { "lightpink", { 0xFF, 0xB6, 0xC1 } },
    { "lightsalmon", { 0xFF, 0xA0, 0x7A } },
    { "lightseagreen", { 0x20, 0xB2, 0xAA } },
    { "lightskyblue", { 0x87, 0xCE, 0xFA } },
    { "lightslategray", { 0x77, 0x88, 0x99 } },
    { "lightslategrey", { 0x77, 0x88, 0x99 } },
    { "lightsteelblue", { 0xB0, 0xC4, 0xDE } },
    { "lightyellow", { 0xFF, 0xFF, 0xE0 } },
    { "lime", { 0x00, 0xFF, 0x00 } },
    { "limegreen", { 0x32, 0xCD, 0x32 } },
    { "linen", { 0xFA, 0xF0, 0xE6 } },
    { "logitech", { 0x00, 0xCD, 0xFF } },       // Logitech default keyboard color
    { "magenta", { 0xFF, 0x00, 0xFF } },
    { "maroon", { 0x80, 0x00, 0x00 } },
    { "mediumaquamarine", { 0x66, 0xCD, 0xAA } },
    { "mediumblue", { 0x00, 0x00, 0xCD } },
    { "mediumorchid", { 0xBA, 0x55, 0xD3 } },
    { "mediumpurple", { 0x93, 0x70, 0xDB } },
    { "mediumseagreen", { 0x3C, 0xB3, 0x71 } },
    { "mediumslateBlue", { 0x7B, 0x68, 0xEE } },
    { "mediumspringGreen", { 0x00, 0xFA, 0x9A } },
    { "mediumturquoise", { 0x48, 0xD1, 0xCC } },
    { "mediumvioletRed", { 0xC7, 0x15, 0x85 } },
    { "midnightblue", { 0x19, 0x19, 0x70 } },
    { "mintcream", { 0xF5, 0xFF, 0xFA } },
    { "mistyrose", { 0xFF, 0xE4, 0xE1 } },
    { "moccasin", { 0xFF, 0xE4, 0xB5 } },
    { "navajowhite", { 0xFF, 0xDE, 0xAD } },
    { "navy", { 0x00, 0x00, 0x80 } },
    { "oldlace", { 0xFD, 0xF5, 0xE6 } },
    { "olive", { 0x80, 0x80, 0x00 } },
    { "olivedrab", { 0x6B, 0x8E, 0x23 } },
    { "orange", { 0xFF, 0xA5, 0x00 } },
    { "orangered", { 0xFF, 0x45, 0x00 } },
    { "orchid", { 0xDA, 0x70, 0xD6 } },
    { "palegoldenrod", { 0xEE, 0xE8, 0xAA } },
    { "palegreen", { 0x98, 0xFB, 0x98 } },
    { "paleturquoise", { 0xAF, 0xEE, 0xEE } },
    { "palevioletred", { 0xDB, 0x70, 0x93 } },
    { "papayawhip", { 0xFF, 0xEF, 0xD5 } },
    { "peachpuff", { 0xFF, 0xDA, 0xB9 } },
    { "peru", { 0xCD, 0x85, 0x3F } },
    { "pink", { 0xFF, 0xC0, 0xCB } },
    { "plum", { 0xDD, 0xA0, 0xDD } },
    { "powderblue", { 0xB0, 0xE0, 0xE6 } },
    { "purple", { 0x80, 0x00, 0x80 } },
    { "rebeccapurple", { 0x66, 0x33, 0x99 } },
    { "red", { 0xFF, 0x00, 0x00 } },
    { "rosybrown", { 0xBC, 0x8F, 0x8F } },
    { "royalblue", { 0x41, 0x69, 0xE1 } },
    { "saddlebrown", { 0x8B, 0x45, 0x13 } },
    { "salmon", { 0xFA, 0x80, 0x72 } },
    { "sandybrown", { 0xF4, 0xA4, 0x60 } },
    { "seagreen", { 0x2E, 0x8B, 0x57 } },
    { "seashell", { 0xFF, 0xF5, 0xEE } },
    { "sienna", { 0xA0, 0x52, 0x2D } },
    { "silver", { 0xC0, 0xC0, 0xC0 } },
    { "skyblue", { 0x87, 0xCE, 0xEB } },
    { "slateblue", { 0x6A, 0x5A, 0xCD } },
    { "slategray", { 0x70, 0x80, 0x90 } },
    { "slategrey", { 0x70, 0x80, 0x90 } },
    { "snow", { 0xFF, 0xFA, 0xFA } },
    { "springgreen", { 0x00, 0xFF, 0x7F } },
    { "steelblue", { 0x46, 0x82, 0xB4 } },
    { "tan", { 0xD2, 0xB4, 0x8C } },
    { "teal", { 0x00, 0x80, 0x80 } },
    { "thistle", { 0xD8, 0xBF, 0xD8 } },
    { "tomato", { 0xFF, 0x63, 0x47 } },
    { "turquoise", { 0x40, 0xE0, 0xD0 } },
    { "violet", { 0xEE, 0x82, 0xEE } },
    { "wheat", { 0xF5, 0xDE, 0xB3 } },
    { "white", { 0xFF, 0xFF, 0xFF } },
    { "whitesmoke", { 0xF5, 0xF5, 0xF5 } },
    { "yellow", { 0xFF, 0xFF, 0x00 } },
    { "yellowgreen", { 0x9A, 0xCD, 0x32 } }
}};
static_assert(predefinedColors.back().second == RGBColor(0x9A, 0xCD, 0x32),
              "Last predefined color is not the expected one - is length correct?");


/** Parse a string into an RGBColor.
 * @param str Color name or hexadecimal string in RRGGBB format.
 * @param [out] color Pointer to RGBColor object to fill.
 * @return `true` on success, `false` on error.
 * @bug Does not accept print() output. Stripping an optional leading hash would allow that.
 */
KEYLEDSD_EXPORT std::optional<RGBColor> RGBColor::parse(const std::string & str)
{
    // Attempt parsing as hex color
    if (str.size() == 6) {
        char * endptr;
        auto code = uint32_t(::strtoul(str.c_str(), &endptr, 16));
        if (*endptr == '\0') {
            return RGBColor(channel_type(code >> 16u),
                            channel_type(code >> 8u),
                            channel_type(code >> 0u));
        }
    }

    // Attempt using a predefined color
    std::string lower;  // to allow case mismatch, make a lowercase version
    lower.reserve(str.size());
    std::transform(str.begin(), str.end(), std::back_inserter(lower), ::tolower);

    const auto it = std::lower_bound(
        predefinedColors.begin(), predefinedColors.end(),
        lower, [](const auto & item, const auto & name) { return item.first < name; }
    );
    if (it != predefinedColors.end() && it->first == lower) {
        return it->second;
    }

    return {};
}

/** Write a human-readable representation of color.
 * @param out Output stream to write color representation into.
 */
KEYLEDSD_EXPORT void RGBColor::print(std::ostream & out) const
{
    const auto flags = out.flags(std::ios_base::hex | std::ios_base::right);
    const auto fillChar = out.fill('0');
    out <<'#' <<std::setw(2) <<int(red) <<std::setw(2) <<int(green) <<std::setw(2) <<int(blue);
    out.flags(flags);
    out.fill(fillChar);
}

/** Parse a string into an RGBAColor.
 * @param str Color name or hexadecimal string in RRGGBBAA or RRGGBB format.
 * @param [out] color Pointer to RGBAColor object to fill.
 * @return `true` on success, `false` on error.
 * @bug Does not accept print() output. Stripping an optional leading hash would allow that.
 */
KEYLEDSD_EXPORT std::optional<RGBAColor> RGBAColor::parse(const std::string & str)
{
    if (str.size() == 8) {
        char * endptr;
        auto code = uint32_t(::strtoul(str.c_str(), &endptr, 16));
        if (*endptr == '\0') {
            return RGBAColor(channel_type(code >> 24u),
                             channel_type(code >> 16u),
                             channel_type(code >> 8u),
                             channel_type(code >> 0u));
        }
    }

    auto color = RGBColor::parse(str);
    if (color) { return RGBAColor(*color, 0xff); }
    return std::nullopt;
}

/** Write a human-readable representation of color.
 * @param out Output stream to write color representation into.
 */
KEYLEDSD_EXPORT void RGBAColor::print(std::ostream & out) const
{
    const auto flags = out.flags(std::ios_base::hex | std::ios_base::right);
    const auto fillChar = out.fill('0');
    out <<'#' <<std::setw(2) <<int(red) <<std::setw(2) <<int(green) <<std::setw(2) <<int(blue);
    out <<std::setw(2) <<int(alpha);
    out.flags(flags);
    out.fill(fillChar);
}
