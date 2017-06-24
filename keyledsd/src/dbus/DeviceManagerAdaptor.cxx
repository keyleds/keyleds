#include "dbus/DeviceManagerAdaptor.h"

using dbus::DeviceManagerAdaptor;

DeviceManagerAdaptor::DeviceManagerAdaptor(keyleds::DeviceManager *parent)
    : QDBusAbstractAdaptor(parent)
{
    setAutoRelaySignals(true);
}
