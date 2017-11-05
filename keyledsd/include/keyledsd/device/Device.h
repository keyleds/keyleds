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
#ifndef KEYLEDSD_KEYBOARD_H_F57B19AC
#define KEYLEDSD_KEYBOARD_H_F57B19AC

#include <QObject>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "keyledsd/colors.h"
#include "tools/DeviceWatcher.h"

struct keyleds_device;
struct keyleds_key_color;

namespace std {
    template <> struct default_delete<struct keyleds_device> {
        void operator()(struct keyleds_device *) const;
    };
};

namespace keyleds { namespace device {

/****************************************************************************/

/** Physical device interface
 *
 * Handles communication with the underlying device. This class is built as a
 * wrapper around libkeyleds, with additional checks and caching. It also
 * converts library errors into exceptions.
 */
class Device final
{
public:
    // Transient types
    enum class Type { Keyboard, Remote, NumPad, Mouse, TouchPad, TrackBall, Presenter, Receiver };
    using ColorDirective = struct keyleds_key_color;
    using color_directive_list = std::vector<ColorDirective>;

    // Data
    class KeyBlock;

    // Exceptions
    class error : public std::runtime_error
    {
        using keyleds_error_t = unsigned int;
    public:
                        error(std::string what, keyleds_error_t code, int oserror=0);
        keyleds_error_t code() const { return m_code; }
        int             oserror() const { return m_oserror; }
    private:
        keyleds_error_t m_code;
        int             m_oserror;
    };

    using key_block_id_type = uint8_t;
    using key_id_type = uint8_t;
private:
    using block_list = std::vector<KeyBlock>;
    using key_list = std::vector<key_id_type>;

public:
                        Device(std::string path);
                        Device(const Device &) = delete;
                        Device(Device &&) = default;
                        ~Device();
    Device &            operator=(const Device &) = delete;
    Device &            operator=(Device &&) = default;

    const std::string & path() const noexcept { return m_path; }

    // Query
    Type                type() const { return m_type; }
    const std::string & name() const { return m_name; }
    const std::string & model() const { return m_model; }
    const std::string & serial() const { return m_serial; }
    const std::string & firmware() const { return m_firmware; }
          bool          hasLayout() const;
          int           layout() const { return m_layout; }
    const block_list &  blocks() const { return m_blocks; }

    std::string         resolveKey(key_block_id_type, key_id_type) const;
    int                 decodeKeyId(key_block_id_type, key_id_type) const;

    // Manipulate
    void                setTimeout(unsigned us);
    void                flush();
    bool                resync() noexcept;
    void                fillColor(const KeyBlock & block, const RGBColor);
    void                setColors(const KeyBlock & block, const color_directive_list &);
    void                setColors(const KeyBlock & block, const ColorDirective[], size_t size);
    color_directive_list getColors(const KeyBlock & block);
    void                commitColors();

    // Quirks
    void                patchMissingKeys(const KeyBlock &, const key_list &);

private:
    static std::unique_ptr<struct keyleds_device> openDevice(const std::string &);
    static Type         getType(struct keyleds_device *);
    static std::string  getName(struct keyleds_device *);
    static block_list   getBlocks(struct keyleds_device *);
    void                cacheVersion();

private:
    const std::string   m_path;             ///< Device node path
    std::unique_ptr<struct keyleds_device> m_device;    ///< Underlying libkeyleds opaque handle
    Type                m_type;             ///< The kind of libkeyleds device
    std::string         m_name;             ///< User-friendly name of the device, eg "Logitech G410"
    std::string         m_model;            ///< Model identification string, eg "c3300000"
    std::string         m_serial;           ///< Device-declared serial - note: usually not unique
    std::string         m_firmware;         ///< Detected firmware version
    int                 m_layout;           ///< Device-declared layout number - used to locate a layout file
    block_list          m_blocks;           ///< List of key blocks detected on device
};


/** Physical device key block description
 *
 * Holds the detected characteristics of a physical key block.
 */
class Device::KeyBlock final
{
public:
                        KeyBlock(key_block_id_type id, key_list keys, RGBColor maxValues);
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

/** Keyleds-specific device watcher
 *
 * A device::FilteredDeviceWatcher that further filters detected devices to only
 * let Logitech devices through.
 */
class DeviceWatcher : public ::device::FilteredDeviceWatcher
{
    Q_OBJECT
public:
            DeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
    bool    isVisible(const ::device::Description & dev) const override;
};

/****************************************************************************/

} } // namespace keyleds::device

#endif
