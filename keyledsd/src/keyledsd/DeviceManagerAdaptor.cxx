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
