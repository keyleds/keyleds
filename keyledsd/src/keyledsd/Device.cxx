#include <QtCore>
#include <string>
#include "config.h"
#include "keyleds.h"
#include "keyledsd/Device.h"

using keyleds::Device;
using keyleds::DeviceWatcher;

Device::Device(const std::string & path)
    : m_device(nullptr, keyleds_close),
      m_blocks(nullptr, keyleds_free_block_info)
{
    m_device.reset(keyleds_open(path.c_str(), KEYLEDSD_APP_ID));
    if (m_device == nullptr) {
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
