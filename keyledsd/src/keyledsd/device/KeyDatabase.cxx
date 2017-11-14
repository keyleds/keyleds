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
#include "keyledsd/device/KeyDatabase.h"

#include <algorithm>
#include "keyledsd/device/Device.h"
#include "keyledsd/device/LayoutDescription.h"

using keyleds::device::KeyDatabase;

static constexpr double pi = 3.14159265358979323846;

/****************************************************************************/

// Return index in relation table for key pair, given a.index < b.index
static unsigned relationIndex(const KeyDatabase::Key & a, const KeyDatabase::Key & b, unsigned N)
{
    // Relations are stored in pyramidal array
    return a.index * (2 * N - 1 - a.index) / 2 + b.index - a.index - 1;
}

/****************************************************************************/


KeyDatabase::KeyDatabase(key_list keys)
 : m_keys(std::move(keys)),
   m_bounds(computeBounds(m_keys)),
   m_relations(computeRelations(m_keys))
{}

KeyDatabase::~KeyDatabase() {}

KeyDatabase KeyDatabase::build(const Device & device, const LayoutDescription & layout)
{
    key_list db;
    RenderTarget::size_type keyIndex = 0;
    for (const auto & block : device.blocks()) {
        for (unsigned kidx = 0; kidx < block.keys().size(); ++kidx) {
            const auto keyId = block.keys()[kidx];
            std::string name;
            auto position = Key::Rect{0, 0, 0, 0};

            for (const auto & key : layout.keys()) {
                if (key.block == block.id() && key.code == keyId) {
                    name = key.name;
                    position = {
                        position_type(key.position.x0),
                        position_type(key.position.y0),
                        position_type(key.position.x1),
                        position_type(key.position.y1)
                    };
                    break;
                }
            }
            if (name.empty()) { name = device.resolveKey(block.id(), keyId); }

            db.emplace_back(
                keyIndex,
                device.decodeKeyId(block.id(), keyId),
                std::move(name),
                position
            );
            ++keyIndex;
        }
    }
    return db;
}

KeyDatabase::const_iterator KeyDatabase::findKeyCode(int keyCode) const
{
    return std::find_if(m_keys.cbegin(), m_keys.cend(),
                        [keyCode](const auto & key) { return key.keyCode == keyCode; });
}

KeyDatabase::const_iterator KeyDatabase::findName(const std::string & name) const
{
    return std::find_if(m_keys.cbegin(), m_keys.cend(),
                        [name](const auto & key) { return key.name == name; });
}

KeyDatabase::position_type KeyDatabase::distance(const Key & a, const Key & b) const
{
    if (a.index == b.index) { return 0; }
    return m_relations[a.index < b.index ? relationIndex(a, b, m_keys.size())
                                         : relationIndex(b, a, m_keys.size())].distance;
}

double KeyDatabase::angle(const Key & a, const Key & b) const
{
    if (a.index == b.index) { return 0.0; }
    auto xa = (a.position.x1 + a.position.x0) / 2;
    auto ya = (a.position.y1 + a.position.y0) / 2;
    auto xb = (b.position.x1 + b.position.x0) / 2;
    auto yb = (b.position.y1 + b.position.y0) / 2;
    return std::atan2(ya - yb, xb - xa);    // note: y axis is inverted
}

KeyDatabase::Key::Rect KeyDatabase::computeBounds(const key_list & keys)
{
    auto result = Key::Rect{
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
            auto dx = xb - xa;
            auto dy = yb - ya;

            result.push_back(Relation{
                position_type(std::sqrt(dx * dx + dy * dy))
            });
        }
    }
    return result;
}

/****************************************************************************/

KeyDatabase::Key::Key(index_type index, int keyCode, std::string name, Rect position)
 : index(index),
   keyCode(keyCode),
   name(std::move(name)),
   position(position)
{}

KeyDatabase::Key::~Key() {}

/****************************************************************************/

KeyDatabase::KeyGroup::KeyGroup(std::string name, key_list keys)
 : m_name(std::move(name)), m_keys(std::move(keys))
{}

KeyDatabase::KeyGroup::~KeyGroup() {}

void KeyDatabase::KeyGroup::swap(KeyGroup & other) noexcept
{
    using std::swap;
    swap(m_name, other.m_name);
    swap(m_keys, other.m_keys);
}

bool operator==(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b)
{
    return a.size() == b.size() && std::equal(a.begin(), a.end(), b.begin());
}
