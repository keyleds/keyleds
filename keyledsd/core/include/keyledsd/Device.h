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
#ifndef KEYLEDSD_KEYBOARD_H_F6DA7CD5
#define KEYLEDSD_KEYBOARD_H_F6DA7CD5

#include <cstdint>
#include <stdexcept>
#include <string>
#include <vector>
#include "keyledsd/colors.h"

namespace keyleds {

/****************************************************************************/

/** Physical device interface
 *
 * Handles communication with the underlying device. This class is built as a
 * wrapper around libkeyleds, with additional checks and caching. It also
 * converts library errors into exceptions.
 */
class Device
{
public:
    // Transient types
    enum class Type { Keyboard, Remote, NumPad, Mouse, TouchPad, TrackBall, Presenter, Receiver };
    struct ColorDirective {
        uint8_t id, red, green, blue;
    };

    // Data
    class KeyBlock;

    // Exceptions
    class error : public std::runtime_error
    {
    public:
        explicit        error(std::string what);
        virtual bool    expected() const = 0;       ///< Error is a normal failure condition
        virtual bool    recoverable() const = 0;    ///< Error recovery can be attempted with resync()
    };

    using key_block_id_type = uint8_t;
    using key_id_type = uint8_t;

protected:
    using block_list = std::vector<KeyBlock>;
    using key_list = std::vector<key_id_type>;

protected:
                        Device(std::string path, Type type, std::string name, std::string model,
                               std::string serial, std::string firmware, int layout, block_list);
public:
    virtual             ~Device();

    // Query
    const std::string & path() const noexcept { return m_path; }
    Type                type() const { return m_type; }
    const std::string & name() const { return m_name; }
    const std::string & model() const { return m_model; }
    const std::string & serial() const { return m_serial; }
    const std::string & firmware() const { return m_firmware; }
    virtual bool        hasLayout() const = 0;
          int           layout() const { return m_layout; }
    const block_list &  blocks() const { return m_blocks; }

    virtual std::string resolveKey(key_block_id_type, key_id_type) const = 0;
    virtual int         decodeKeyId(key_block_id_type, key_id_type) const = 0;

    // Manipulate
    virtual void        setTimeout(unsigned us) = 0;
    virtual void        flush() = 0;
    virtual bool        resync() noexcept = 0;
    virtual void        fillColor(const KeyBlock & block, const RGBColor) = 0;
    virtual void        setColors(const KeyBlock & block, const ColorDirective[], size_t size) = 0;
    virtual void        getColors(const KeyBlock & block, ColorDirective[]) = 0;
    virtual void        commitColors() = 0;

    // Quirks
    void                patchMissingKeys(const KeyBlock &, const key_list &);

private:
    const std::string   m_path;             ///< Device node path
    Type                m_type;             ///< The kind of libkeyleds device
    std::string         m_name;             ///< User-friendly name of the device, eg "Logitech G410"
    std::string         m_model;            ///< Model identification string, eg "c3300000"
    std::string         m_serial;           ///< Device-declared serial - note: usually not unique
    std::string         m_firmware;         ///< Detected firmware version
    int                 m_layout;           ///< Device-declared layout number - used to locate a layout file
    block_list          m_blocks;           ///< List of key blocks detected on device
};

/****************************************************************************/

/** Physical device key block description
 *
 * Holds the detected characteristics of a physical key block.
 */
class Device::KeyBlock final
{
public:
                        KeyBlock(key_block_id_type id, std::string name, key_list, RGBColor);
                        ~KeyBlock();

    key_block_id_type   id() const { return m_id; }
    const std::string & name() const { return m_name; }
    const key_list &    keys() const { return m_keys; }
    const RGBColor &    maxValues() const { return m_maxValues; }

    // Quirks
    void                patchMissingKeys(const key_list &);

private:
    key_block_id_type   m_id;           ///< Block identifier, eg: 0 for normal keys, 64 for game/light keys, ...
    std::string         m_name;         ///< User-readable block name
    key_list            m_keys;         ///< List of key identifiers in block
    RGBColor            m_maxValues;    ///< Color values that represent maximum light power for the block
};

/****************************************************************************/

} // namespace keyleds

#endif
