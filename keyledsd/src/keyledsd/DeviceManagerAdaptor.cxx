#include "keyledsd/DeviceManagerAdaptor.h"

using keyleds::DeviceManagerAdaptor;

DeviceManagerAdaptor::DeviceManagerAdaptor(keyleds::DeviceManager *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
}

DeviceManagerAdaptor::~DeviceManagerAdaptor()
{
    // destructor
}

QString DeviceManagerAdaptor::serial() const
{
    auto usbDevDescription = parent()->description().parentWithType("usb", "usb_device");
    return usbDevDescription.attributes().at("serial").c_str();
}
