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
#include "keyledsd/device/Logitech.h"

#include "config.h"
#include "keyleds.h"
#include "keyledsd/logging.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>

LOGGING("device");

using keyleds::device::Logitech;
using keyleds::device::LogitechWatcher;

void Logitech::keyleds_device_deleter::operator()(struct keyleds_device *p) const {
    keyleds_close(p);
}

template <typename T> struct keyleds_deleter {};
template <> struct keyleds_deleter<keyleds_keyblocks_info> {
    void operator()(struct keyleds_keyblocks_info *p) const { keyleds_free_block_info(p); }
};
template <> struct keyleds_deleter<keyleds_device_version> {
    void operator()(struct keyleds_device_version *p) const { keyleds_free_device_version(p); }
};
struct keyleds_device_name_deleter {
    void operator()(char *p) const { keyleds_free_device_name(p); }
};

template <typename T, typename deleter = keyleds_deleter<T>>
using keyleds_ptr = std::unique_ptr<T, deleter>;


static constexpr char InterfaceProtocolAttr[] = "bInterfaceProtocol";
static constexpr unsigned ApplicationInterfaceProtocol = 0;
static constexpr char DeviceVendorAttr[] = "idVendor";

/****************************************************************************/

Logitech::Logitech(device_ptr device,
                   std::string path, Type type, std::string name, std::string model,
                   std::string serial, std::string firmware, int layout, block_list blocks)
 : Device(std::move(path), type, std::move(name), std::move(model), std::move(serial),
          std::move(firmware), layout, std::move(blocks)),
   m_device(std::move(device))
{}

Logitech::~Logitech() = default;

std::unique_ptr<keyleds::device::Device> Logitech::open(const std::string & path)
{
    auto device = device_ptr(keyleds_open(path.c_str(), KEYLEDSD_APP_ID));
    if (device == nullptr) { throw error(keyleds_get_error_str(), keyleds_get_errno()); }

    auto type = getType(device.get());
    auto name = getName(device.get());
    std::string model, serial, firmware;
    parseVersion(device.get(), &model, &serial, &firmware);
    auto layout = keyleds_keyboard_layout(device.get(), KEYLEDS_TARGET_DEFAULT);
    auto blocks = getBlocks(device.get());

    return std::unique_ptr<Logitech>(new Logitech(
        std::move(device), path,
        type, std::move(name),
        std::move(model), std::move(serial), std::move(firmware),
        layout, std::move(blocks)
    ));
}

/****************************************************************************/
/****************************************************************************/

bool Logitech::hasLayout() const
{
    return layout() != KEYLEDS_KEYBOARD_LAYOUT_INVALID;
}

std::string Logitech::resolveKey(key_block_id_type blockId, key_id_type keyId) const
{
    auto keyCode = keyleds_translate_scancode(keyleds_block_id_t(blockId), keyId);
    const char * name = keyleds_lookup_string(keyleds_keycode_names, keyCode);
    if (name == nullptr) { return {}; }
    return name;
}

int Logitech::decodeKeyId(key_block_id_type blockId, key_id_type keyId) const
{
    return static_cast<int>(keyleds_translate_scancode(keyleds_block_id_t(blockId), keyId));
}

/****************************************************************************/

void Logitech::setTimeout(unsigned us)
{
    keyleds_set_timeout(m_device.get(), us);
}

