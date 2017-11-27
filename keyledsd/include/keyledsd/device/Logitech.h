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
#ifndef KEYLEDSD_LOGITECH_H_D395E8C7
#define KEYLEDSD_LOGITECH_H_D395E8C7

#include <QObject>
#include <cstdint>
#include <memory>
#include <stdexcept>
#include <string>
#include <vector>
#include "keyledsd/device/Device.h"
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

/** Logitech device interface */
class LogitechDevice final : public Device
{
public:
    // Exceptions
    class error : public Device::error
    {
        using keyleds_error_t = unsigned int;
    public:
                        error(std::string what, keyleds_error_t code, int oserror=0);
                        ~error() override;
        bool            expected() const override;
        bool            recoverable() const override;
        keyleds_error_t code() const { return m_code; }
        int             oserror() const { return m_oserror; }
    private:
        keyleds_error_t m_code;
        int             m_oserror;
    };

public:
                    LogitechDevice(std::unique_ptr<struct keyleds_device>,
                                   std::string path, Type type, std::string name, std::string model,
                                   std::string serial, std::string firmware, int layout, block_list);
                    LogitechDevice(const LogitechDevice &) = delete;
    LogitechDevice & operator=(const LogitechDevice &) = delete;
                    ~LogitechDevice() override;

    // Factory method
    static std::unique_ptr<LogitechDevice> open(std::string path);

    // Virtual method implementation
          bool          hasLayout() const override;
    std::string         resolveKey(key_block_id_type, key_id_type) const override;
    int                 decodeKeyId(key_block_id_type, key_id_type) const override;

    // Manipulate
    void                setTimeout(unsigned us) override;
    void                flush() override;
    bool                resync() noexcept override;
    void                fillColor(const KeyBlock & block, const RGBColor) override;
    void                setColors(const KeyBlock & block, const ColorDirective[], size_t size) override;
    void                getColors(const KeyBlock & block, ColorDirective[]) override;
    void                commitColors() override;

private:
    static Type         getType(struct keyleds_device *);
    static std::string  getName(struct keyleds_device *);
    static block_list   getBlocks(struct keyleds_device *);
    static void         parseVersion(struct keyleds_device *, std::string * model,
                                     std::string * serial, std::string * firmware);

private:
    std::unique_ptr<struct keyleds_device> m_device;    ///< Underlying libkeyleds opaque handle
};

/****************************************************************************/

/** Keyleds-specific device watcher
 *
 * A device::FilteredDeviceWatcher that further filters detected devices to only
 * let Logitech devices through.
 */
class LogitechDeviceWatcher : public ::device::FilteredDeviceWatcher
{
    Q_OBJECT
public:
            LogitechDeviceWatcher(struct udev * udev = nullptr, QObject *parent = nullptr);
            ~LogitechDeviceWatcher() override;
    bool    isVisible(const ::device::Description & dev) const override;
};

/****************************************************************************/

} } // namespace keyleds::device

#endif
