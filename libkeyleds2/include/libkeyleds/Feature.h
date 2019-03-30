/* Keyleds -- Gaming keyboard tool
 * Copyright (C) 2017-2019 Julien Hartmann, juli1.hartmann@gmail.com
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
#ifndef LIBKEYLEDS_FEATURE_H_5F4E1A42
#define LIBKEYLEDS_FEATURE_H_5F4E1A42

#include <cstdint>
#include <functional>
#include <limits>

namespace libkeyleds {

class error;

/****************************************************************************/

class Feature
{
public:
    using type_id_t = const void *;
    template <typename ... Args>
    using done_handler = std::function<void(Args...)>;
    using error_handler = std::function<void(const error&)>;

public:
    virtual ~Feature() = 0;

    virtual const char * name() const = 0;
};

template <typename T>
class FeatureCRTP : public Feature
{
    static constexpr char type_marker = 0;
public:
    static constexpr type_id_t type_id = &type_marker;

    const char * name() const final override { return T::featureName; }
};

/****************************************************************************/

/// Read firmware version from device
class FirmwareFeature : public FeatureCRTP<FirmwareFeature>
{
public:
    static constexpr auto featureName = "firmware";
public:
    virtual void version(done_handler<std::string> &&, error_handler &&) = 0;
};

/****************************************************************************/

enum class EventCode : uint16_t;

/// Mapping of buttons to keyboard keys
class KeyboardFeature : public FeatureCRTP<KeyboardFeature>
{
public:
    static constexpr auto featureName = "keyboard";
    using key_id = uint8_t;
    static constexpr auto key_invalid = std::numeric_limits<key_id>::max();

public:
    virtual key_id      keyId(EventCode) const noexcept = 0;
    virtual key_id      keyId(const std::string &) const noexcept = 0;
    virtual EventCode   keyEvent(key_id) const noexcept = 0;
    virtual const std::string & keyName(key_id) const noexcept = 0;

    virtual std::string layoutName() const = 0;
};

/****************************************************************************/

/// Configurable lights on device
class LedsFeature : public FeatureCRTP<LedsFeature>
{
public:
    static constexpr auto featureName = "leds";
public:
    struct RGBColor {
        uint8_t red, green, blue;
    };
    struct Directive {
        KeyboardFeature::key_id id;
        RGBColor                color;
    };
    using size_type = unsigned;

public:
    virtual void fill(RGBColor, done_handler<> &&, error_handler &&) = 0;
    virtual void setColors(const Directive *, size_type, done_handler<> &&) = 0;
    virtual void getColors(done_handler<Directive *> &&, error_handler &&) = 0;
};

/****************************************************************************/

/// On-device blocking of key events
class GameModeFeature : public FeatureCRTP<GameModeFeature>
{
public:
    static constexpr auto featureName = "game-mode";
public:
    virtual void setBlockedKeys(const std::vector<KeyboardFeature::key_id> &,
                                done_handler<> &&, error_handler &&) = 0;
};

/****************************************************************************/

/// Non-standard key/button events
class ExtraEventsFeature : public FeatureCRTP<ExtraEventsFeature>
{
protected:
    using key_type = const void *;
public:
    static constexpr auto featureName = "extra-events";
    using event_id = unsigned;
    using event_handler = std::function<void(event_id)>;
public:
    virtual const std::string & eventName(event_id) const = 0;

    virtual void registerHandler(key_type, event_handler &&) = 0;
    virtual void unregisterHandler(key_type) = 0;
};


/****************************************************************************/

} // namespace libkeyleds

#endif
