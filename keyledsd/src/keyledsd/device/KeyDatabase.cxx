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
#include <iomanip>
#include <sstream>
#include "keyledsd/device/Device.h"
#include "keyledsd/device/LayoutDescription.h"

using keyleds::device::KeyDatabase;

/****************************************************************************/

static std::string layoutName(const keyleds::device::Device & device)
{
    std::ostringstream fileNameBuf;
    fileNameBuf.fill('0');
    fileNameBuf <<device.model() <<'_' <<std::hex <<std::setw(4) <<device.layout() <<".xml";
    return fileNameBuf.str();
}

/****************************************************************************/

KeyDatabase::KeyDatabase(key_list keys)
 : m_keys(std::move(keys)),
   m_bounds(computeBounds(m_keys))
{}

KeyDatabase::~KeyDatabase() {}

KeyDatabase KeyDatabase::build(const Device & device)
{
    LayoutDescription layout;
    if (device.hasLayout()) {
        layout = LayoutDescription::loadFile(layoutName(device));
    }

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
                    position = {key.position.x0, key.position.y0,
                                key.position.x1, key.position.y1};
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

KeyDatabase::const_iterator KeyDatabase::findIndex(RenderTarget::size_type index) const
{
    return std::find_if(m_keys.cbegin(), m_keys.cend(),
                        [index](const auto & key) { return key.index == index; });
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
