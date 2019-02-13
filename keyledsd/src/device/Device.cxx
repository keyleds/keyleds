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
#include "keyledsd/device/Device.h"
#include <algorithm>

using keyleds::device::Device;

/****************************************************************************/

Device::Device(std::string path, Type type, std::string name, std::string model,
               std::string serial, std::string firmware, int layout, block_list blocks)
 : m_path(std::move(path)),
   m_type(type),
   m_name(std::move(name)),
   m_model(std::move(model)),
   m_serial(std::move(serial)),
   m_firmware(std::move(firmware)),
   m_layout(layout),
   m_blocks(std::move(blocks))
{}

Device::~Device() = default;

/** Patch key declaration with missing keys.
 * @param block Which block the keys are in.
 * @param keyIds List of key identifiers to add. They must not already be known.
 * @warning Accesses `block` in a non-thread-safe way.
 */
void Device::patchMissingKeys(const KeyBlock & block, const key_list & keyIds)
{
    auto it = std::find_if(m_blocks.begin(), m_blocks.end(),
                           [&](const auto & item){ return &item == &block; });
    if (it != m_blocks.end()) {
        it->patchMissingKeys(keyIds);
    }
}

/****************************************************************************/

Device::error::error(const std::string & what) : std::runtime_error(what)
{}

/****************************************************************************/

Device::KeyBlock::KeyBlock(key_block_id_type id, std::string name, key_list keys, RGBColor maxValues)
    : m_id(id),
      m_name(std::move(name)),
      m_keys(std::move(keys)),
      m_maxValues(maxValues)
{}

Device::KeyBlock::~KeyBlock() = default;

/** Add key identifiers into the block.
 * @param keyIds List of key identifiers to add. They must not already be known.
 */
void Device::KeyBlock::patchMissingKeys(const key_list & keyIds)
{
    m_keys.reserve(m_keys.size() + keyIds.size());
    std::copy(keyIds.begin(), keyIds.end(), std::back_inserter(m_keys));
}
