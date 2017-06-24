#include <QtCore>
#include <cstdlib>
#include <iomanip>
#include <memory>
#include <sstream>
#include <string>
#include "config.h"
#include "keyleds.h"
#include "keyledsd/Device.h"

using keyleds::Device;
using keyleds::DeviceWatcher;

namespace std {
    template<> struct default_delete<struct keyleds_keyblocks_info> {
        void operator()(struct keyleds_keyblocks_info *p) const { keyleds_free_block_info(p); }
    };
    template<> struct default_delete<struct keyleds_device_version> {
        void operator()(struct keyleds_device_version *p) const { keyleds_free_device_version(p); }
    };
}

/****************************************************************************/

Device::Device(const std::string & path)
    : m_device(openDevice(path)),
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
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
    return device;
}

Device::Type Device::getType(struct keyleds_device * device)
{
    keyleds_device_type_t type;
    if (!keyleds_get_device_type(device, KEYLEDS_TARGET_DEFAULT, &type)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
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
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
    auto name_p = std::unique_ptr<char, void(*)(void*)>(name, std::free);
    return std::string(name);
}

void Device::cacheVersion()
{
    struct keyleds_device_version * version;
    if (!keyleds_get_device_version(m_device.get(), KEYLEDS_TARGET_DEFAULT, &version)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
    auto version_p = std::unique_ptr<struct keyleds_device_version>(version);

    std::ostringstream model;
    model <<std::hex <<std::setfill('0')
          <<std::setw(2) <<+version->model[0] <<std::setw(2) <<+version->model[1]
          <<std::setw(2) <<+version->model[2] <<std::setw(2) <<+version->model[3]
          <<std::setw(2) <<+version->model[4] <<std::setw(2) <<+version->model[5];
    m_model = model.str();

    std::ostringstream serial;
    serial <<std::hex <<std::setfill('0')
           <<std::setw(2) <<+version->serial[0] <<std::setw(2) <<+version->serial[1]
           <<std::setw(2) <<+version->serial[2] <<std::setw(2) <<+version->serial[3];
    m_serial = serial.str();

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
    block_list blocks;

    struct keyleds_keyblocks_info * info;
    if (!keyleds_get_block_info(device, KEYLEDS_TARGET_DEFAULT, &info)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
    auto blockinfo_p = std::unique_ptr<struct keyleds_keyblocks_info>(info);

    for (unsigned i = 0; i < info->length; i += 1) {
        const auto & block = info->blocks[i];

        struct keyleds_key_color keys[block.nb_keys];
        if (!keyleds_get_leds(device, KEYLEDS_TARGET_DEFAULT, block.block_id,
                              keys, 0, block.nb_keys)) {
            throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
        }

        KeyBlock::key_list key_ids;
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

void Device::setTimeout(unsigned us)
{
    keyleds_set_timeout(m_device.get(), us);
}

bool Device::resync()
{
    return keyleds_ping(m_device.get(), KEYLEDS_TARGET_DEFAULT);
}

void Device::fillColor(const KeyBlock & block, RGBColor color)
{
    if (!keyleds_set_led_block(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                               color.red, color.green, color.blue)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Device::setColors(const KeyBlock & block, const color_directive_list & colors)
{
    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                          colors.data(), colors.size())) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

void Device::setColors(const KeyBlock & block, const ColorDirective colors[], size_t size)
{
    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                          colors, size)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

Device::color_directive_list Device::getColors(const KeyBlock & block)
{
    color_directive_list result(block.keys().size());
    if (!keyleds_get_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                          result.data(), 0, result.size())) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
    return result;
}

void Device::commitColors()
{
    if (!keyleds_commit_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT)) {
        throw Device::error(keyleds_get_error_str(), keyleds_get_errno());
    }
}

/****************************************************************************/

Device::KeyBlock::KeyBlock(keyleds_block_id_t id, key_list && keys, RGBColor maxValues)
    : m_id(id),
      m_name(keyleds_lookup_string(keyleds_block_id_names, id)),
      m_keys(keys),
      m_maxValues(maxValues)
{
    m_keysInverse.resize(256);
    for (size_t idx = 0; idx < m_keys.size(); ++idx) {
        m_keysInverse[m_keys[idx]] = idx;
    }
}

/****************************************************************************/

DeviceWatcher::DeviceWatcher(struct udev * udev, QObject *parent)
    : FilteredDeviceWatcher(udev, parent)
{
    setSubsystem("hidraw");
}

bool DeviceWatcher::isVisible(const device::Description & dev) const
{
    const auto & iface = dev.parentWithType("usb", "usb_interface");
    if (std::stoul(iface.attributes().at("bInterfaceProtocol"), nullptr, 16) != 0) {
        return false;
    }

    const auto & usbdev = dev.parentWithType("usb", "usb_device");
    if (std::stoul(usbdev.attributes().at("idVendor"), nullptr, 16) != LOGITECH_VENDOR_ID) {
        return false;
    }

    return true;
}
