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
#include "keyledsd/KeyDatabase.h"

using keyleds::KeyDatabase;

/****************************************************************************/

KeyDatabase::KeyDatabase(key_map keys)
 : m_keys(std::move(keys)),
   m_bounds(computeBounds(m_keys))
{}

KeyDatabase::iterator KeyDatabase::find(RenderTarget::key_descriptor index) const
{
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
        if (it->second.index == index) { return it; }
    }
    return m_keys.end();
}

KeyDatabase::Key::Rect KeyDatabase::computeBounds(const key_map & keys)
{
    auto result = Key::Rect{
        keys.begin()->second.position.x0,
        keys.begin()->second.position.y0,
        keys.begin()->second.position.x1,
        keys.begin()->second.position.y1
    };
    for (const auto & item : keys) {
        const auto & pos = item.second.position;
        if (pos.x0 < result.x0) { result.x0 = pos.x0; }
        if (pos.y0 < result.y0) { result.y0 = pos.y0; }
        if (pos.x1 > result.x1) { result.x1 = pos.x1; }
        if (pos.y1 > result.y1) { result.y1 = pos.y1; }
    }
    return result;
}
