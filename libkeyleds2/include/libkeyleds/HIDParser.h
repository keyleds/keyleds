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
#ifndef LIBKEYLEDS_HIDPARSER_H_92AAA3A9
#define LIBKEYLEDS_HIDPARSER_H_92AAA3A9

#include <cstdint>
#include <numeric>
#include <optional>
#include <vector>

namespace libkeyleds::hid {

enum class Tag : uint8_t {
    // Main tags
    Input           = 0b1000'00 << 2,
    Output          = 0b1001'00 << 2,
    Feature         = 0b1011'00 << 2,
    Collection      = 0b1010'00 << 2,
    EndCollection   = 0b1100'00 << 2,

    // Global tags
    UsagePage       = 0b0000'01 << 2,
    LogicalMinimum  = 0b0001'01 << 2,
    LogicalMaximum  = 0b0010'01 << 2,
    PhysicalMinimum = 0b0011'01 << 2,
    PhysicalMaximum = 0b0100'01 << 2,
    UnitExponent    = 0b0101'01 << 2,
    Unit            = 0b0110'01 << 2,
    ReportSize      = 0b0111'01 << 2,
    ReportId        = 0b1000'01 << 2,
    ReportCount     = 0b1001'01 << 2,
    Push            = 0b1010'01 << 2,
    Pop             = 0b1011'01 << 2,

    // Local tags
    Usage           = 0b0000'10 << 2,
    UsageMinimum    = 0b0001'10 << 2,
    UsageMaximum    = 0b0010'10 << 2,
    DesignatorIndex = 0b0011'10 << 2,
    DesignatorMinimum = 0b0100'10 << 2,
    DesignatorMaximum = 0b0101'10 << 2,
    StringIndex     = 0b0111'10 << 2,
    StringMinimum   = 0b1000'10 << 2,
    StringMaximum   = 0b1001'10 << 2,
    Delimiter       = 0b1010'10 << 2,

    Invalid         = 0b1111'11 << 2
};

enum class CollectionType : uint8_t {
    Physical = 0x00,
    Application = 0x01,
    Logical = 0x02,
    Report = 0x03,
    NamedArray = 0x04,
    UsageSwitch = 0x05,
    UsageModifier = 0x06
};

enum class UsagePage : uint16_t {
    Undefined = 0x00,
    GenericDesktopControls = 0x01,
    SimulationControls = 0x02,
    VRControls = 0x03,
    SportControls = 0x04,
    GameControls = 0x05,
    GenericDeviceControls = 0x06,
    Keyboard = 0x07,
    LEDs = 0x08,
    Button = 0x09,
    Ordinal = 0x0a,
    Telephony = 0x0b,
    Consumer = 0x0c,
    Digitizer = 0x0d,
    PIDPage = 0x0f,
    Unicode = 0x10,
    AlphanumericDisplay = 0x14,
    MedicalInstruments = 0x40,
    BarCodeScanner = 0x8c,
    Scale = 0x8d,
    MRSDevices = 0x8e,
    CameraControl = 0x90,
    Arcade = 0x91,
    VendorStart = 0xff00,
    VendorEnd = 0xffff
};

enum class Usage : uint32_t {
    Undefined = 0x00000000,

    // GenericDesktopControls
    Pointer = 0x00010001,
    Mouse = 0x00010002,
    Joystick = 0x00010004,
    GamePad = 0x00010005,
    Keyboard = 0x00010006,
    Keypad = 0x00010007,
    MultiAxisControler = 0x00010008,
    TabletPCSystemControls = 0x00010009,
    X = 0x00010030,
    Y = 0x00010031,
    Z = 0x00010032,
    Rx = 0x00010033,
    Ry = 0x00010034,
    Rz = 0x00010035,
    Slider = 0x00010036,
    Dial = 0x00010037,
    Wheel = 0x00010038,
    HatSwitch = 0x00010039,
    ResolutionMultiplier = 0x00010048,

    // Consumer
    ConsumerControl = 0x000c0001,
    NumericKeyPad = 0x000c0002,
    ProgrammableButtons = 0x000c0003,
    Microphone = 0x000c0004,
    Headphone = 0x000c0005,
    GraphicEqualizer = 0x000c0006,
    FunctionButtons = 0x000c0036,
    Selection = 0x000c0080,
    MediaSelection = 0x000c0087,
    SpeakerSystem = 0x000c0160,
    ApplicationLaunchButtons = 0x000c0180,
    GenericGUIApplicationControls = 0x000c0200,

    PageMask = 0xffff0000
};

/****************************************************************************/

struct ReportDescriptor
{
    using collection_index = unsigned;
    static constexpr collection_index no_collection = std::numeric_limits<collection_index>::max();

    struct Collection
    {
        collection_index        parent;
        CollectionType          type;
        Usage                   usage;
        std::vector<collection_index> children; ///< index of subcollections
    };

    struct LocalItem
    {
        Tag      tag;
        uint32_t    value;
    };

    struct Field
    {
        collection_index        collectionIdx;

        Tag                     tag;
        uint32_t                flags;
        UsagePage               usagePage;
        int32_t                 logicalMinimum;
        int32_t                 logicalMaximum;
        std::optional<int32_t>  physicalMinimum;
        std::optional<int32_t>  physicalMaximum;
        uint32_t                unit;           // nibble field
        signed                  exponent;
        uint8_t                 reportSize;     // in bits
        uint8_t                 reportCount;    // each being reportSize bits
        std::vector<LocalItem>  items;
    };

    struct Report
    {
        static constexpr uint8_t invalid_id = 0;

        uint8_t                 id;
        std::vector<Field>      fields;
    };

public:
    std::vector<Collection>  collections;
    std::vector<Report>      reports;
};

class parse_error : public std::runtime_error
{
    using std::runtime_error::runtime_error;
};

std::optional<ReportDescriptor> parse(const uint8_t * data, std::size_t length);

/****************************************************************************/

} // namespace libkeyleds

#endif
