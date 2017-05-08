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

typedef std::unique_ptr<struct keyleds_keyblocks_info,
                        void (*)(struct keyleds_keyblocks_info *)> blockinfo_ptr;
typedef std::unique_ptr<struct keyleds_device_version,
                        void (*)(struct keyleds_device_version*)> version_ptr;

/****************************************************************************/

Device::Device(const std::string & path)
    : m_device(nullptr, keyleds_close)
{
    m_device.reset(keyleds_open(path.c_str(), KEYLEDSD_APP_ID));
    if (m_device == nullptr) {
        throw Device::error(keyleds_get_error_str());
    }

    cacheType();
    cacheName();
    cacheVersion();
    cacheLayout();
    loadBlocks();
}

void Device::cacheType()
{
    keyleds_device_type_t type;
    if (!keyleds_get_device_type(m_device.get(), KEYLEDS_TARGET_DEFAULT, &type)) {
        throw Device::error(keyleds_get_error_str());
    }
    switch(type) {
    case KEYLEDS_DEVICE_TYPE_KEYBOARD:  m_type = Type::Keyboard; break;
    case KEYLEDS_DEVICE_TYPE_REMOTE:    m_type = Type::Remote; break;
    case KEYLEDS_DEVICE_TYPE_NUMPAD:    m_type = Type::NumPad; break;
    case KEYLEDS_DEVICE_TYPE_MOUSE:     m_type = Type::Mouse; break;
    case KEYLEDS_DEVICE_TYPE_TOUCHPAD:  m_type = Type::TouchPad; break;
    case KEYLEDS_DEVICE_TYPE_TRACKBALL: m_type = Type::TrackBall; break;
    case KEYLEDS_DEVICE_TYPE_PRESENTER: m_type = Type::Presenter; break;
    case KEYLEDS_DEVICE_TYPE_RECEIVER:  m_type = Type::Receiver; break;
    default:
        throw std::logic_error("Invalid device type");
    }
}

void Device::cacheName()
{
    char * name;
    if (!keyleds_get_device_name(m_device.get(), KEYLEDS_TARGET_DEFAULT, &name)) {
        throw Device::error(keyleds_get_error_str());
    }
    auto name_p = std::unique_ptr<char, void(*)(void*)>(name, std::free);
    m_name = std::string(name);
}

void Device::cacheVersion()
{
    struct keyleds_device_version * version;
    if (!keyleds_get_device_version(m_device.get(), KEYLEDS_TARGET_DEFAULT, &version)) {
        throw Device::error(keyleds_get_error_str());
    }
    auto version_p = version_ptr(version, keyleds_free_device_version);

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

void Device::cacheLayout()
{
    m_layout = keyleds_keyboard_layout(m_device.get(), KEYLEDS_TARGET_DEFAULT);
}

void Device::loadBlocks()
{
    m_blocks.clear();

    struct keyleds_keyblocks_info * info;
    if (!keyleds_get_block_info(m_device.get(), KEYLEDS_TARGET_DEFAULT, &info)) {
        throw Device::error(keyleds_get_error_str());
    }
    auto blockinfo_p = blockinfo_ptr(info, keyleds_free_block_info);

    for (unsigned i = 0; i < info->length; i += 1) {
        const auto & block = info->blocks[i];

        struct keyleds_key_color keys[block.nb_keys];
        if (!keyleds_get_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.block_id,
                              keys, 0, block.nb_keys)) {
            throw Device::error(keyleds_get_error_str());
        }

        KeyBlock::key_list key_ids;
        key_ids.reserve(block.nb_keys);
        for (unsigned key_idx = 0; key_idx < block.nb_keys; key_idx += 1) {
            if (keys[key_idx].id != 0) { key_ids.push_back(keys[key_idx].id); }
        }

        m_blocks.emplace_back(
            block.block_id,
            std::move(key_ids),
            Color{block.red, block.green, block.blue}
        );
    }
}

/****************************************************************************/

void Device::fillColor(const KeyBlock & block, Color color)
{
    if (!keyleds_set_led_block(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                               color.red, color.green, color.blue)) {
        throw Device::error(keyleds_get_error_str());
    }
}

void Device::setColors(const KeyBlock & block, const color_directive_list & colors)
{
    if (!keyleds_set_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                          colors.data(), colors.size())) {
        throw Device::error(keyleds_get_error_str());
    }
}

Device::color_directive_list Device::getColors(const KeyBlock & block)
{
    color_directive_list result(block.keys().size());
    if (!keyleds_get_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT, block.id(),
                          result.data(), 0, result.size())) {
        throw Device::error(keyleds_get_error_str());
    }
    return result;
}

void Device::commitColors()
{
    if (!keyleds_commit_leds(m_device.get(), KEYLEDS_TARGET_DEFAULT)) {
        throw Device::error(keyleds_get_error_str());
    }
}

/****************************************************************************/

DeviceWatcher::DeviceWatcher(struct udev * udev, QObject *parent)
    : FilteredDeviceWatcher(udev, parent)
{
    setSubsystem("hidraw");
}

bool DeviceWatcher::isVisible(const device::DeviceDescription & dev) const
{
    auto iface = dev.parentWithType("usb", "usb_interface");
    if (std::stoul(iface.attributes().at("bInterfaceProtocol"), nullptr, 16) != 0) {
        return false;
    }

    auto usbdev = dev.parentWithType("usb", "usb_device");
    if (std::stoul(usbdev.attributes().at("idVendor"), nullptr, 16) != LOGITECH_VENDOR_ID) {
        return false;
    }

    return true;
}
