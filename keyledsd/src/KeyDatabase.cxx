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

#include "config.h"
#include <algorithm>
#include <cassert>
#include <cmath>
#include <ostream>

using keyleds::KeyDatabase;

/****************************************************************************/

template <class T> T abs_difference(T a, T b) { return a > b ? a - b : b - a; }

// Return index in relation table for key pair, given a.index < b.index
static unsigned relationIndex(const KeyDatabase::Key & a, const KeyDatabase::Key & b, KeyDatabase::size_type N)
{
    // Relations are stored in pyramidal array
    return a.index * (2 * N - 1 - a.index) / 2 + b.index - a.index - 1;
}

/****************************************************************************/


KEYLEDSD_EXPORT KeyDatabase::KeyDatabase(key_list keys)
 : m_keys(std::move(keys)),
   m_bounds(computeBounds(m_keys)),
   m_relations(computeRelations(m_keys))
{
#ifndef NDEBUG
    for (auto it = m_keys.begin(); it != m_keys.end(); ++it) {
        assert(it->index == std::distance(m_keys.begin(), it));
    }
#endif
}

KEYLEDSD_EXPORT KeyDatabase::~KeyDatabase() = default;

KEYLEDSD_EXPORT KeyDatabase::const_iterator KeyDatabase::findKeyCode(int keyCode) const
{
    return std::find_if(m_keys.cbegin(), m_keys.cend(),
                        [&](const auto & key) { return key.keyCode == keyCode; });
}

KEYLEDSD_EXPORT KEYLEDSD_EXPORT KeyDatabase::const_iterator KeyDatabase::findName(const char * name) const
{
    return std::find_if(m_keys.cbegin(), m_keys.cend(),
                        [&](const auto & key) { return key.name == name; });
}

KEYLEDSD_EXPORT KeyDatabase::position_type
KeyDatabase::distance(const Key & a, const Key & b) const noexcept
{
    if (a.index == b.index) { return 0; }
    return m_relations[a.index < b.index ? relationIndex(a, b, size())
                                         : relationIndex(b, a, size())].distance;
}

KEYLEDSD_EXPORT double KeyDatabase::angle(const Key & a, const Key & b) const noexcept
{
    if (a.index == b.index) { return 0.0; }
    auto xa = double(a.position.x1 + a.position.x0) / 2.0;
    auto ya = double(a.position.y1 + a.position.y0) / 2.0;
    auto xb = double(b.position.x1 + b.position.x0) / 2.0;
    auto yb = double(b.position.y1 + b.position.y0) / 2.0;
    return std::atan2(ya - yb, xb - xa);    // note: y axis is inverted
}

KeyDatabase::Rect KeyDatabase::computeBounds(const key_list & keys)
{
    auto result = Rect{
        keys.front().position.x0,
        keys.front().position.y0,
        keys.front().position.x1,
        keys.front().position.y1
    };
    for (const auto & key : keys) {
        if (key.position.x0 < result.x0) { result.x0 = key.position.x0; }
        if (key.position.y0 < result.y0) { result.y0 = key.position.y0; }
        if (key.position.x1 > result.x1) { result.x1 = key.position.x1; }
        if (key.position.y1 > result.y1) { result.y1 = key.position.y1; }
    }
    return result;
}

KeyDatabase::relation_list KeyDatabase::computeRelations(const key_list & keys)
{
    KeyDatabase::relation_list result;
    result.reserve(keys.size() * (keys.size() - 1) / 2);

    for (auto it = keys.begin(); it != keys.end(); ++it) {
        for (auto other = std::next(it); other != keys.end(); ++other) {
            auto xa = (it->position.x1 + it->position.x0) / 2;
            auto ya = (it->position.y1 + it->position.y0) / 2;
            auto xb = (other->position.x1 + other->position.x0) / 2;
            auto yb = (other->position.y1 + other->position.y0) / 2;
            auto dx = abs_difference(xa, xb);
            auto dy = abs_difference(ya, yb);

            result.push_back(Relation{
                position_type(std::sqrt(dx * dx + dy * dy))
            });
        }
    }
    return result;
}

/****************************************************************************/

KEYLEDSD_EXPORT KeyDatabase::KeyGroup::KeyGroup(std::string name, key_list keys)
 : m_name(std::move(name)), m_keys(std::move(keys))
{}

KEYLEDSD_EXPORT KeyDatabase::KeyGroup::~KeyGroup() = default;

KEYLEDSD_EXPORT std::ostream &
keyleds::operator<<(std::ostream & out, const KeyDatabase::Key & key)
{
    out <<"Key(" <<key.index <<", " <<key.keyCode <<", " <<key.name <<')';
    return out;
}

KEYLEDSD_EXPORT std::ostream &
keyleds::operator<<(std::ostream & out, const KeyDatabase::KeyGroup & group)
{
    if (group.empty()) {
        out <<"KeyGroup{}";
    } else {
        out <<"KeyGroup{" <<group[0].name;
        std::for_each(group.begin() + 1, group.end(), [&](auto & key) { out <<", " <<key.name; });
        out <<'}';
    }
    return out;
}

KEYLEDSD_EXPORT bool
keyleds::operator==(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
