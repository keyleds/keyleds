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
#include <algorithm>
#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include "keyleds.h"
#include "keyledsd/Device.h"
#include "logging.h"
#include "config.h"

LOGGING("device");

using keyleds::Device;
using keyleds::DeviceWatcher;

namespace std {
    void default_delete<struct keyleds_device>::operator()(struct keyleds_device *p) const {
        keyleds_close(p);
    }
    template<> struct default_delete<struct keyleds_keyblocks_info> {
        void operator()(struct keyleds_keyblocks_info *p) const { keyleds_free_block_info(p); }
    };
    template<> struct default_delete<struct keyleds_device_version> {
        void operator()(struct keyleds_device_version *p) const { keyleds_free_device_version(p); }
    };
}

/****************************************************************************/

Device::Device(std::string path)
    : m_path(std::move(path)),
      m_device(openDevice(m_path)),
      m_type(getType(m_device.get())),
      m_name(getName(m_device.get())),
      m_layout(keyleds_keyboard_layout(m_device.get(), KEYLEDS_TARGET_DEFAULT)),
      m_blocks(getBlocks(m_device.get()))
{
    cacheVersion();
}

std::unique_ptr<struct keyleds_device> Device::openDevice(const std::string & path)
{
    auto device = std::unique_ptr<struct keyleds_device>(
        keyleds_open(path.c_str(), KEYLEDSD_APP_ID)
    );
    if (device == nullptr) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    return device;
}