void Logitech::flush()
{
    if (!keyleds_flush_fd(m_device.get())) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

bool Logitech::resync() noexcept
{
    // Note this method does not throw in case of failure. As it is used in error
    // recovery, it is a normal outcome for it to be enable to resync device
    // communications.
    return keyleds_flush_fd(m_device.get()) &&
           keyleds_ping(m_device.get(), KEYLEDS_TARGET_DEFAULT);
}

void Logitech::fillColor(const KeyBlock & block, const RGBColor color)
{
    if (!keyleds_set_led_block(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                               color.red, color.green, color.blue)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Logitech::setColors(const KeyBlock & block, const ColorDirective colors[], size_type size)
{
    assert(size > 0);
    struct keyleds_key_color buffer[size];
    std::transform(colors, colors + size, buffer,
                   [](const auto & color) -> struct keyleds_key_color
                   { return { color.id, color.red, color.green, color.blue }; });

    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                          buffer, size)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Logitech::getColors(const KeyBlock & block, ColorDirective colors[])
{
    if (block.keys().empty()) { return; }

    struct keyleds_key_color buffer[block.keys().size()];

    if (!keyleds_get_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, keyleds_block_id_t(block.id()),
                          buffer, 0, static_cast<unsigned>(block.keys().size()))) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    std::transform(buffer, buffer + block.keys().size(), colors,
                   [](const auto & color) -> ColorDirective
                   { return { color.id, color.red, color.green, color.blue }; });
}

void Logitech::commitColors()
{
    if (!keyleds_commit_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

/****************************************************************************/
/****************************************************************************/


Logitech::Type Logitech::getType(struct keyleds_device * device)
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

std::string Logitech::getName(struct keyleds_device * device)
{
    char * name;
    if (!keyleds_get_device_name(device, KEYLEDS_TARGET_DEFAULT, &name)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    return std::string(name);
}

Logitech::block_list Logitech::getBlocks(struct keyleds_device * device)
{
    struct keyleds_keyblocks_info * info;
    if (!keyleds_get_block_info(device, KEYLEDS_TARGET_DEFAULT, &info)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    // Wrap retrieved data in a smart pointer so it is freed if something throws
    auto blockinfo_p = keyleds_ptr<keyleds_keyblocks_info>(info);

    block_list blocks;
    for (unsigned i = 0; i < info->length; i += 1) {
        const auto & block = info->blocks[i];
        key_list key_ids;

        // We still create blocks with no keys so they can be patched from layout file
        // But we don't attempt to retrieve a list
        if (block.nb_keys > 0) {
            struct keyleds_key_color keys[block.nb_keys];
            if (!keyleds_get_leds(device, KEYLEDS_TARGET_DEFAULT, block.block_id,
                                keys, 0, block.nb_keys)) {
                throw error(keyleds_get_error_str(), keyleds_get_errno());
            }

            key_ids.reserve(block.nb_keys);
            for (unsigned key_idx = 0; key_idx < block.nb_keys; key_idx += 1) {
                if (keys[key_idx].id != 0) { key_ids.push_back(keys[key_idx].id); }
            }
        }

        assert(block.block_id >= 0);
        blocks.emplace_back(
            block.block_id,
            keyleds_lookup_string(keyleds_block_id_names, static_cast<unsigned>(block.block_id)),
            std::move(key_ids),
            RGBColor{block.red, block.green, block.blue}
        );
    }
    return blocks;
}

void Logitech::parseVersion(struct keyleds_device * device, std::string * model,
                            std::string * serial, std::string * firmware)
{
    struct keyleds_device_version * version;
    if (!keyleds_get_device_version(device, KEYLEDS_TARGET_DEFAULT, &version)) {
        throw error(keyleds_get_error_str(), keyleds_get_errno());
    }
    // Wrap retrieved data in a smart pointer so it is freed if something throws
    auto version_p = keyleds_ptr<keyleds_device_version>(version);

    // Build hex representation of HID++ device model
    {
        std::ostringstream buffer;
        buffer <<std::hex <<std::setfill('0')
               <<std::setw(2) <<+version->model[0] <<std::setw(2) <<+version->model[1]
               <<std::setw(2) <<+version->model[2] <<std::setw(2) <<+version->model[3]
               <<std::setw(2) <<+version->model[4] <<std::setw(2) <<+version->model[5];
        *model = buffer.str();
    }

    // Build hex representation of device's serial number
    {
        std::ostringstream buffer;
        buffer <<std::hex <<std::setfill('0')
               <<std::setw(2) <<+version->serial[0] <<std::setw(2) <<+version->serial[1]
               <<std::setw(2) <<+version->serial[2] <<std::setw(2) <<+version->serial[3];
        *serial = buffer.str();
    }

    // Find active firmware and format its version string
    for (unsigned i = 0; i < version->length; i += 1) {
        const auto & protocol = version->protocols[i];
        if (protocol.is_active) {
            std::ostringstream buffer;
            auto prefix = std::string(protocol.prefix);
            auto endpos = prefix.find_last_not_of(' ');
            if (endpos != std::string::npos) { prefix.erase(endpos + 1, std::string::npos); }
            buffer <<prefix
                     <<'v' <<protocol.version_major
                     <<'.' <<protocol.version_minor
                     <<'.' <<std::hex <<protocol.build;
            *firmware = buffer.str();
            break;
        }
    }
}

/****************************************************************************/
/****************************************************************************/

Logitech::error::error(const std::string & what, keyleds_error_t code, int oserror)
 : Device::error(what), m_code(code), m_oserror(oserror)
{
    if (m_code == KEYLEDS_ERROR_ERRNO && m_oserror == 0) { m_oserror = errno; }
}

bool Logitech::error::expected() const
{
    return (m_code == KEYLEDS_ERROR_ERRNO && m_oserror == ENODEV)
        || m_code == KEYLEDS_ERROR_TIMEDOUT
        || m_code == KEYLEDS_ERROR_HIDNOPP
        || m_code == KEYLEDS_ERROR_HIDVERSION;
}

bool Logitech::error::recoverable() const
{
    if (m_code == KEYLEDS_ERROR_ERRNO) {
        switch (m_oserror) {
            case EIO:
            case EINTR:
                return true;
            default:
                return false;
        }
    }
    return true;
}

/****************************************************************************/
/****************************************************************************/

LogitechWatcher::LogitechWatcher(uv_loop_t & loop, bool active, struct udev * udev)
    : FilteredDeviceWatcher(loop, active, udev)
{
    setSubsystem("hidraw");
}

bool LogitechWatcher::isVisible(const tools::device::Description & dev) const
{
    return checkInterface(dev) && checkDevice(dev);
}

bool LogitechWatcher::checkInterface(const tools::device::Description & dev) const
{
    auto iface = dev.parentWithType("usb", "usb_interface");
    if (!iface) {
        DEBUG("Cannot check ", dev.sysPath(), ": no usb interface");
        return false;
    }

    auto protocol = getAttribute(*iface, InterfaceProtocolAttr);
    if (!protocol) {
        ERROR("Device ", iface->sysPath(), " has not interface protocol attribute");
        return false;
    }
    if (std::stoul(*protocol, nullptr, 16) != ApplicationInterfaceProtocol) { return false; }

    return true;
}

bool LogitechWatcher::checkDevice(const tools::device::Description & dev) const
{
    auto usbdev = dev.parentWithType("usb", "usb_device");
    if (!usbdev) {
        DEBUG("Cannot check ", dev.sysPath(), ": no usb device");
        return false;
    }

    auto vendor = getAttribute(*usbdev, DeviceVendorAttr);
    if (!vendor) {
        ERROR("Device ", usbdev->sysPath(), " has not vendor id attribute");
        return false;
    }
    if (std::stoul(*vendor, nullptr, 16) != LOGITECH_VENDOR_ID) { return false; }

    return true;
}
