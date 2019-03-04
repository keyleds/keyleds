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
#include "keyledsd/service/DeviceManager_util.h"

#include "keyledsd/device/Device.h"
#include "keyledsd/device/LayoutDescription.h"
#include "keyledsd/KeyDatabase.h"
#include "keyledsd/logging.h"
#include "keyledsd/tools/DeviceWatcher.h"
#include <algorithm>
#include <iomanip>
#include <sstream>

LOGGING("device-manager");

namespace keyleds::service {

static constexpr int fallbackLayoutIndex = 2;

static std::string layoutName(const std::string & model, int layout)
{
    std::ostringstream fileNameBuf;
    fileNameBuf.fill('0');
    fileNameBuf <<model <<'_' <<std::hex <<std::setw(4) <<layout <<".yaml";
    return fileNameBuf.str();
}

device::LayoutDescription loadLayout(const device::Device & device)
{
    auto attempts = std::vector<int>{ fallbackLayoutIndex };
    if (device.hasLayout()) { attempts.insert(attempts.begin(), device.layout()); }

    for (auto layoutId : attempts) {
        auto name = layoutName(device.model(), layoutId);
        try {
            auto result = device::LayoutDescription::loadFile(name);
            DEBUG("loaded layout <", name, ">");
            return result;
        } catch (std::runtime_error & error) {
            ERROR("could not load layout <", name, ">: ", error.what());
        }
    }
    return {};
}

std::vector<std::string> findEventDevices(const tools::device::Description & description)
{
    // Event devices are any device detected as input devices and attached
    // to same USB device as ours
    std::vector<std::string> result;
    const auto & usbdev = description.parentWithType("usb", "usb_device");
    if (!usbdev) { return result; }

    const auto & candidates = usbdev->descendantsWithType("input");
    for (const auto & candidate : candidates) {
        auto && devNode = candidate.devNode();
        if (!devNode.empty()) {
            result.emplace_back(std::forward<decltype(devNode)>(devNode));
        }
    }
    return result;
}

std::string getSerial(const tools::device::Description & description)
{
    // Serial is stored on master USB device, so we walk up the hierarchy
    auto usbDevDescription = description.parentWithType("usb", "usb_device");
    if (!usbDevDescription) {
        throw std::runtime_error("Device is not an usb device: " + description.sysPath());
    }

    auto serial = getAttribute(*usbDevDescription, "serial");
    if (!serial) {
        throw std::runtime_error("Device has no serial: " + description.sysPath());
    }
    return *serial;
}

static KeyDatabase buildKeyDatabase(const device::Device & device, const device::LayoutDescription & layout)
{
    std::vector<KeyDatabase::Key> db;
    RenderTarget::size_type keyIndex = 0;
    for (const auto & block : device.blocks()) {
        for (unsigned kidx = 0; kidx < block.keys().size(); ++kidx) {
            const auto keyId = block.keys()[kidx];
            auto spurious = false;
            std::string name;
            auto position = KeyDatabase::Rect{0, 0, 0, 0};

            auto it = std::find_if(
                layout.spurious.cbegin(), layout.spurious.cend(),
                [&](const auto & pos) { return pos.first == block.id() && pos.second == keyId; }
            );
            if (it != layout.spurious.cend()) {
                DEBUG("marking <", int(block.id()), ", ", int(keyId), "> as spurious");
                spurious = true;
            }

            for (const auto & key : layout.keys) {
                if (key.block == block.id() && key.code == keyId) {
                    name = key.name;
                    position = {
                        KeyDatabase::position_type(key.position.x0),
                        KeyDatabase::position_type(key.position.y0),
                        KeyDatabase::position_type(key.position.x1),
                        KeyDatabase::position_type(key.position.y1)
                    };
                    break;
                }
            }
            if (name.empty()) { name = device.resolveKey(block.id(), keyId); }

            db.push_back({
                keyIndex,
                spurious ? 0 : device.decodeKeyId(block.id(), keyId),
                spurious ? std::string() : std::move(name),
                position
            });
            ++keyIndex;
        }
    }
    return KeyDatabase(db);
}

KeyDatabase setupKeyDatabase(device::Device & device)
{
    // Load layout description file from disk
    auto layout = loadLayout(device);

    // Some keyboards do not report all keys, look for missing keys and patch device
    for (const auto & block : device.blocks()) {
        std::vector<device::Device::key_id_type> keyIds;

        for (const auto & key : layout.keys) {
            if (key.block != block.id()) { continue; }
            if (key.code > std::numeric_limits<device::Device::key_id_type>::max()) {
                WARNING("invalid key code ", key.code, " in layout");
                continue;
            }
            if (std::find(block.keys().begin(), block.keys().end(), key.code) == block.keys().end()) {
                keyIds.push_back(static_cast<device::Device::key_id_type>(key.code));
            }
        }
        if (!keyIds.empty()) {
            DEBUG("patching ", keyIds.size(), " missing keys in block ", block.name());
            device.patchMissingKeys(block, keyIds);
        }
    }

    return buildKeyDatabase(device, layout);
}

}