Device::Type Device::getType(struct keyleds_device * device)
{
    keyleds_device_type_t type;
    if (!keyleds_get_device_type(device, KEYLEDS_TARGET_DEFAULT, &type)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    switch(type) {
    case KEYLEDS_DEVICE_TYPE_KEYBOARD:  return Type::Keyboard;
    case KEYLEDS_DEVICE_TYPE_REMOTE:    return Type::Remote;
    case KEYLEDS_DEVICE_TYPE_NUMPAD:    return Type::NumPad;
    case KEYLEDS_DEVICE_TYPE_MOUSE:     return Type::Mouse;
    case KEYLEDS_DEVICE_TYPE_TOUCHPAD:  return Type::TouchPad;
    case KEYLEDS_DEVICE_TYPE_TRACKBALL: return Type::TrackBall;
    case KEYLEDS_DEVICE_TYPE_PRESENTER: return Type::Presenter;
    case KEYLEDS_DEVICE_TYPE_RECEIVER:  return Type::Receiver;
    }
    throw std::logic_error("Invalid device type");
}

std::string Device::getName(struct keyleds_device * device)
{
    char * name;
    if (!keyleds_get_device_name(device, KEYLEDS_TARGET_DEFAULT, &name)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    // Wrap the pointer in a smart pointer in case string creation throws
    auto name_p = std::unique_ptr<char[], void(*)(char*)>(name, keyleds_free_device_name);
    return std::string(name);
}

void Device::cacheVersion()
{
    struct keyleds_device_version * version;
    if (!keyleds_get_device_version(m_device.get(), KEYLEDS_TARGET_DEFAULT, &version)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    // Wrap retrieved data in a smart pointer so it is freed if something throws
    auto version_p = std::unique_ptr<struct keyleds_device_version>(version);

    // Build hex representation of HID++ device model
    std::ostringstream model;
    model <<std::hex <<std::setfill('0')
          <<std::setw(2) <<+version->model[0] <<std::setw(2) <<+version->model[1]
          <<std::setw(2) <<+version->model[2] <<std::setw(2) <<+version->model[3]
          <<std::setw(2) <<+version->model[4] <<std::setw(2) <<+version->model[5];
    m_model = model.str();

    // Build hex representation of device's serial number
    std::ostringstream serial;
    serial <<std::hex <<std::setfill('0')
           <<std::setw(2) <<+version->serial[0] <<std::setw(2) <<+version->serial[1]
           <<std::setw(2) <<+version->serial[2] <<std::setw(2) <<+version->serial[3];
    m_serial = serial.str();

    // Find active firmware and format its version string
    for (unsigned i = 0; i < version->length; i += 1) {
        const auto & protocol = version->protocols[i];
        if (protocol.is_active) {
            std::ostringstream firmware;
            auto prefix = std::string(protocol.prefix);
            auto endpos = prefix.find_last_not_of(" ");
            if (endpos != std::string::npos) { prefix.erase(endpos + 1, std::string::npos); }
            firmware <<prefix
                     <<'v' <<protocol.version_major
                     <<'.' <<protocol.version_minor
                     <<'.' <<std::hex <<protocol.build;
            m_firmware = firmware.str();
            break;
        }
    }
}

Device::block_list Device::getBlocks(struct keyleds_device * device)
{
    struct keyleds_keyblocks_info * info;
    if (!keyleds_get_block_info(device, KEYLEDS_TARGET_DEFAULT, &info)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    // Wrap retrieved data in a smart pointer so it is freed if something throws
    auto blockinfo_p = std::unique_ptr<struct keyleds_keyblocks_info>(info);

    block_list blocks;
    for (unsigned i = 0; i < info->length; i += 1) {
        const auto & block = info->blocks[i];

        struct keyleds_key_color keys[block.nb_keys];
        if (!keyleds_get_leds(device, KEYLEDS_TARGET_DEFAULT, block.block_id,
                              keys, 0, block.nb_keys)) {
            throw error(keyleds_get_error_str(), keyleds_get_errno());
        }

        key_list key_ids;
        key_ids.reserve(block.nb_keys);
        for (unsigned key_idx = 0; key_idx < block.nb_keys; key_idx += 1) {
            if (keys[key_idx].id != 0) { key_ids.push_back(keys[key_idx].id); }
        }

        blocks.emplace_back(
            block.block_id,
            std::move(key_ids),
            RGBColor{block.red, block.green, block.blue}
        );
    }
    return blocks;
}

/****************************************************************************/

bool Device::hasLayout() const
{
    return m_layout != KEYLEDS_KEYBOARD_LAYOUT_INVALID;
}

std::string Device::resolveKey(key_block_id_type blockId, key_id_type keyId) const
{
    if (blockId != KEYLEDS_BLOCK_KEYS) {
        return {};  // only keys from the regular key block have a built-in symbol
    }
    auto keyCode = keyleds_translate_scancode(keyId);
    const char * name = keyleds_lookup_string(keyleds_keycode_names, keyCode);
    if (name == nullptr) { return {}; }
    return name;
}

int Device::decodeKeyId(key_block_id_type blockId, key_id_type keyId) const
{
    if (blockId != KEYLEDS_BLOCK_KEYS) {
        return 0;  // only keys from the regular key block have a built-in key code
    }
    return keyleds_translate_scancode(keyId);
}

/****************************************************************************/

void Device::setTimeout(unsigned us)
{
    keyleds_set_timeout(m_device.get(), us);
}

void Device::flush()
{
    if (!keyleds_flush_fd(m_device.get())) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

bool Device::resync() noexcept
{
    // Note this method does not throw in case of failure. As it is used in error
    // recovery, it is a normal outcome for it to be enable to resync device
    // communications.
    return keyleds_flush_fd(m_device.get()) &&
           keyleds_ping(m_device.get(), KEYLEDS_TARGET_DEFAULT);
}

void Device::fillColor(const KeyBlock & block, const RGBColor color)
{
    if (!keyleds_set_led_block(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                               color.red, color.green, color.blue)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Device::setColors(const KeyBlock & block, const color_directive_list & colors)
{
    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                          colors.data(), colors.size())) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Device::setColors(const KeyBlock & block, const ColorDirective colors[], size_t size)
{
    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                          colors, size)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

Device::color_directive_list Device::getColors(const KeyBlock & block)
{
    color_directive_list result(block.keys().size());
    if (!keyleds_get_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                          result.data(), 0, result.size())) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    return result;
}

void Device::commitColors()
{
    if (!keyleds_commit_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

/****************************************************************************/

Device::KeyBlock::KeyBlock(key_block_id_type id, key_list keys, RGBColor maxValues)
    : m_id(id),
      m_name(keyleds_lookup_string(keyleds_block_id_names, id)),
      m_keys(std::move(keys)),
      m_maxValues(maxValues)
{}

/****************************************************************************/

Device::error::error(std::string what, keyleds_error_t code, int oserror)
 : std::runtime_error(what), m_code(code), m_oserror(oserror)
{
    if (m_code == KEYLEDS_ERROR_ERRNO && m_oserror == 0) { m_oserror = errno; }
}

/****************************************************************************/

DeviceWatcher::DeviceWatcher(struct udev * udev, QObject *parent)
    : FilteredDeviceWatcher(udev, parent)
{
    setSubsystem("hidraw");
}

bool DeviceWatcher::isVisible(const device::Description & dev) const
{
    // Filter interface protocol
    const auto & iface = dev.parentWithType("usb", "usb_interface");
    auto it = std::find_if(
        iface.attributes().begin(), iface.attributes().end(),
        [](const auto & attr) { return attr.first == "bInterfaceProtocol"; }
    );
    if (it == iface.attributes().end()) {
        ERROR("Device ", iface.sysPath(), " has not interface protocol attribute");
        return false;
    }
    if (std::stoul(it->second, nullptr, 16) != 0) { return false; }

    // Filter device id
    const auto & usbdev = dev.parentWithType("usb", "usb_device");
    it = std::find_if(
        usbdev.attributes().begin(), usbdev.attributes().end(),
        [](const auto & attr) { return attr.first == "idVendor"; }
    );
    if (it == usbdev.attributes().end()) {
        ERROR("Device ", usbdev.sysPath(), " has not vendor id attribute");
        return false;
    }
    if (std::stoul(it->second, nullptr, 16) != LOGITECH_VENDOR_ID) { return false; }

    return true;
}
