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
#include "keyledsd/RenderTarget.h"

#include <memory>
#include <type_traits>

using keyleds::RenderTarget;

static_assert(std::is_pod<keyleds::RGBAColor>::value, "RGBAColor must be a POD type");
static_assert(sizeof(keyleds::RGBAColor) == 4, "RGBAColor must be tightly packed");

// 16 is minimum for SSE2, 32 for AVX2
static constexpr auto alignBytes = static_cast<std::align_val_t>(32);
static constexpr auto alignColors = static_cast<RenderTarget::size_type>(
    static_cast<unsigned>(alignBytes) / sizeof(keyleds::RGBAColor)
);


/// Returns the given value, aligned to upper bound of given aligment
template <typename T> constexpr T align(T value, T alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/****************************************************************************/

RenderTarget::RenderTarget(size_type size)
 : m_size(size),                            // m_size tracks actual number of keys
   m_capacity(align(size, alignColors)),    // m_capacity tracks actual buffer size
   m_colors(new (operator new[](m_capacity * sizeof(RGBAColor), alignBytes)) RGBAColor[m_size])
{}

RenderTarget::~RenderTarget()
{
    std::destroy(begin(), end());
    operator delete[](m_colors, alignBytes);
}

void RenderTarget::clear() noexcept
{
    std::destroy(begin(), end());
    operator delete[](m_colors, alignBytes);
    m_size = 0;
    m_capacity = 0;
    m_colors = nullptr;
}
