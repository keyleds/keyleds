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
#include "libkeyleds/HIDParser.h"

#include <gtest/gtest.h>

namespace hid = libkeyleds::hid;
static constexpr auto no_collection = hid::ReportDescriptor::no_collection;

namespace libkeyleds::hid {
    bool operator==(const ReportDescriptor::LocalItem & lhs, const ReportDescriptor::LocalItem & rhs)
    { return lhs.tag == rhs.tag && lhs.value == rhs.value; }
}

static constexpr const uint8_t g410[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xE7, 0x00,  //   Logical Maximum (231)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x14,        //   Report Count (20)
    0x85, 0x01,        //   Report ID (1)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x05, 0x0C,        // Usage Page (Consumer)
    0x09, 0x01,        // Usage (Consumer Control)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x02,        //   Report ID (2)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x07,        //   Report Count (7)
    0x09, 0xB5,        //   Usage (Scan Next Track)
    0x09, 0xB6,        //   Usage (Scan Previous Track)
    0x09, 0xB7,        //   Usage (Stop)
    0x09, 0xCD,        //   Usage (Play/Pause)
    0x09, 0xE9,        //   Usage (Volume Increment)
    0x09, 0xEA,        //   Usage (Volume Decrement)
    0x09, 0xE2,        //   Usage (Mute)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
    0x06, 0x43, 0xFF,  // Usage Page (Vendor Defined 0xFF43)
    0x0A, 0x02, 0x06,  // Usage (0x0602)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x11,        //   Report ID (17)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x13,        //   Report Count (19)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x09, 0x02,        //   Usage (0x02)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x02,        //   Usage (0x02)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
    0x06, 0x43, 0xFF,  // Usage Page (Vendor Defined 0xFF43)
    0x0A, 0x04, 0x06,  // Usage (0x0604)
    0xA1, 0x01,        // Collection (Application)
    0x85, 0x12,        //   Report ID (18)
    0x75, 0x08,        //   Report Size (8)
    0x95, 0x3F,        //   Report Count (63)
    0x15, 0x00,        //   Logical Minimum (0)
    0x26, 0xFF, 0x00,  //   Logical Maximum (255)
    0x09, 0x04,        //   Usage (0x04)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x09, 0x04,        //   Usage (0x04)
    0x91, 0x00,        //   Output (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0xC0,              // End Collection
};

TEST(HIDParser, g410) {
    auto description = libkeyleds::hid::parse(g410, sizeof(g410));
    ASSERT_TRUE(description);

    ASSERT_EQ(description->collections.size(), 4);
    ASSERT_EQ(description->reports.size(), 4);

    {   // Collection #0
        const auto & collection = description->collections[0];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage::Keyboard);
        EXPECT_TRUE(collection.children.empty());
    }
    {   // Collection #1
        const auto & collection = description->collections[1];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage::ConsumerControl);
        EXPECT_TRUE(collection.children.empty());
    }
    {   // Collection #2
        const auto & collection = description->collections[2];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage{0xff430602});
        EXPECT_TRUE(collection.children.empty());
    }
    {   // Collection #3
        const auto & collection = description->collections[3];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage{0xff430604});
        EXPECT_TRUE(collection.children.empty());
    }

    {   // Report #0
        const auto & report = description->reports[0];
        EXPECT_EQ(report.id, 0x01);
        ASSERT_EQ(report.fields.size(), 1);
        const auto & field = report.fields[0];
        EXPECT_EQ(field.collectionIdx, 0);
        EXPECT_EQ(field.tag, hid::Tag::Input);
        EXPECT_EQ(field.flags, 0);
        EXPECT_EQ(field.usagePage, hid::UsagePage::Keyboard);
        EXPECT_EQ(field.logicalMinimum, 0);
        EXPECT_EQ(field.logicalMaximum, 231);
        EXPECT_FALSE(field.physicalMinimum);
        EXPECT_FALSE(field.physicalMaximum);
        EXPECT_EQ(field.unit, 0);
        EXPECT_EQ(field.exponent, 0);
        EXPECT_EQ(field.reportSize, 8);
        EXPECT_EQ(field.reportCount, 20);
    }
    {   // Report #1
        const auto & report = description->reports[1];
        EXPECT_EQ(report.id, 0x02);
        ASSERT_EQ(report.fields.size(), 2);
        const auto & fields = report.fields;
        EXPECT_EQ(fields[0].collectionIdx, 1);
        EXPECT_EQ(fields[0].tag, hid::Tag::Input);
        EXPECT_EQ(fields[0].flags, 0x02);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage::Consumer);
        EXPECT_EQ(fields[0].logicalMinimum, 0);
        EXPECT_EQ(fields[0].logicalMaximum, 1);
        EXPECT_FALSE(fields[0].physicalMinimum);
        EXPECT_FALSE(fields[0].physicalMaximum);
        EXPECT_EQ(fields[0].unit, 0);
        EXPECT_EQ(fields[0].exponent, 0);
        EXPECT_EQ(fields[0].reportSize, 1);
        EXPECT_EQ(fields[0].reportCount, 7);
        EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0x000c00b5 },
            { hid::Tag::Usage, 0x000c00b6 },
            { hid::Tag::Usage, 0x000c00b7 },
            { hid::Tag::Usage, 0x000c00cd },
            { hid::Tag::Usage, 0x000c00e9 },
            { hid::Tag::Usage, 0x000c00ea },
            { hid::Tag::Usage, 0x000c00e2 },
        }));

        EXPECT_EQ(fields[1].collectionIdx, 1);
        EXPECT_EQ(fields[1].tag, hid::Tag::Input);
        EXPECT_EQ(fields[1].flags, 0x01);
        EXPECT_EQ(fields[1].usagePage, hid::UsagePage::Consumer);
        EXPECT_EQ(fields[1].logicalMinimum, 0);
        EXPECT_EQ(fields[1].logicalMaximum, 1);
        EXPECT_FALSE(fields[1].physicalMinimum);
        EXPECT_FALSE(fields[1].physicalMaximum);
        EXPECT_EQ(fields[1].unit, 0);
        EXPECT_EQ(fields[1].exponent, 0);
        EXPECT_EQ(fields[1].reportSize, 1);
        EXPECT_EQ(fields[1].reportCount, 1);
        EXPECT_EQ(fields[1].items.size(), 0);
    }
    {   // Report #2
        const auto & report = description->reports[2];
        EXPECT_EQ(report.id, 0x11);
        ASSERT_EQ(report.fields.size(), 2);
        const auto & fields = report.fields;
        EXPECT_EQ(fields[0].collectionIdx, 2);
        EXPECT_EQ(fields[0].tag, hid::Tag::Input);
        EXPECT_EQ(fields[0].flags, 0);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage{0xff43});
        EXPECT_EQ(fields[0].logicalMinimum, 0);
        EXPECT_EQ(fields[0].logicalMaximum, 255);
        EXPECT_FALSE(fields[0].physicalMinimum);
        EXPECT_FALSE(fields[0].physicalMaximum);
        EXPECT_EQ(fields[0].unit, 0);
        EXPECT_EQ(fields[0].exponent, 0);
        EXPECT_EQ(fields[0].reportSize, 8);
        EXPECT_EQ(fields[0].reportCount, 19);
        EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0xff430002 },
        }));

        EXPECT_EQ(fields[1].collectionIdx, 2);
        EXPECT_EQ(fields[1].tag, hid::Tag::Output);
        EXPECT_EQ(fields[1].flags, 0);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage{0xff43});
        EXPECT_EQ(fields[1].logicalMinimum, 0);
        EXPECT_EQ(fields[1].logicalMaximum, 255);
        EXPECT_FALSE(fields[1].physicalMinimum);
        EXPECT_FALSE(fields[1].physicalMaximum);
        EXPECT_EQ(fields[1].unit, 0);
        EXPECT_EQ(fields[1].exponent, 0);
        EXPECT_EQ(fields[1].reportSize, 8);
        EXPECT_EQ(fields[1].reportCount, 19);
        EXPECT_EQ(fields[1].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0xff430002 },
        }));
    }
    {   // Report #3
        const auto & report = description->reports[3];
        EXPECT_EQ(report.id, 0x12);
        ASSERT_EQ(report.fields.size(), 2);
        const auto & fields = report.fields;
        EXPECT_EQ(fields[0].collectionIdx, 3);
        EXPECT_EQ(fields[0].tag, hid::Tag::Input);
        EXPECT_EQ(fields[0].flags, 0);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage{0xff43});
        EXPECT_EQ(fields[0].logicalMinimum, 0);
        EXPECT_EQ(fields[0].logicalMaximum, 255);
        EXPECT_FALSE(fields[0].physicalMinimum);
        EXPECT_FALSE(fields[0].physicalMaximum);
        EXPECT_EQ(fields[0].unit, 0);
        EXPECT_EQ(fields[0].exponent, 0);
        EXPECT_EQ(fields[0].reportSize, 8);
        EXPECT_EQ(fields[0].reportCount, 63);
        EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0xff430004 },
        }));

        EXPECT_EQ(fields[1].collectionIdx, 3);
        EXPECT_EQ(fields[1].tag, hid::Tag::Output);
        EXPECT_EQ(fields[1].flags, 0);
        EXPECT_EQ(fields[1].usagePage, hid::UsagePage{0xff43});
        EXPECT_EQ(fields[1].logicalMinimum, 0);
        EXPECT_EQ(fields[1].logicalMaximum, 255);
        EXPECT_FALSE(fields[1].physicalMinimum);
        EXPECT_FALSE(fields[1].physicalMaximum);
        EXPECT_EQ(fields[1].unit, 0);
        EXPECT_EQ(fields[1].exponent, 0);
        EXPECT_EQ(fields[1].reportSize, 8);
        EXPECT_EQ(fields[1].reportCount, 63);
        EXPECT_EQ(fields[1].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0xff430004 },
        }));
    }
}

static constexpr const uint8_t specKeyboard[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x06,        // Usage (Keyboard)
    0xA1, 0x01,        // Collection (Application)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0xE0,        //   Usage Minimum (0xE0)
    0x29, 0xE7,        //   Usage Maximum (0xE7)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x01,        //   Logical Maximum (1)
    0x75, 0x01,        //   Report Size (1)
    0x95, 0x08,        //   Report Count (8)
    0x81, 0x02,        //   Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x08,        //   Report Size (8)
    0x81, 0x01,        //   Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x05,        //   Report Count (5)
    0x75, 0x01,        //   Report Size (1)
    0x05, 0x08,        //   Usage Page (LEDs)
    0x19, 0x01,        //   Usage Minimum (Num Lock)
    0x29, 0x05,        //   Usage Maximum (Kana)
    0x91, 0x02,        //   Output (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x01,        //   Report Count (1)
    0x75, 0x03,        //   Report Size (3)
    0x91, 0x01,        //   Output (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x95, 0x06,        //   Report Count (6)
    0x75, 0x08,        //   Report Size (8)
    0x15, 0x00,        //   Logical Minimum (0)
    0x25, 0x65,        //   Logical Maximum (101)
    0x05, 0x07,        //   Usage Page (Kbrd/Keypad)
    0x19, 0x00,        //   Usage Minimum (0x00)
    0x29, 0x65,        //   Usage Maximum (0x65)
    0x81, 0x00,        //   Input (Data,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              // End Collection
};

TEST(HIDParser, specKeyboard) {
    auto description = libkeyleds::hid::parse(specKeyboard, sizeof(specKeyboard));
    ASSERT_TRUE(description);

    ASSERT_EQ(description->collections.size(), 1);
    ASSERT_EQ(description->reports.size(), 1);

    const auto & collection = description->collections[0];
    EXPECT_EQ(collection.parent, no_collection);
    EXPECT_EQ(collection.type, hid::CollectionType::Application);
    EXPECT_EQ(collection.usage, hid::Usage::Keyboard);
    EXPECT_TRUE(collection.children.empty());

    const auto & report = description->reports[0];
    EXPECT_EQ(report.id, 0);
    ASSERT_EQ(report.fields.size(), 5);

    const auto & fields = report.fields;
    EXPECT_EQ(fields[0].collectionIdx, 0);
    EXPECT_EQ(fields[0].tag, hid::Tag::Input);
    EXPECT_EQ(fields[0].flags, 0x02);
    EXPECT_EQ(fields[0].usagePage, hid::UsagePage::Keyboard);
    EXPECT_EQ(fields[0].logicalMinimum, 0);
    EXPECT_EQ(fields[0].logicalMaximum, 1);
    EXPECT_FALSE(fields[0].physicalMinimum);
    EXPECT_FALSE(fields[0].physicalMaximum);
    EXPECT_EQ(fields[0].unit, 0);
    EXPECT_EQ(fields[0].exponent, 0);
    EXPECT_EQ(fields[0].reportSize, 1);
    EXPECT_EQ(fields[0].reportCount, 8);
    EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
        { hid::Tag::UsageMinimum, 0xe0 },
        { hid::Tag::UsageMaximum, 0xe7 }
    }));
    EXPECT_EQ(fields[1].collectionIdx, 0);
    EXPECT_EQ(fields[1].tag, hid::Tag::Input);
    EXPECT_EQ(fields[1].flags, 0x01);
    EXPECT_EQ(fields[1].usagePage, hid::UsagePage::Keyboard);
    EXPECT_EQ(fields[1].logicalMinimum, 0);
    EXPECT_EQ(fields[1].logicalMaximum, 1);
    EXPECT_FALSE(fields[1].physicalMinimum);
    EXPECT_FALSE(fields[1].physicalMaximum);
    EXPECT_EQ(fields[1].unit, 0);
    EXPECT_EQ(fields[1].exponent, 0);
    EXPECT_EQ(fields[1].reportSize, 8);
    EXPECT_EQ(fields[1].reportCount, 1);
    EXPECT_TRUE(fields[1].items.empty());
    EXPECT_EQ(fields[2].collectionIdx, 0);
    EXPECT_EQ(fields[2].tag, hid::Tag::Output);
    EXPECT_EQ(fields[2].flags, 0x02);
    EXPECT_EQ(fields[2].usagePage, hid::UsagePage::LEDs);
    EXPECT_EQ(fields[2].logicalMinimum, 0);
    EXPECT_EQ(fields[2].logicalMaximum, 1);
    EXPECT_FALSE(fields[2].physicalMinimum);
    EXPECT_FALSE(fields[2].physicalMaximum);
    EXPECT_EQ(fields[2].unit, 0);
    EXPECT_EQ(fields[2].exponent, 0);
    EXPECT_EQ(fields[2].reportSize, 1);
    EXPECT_EQ(fields[2].reportCount, 5);
    EXPECT_EQ(fields[2].items, (std::vector<hid::ReportDescriptor::LocalItem>{
        { hid::Tag::UsageMinimum, 0x01 },
        { hid::Tag::UsageMaximum, 0x05 }
    }));
    EXPECT_EQ(fields[3].collectionIdx, 0);
    EXPECT_EQ(fields[3].tag, hid::Tag::Output);
    EXPECT_EQ(fields[3].flags, 0x01);
    EXPECT_EQ(fields[3].usagePage, hid::UsagePage::LEDs);
    EXPECT_EQ(fields[3].logicalMinimum, 0);
    EXPECT_EQ(fields[3].logicalMaximum, 1);
    EXPECT_FALSE(fields[3].physicalMinimum);
    EXPECT_FALSE(fields[3].physicalMaximum);
    EXPECT_EQ(fields[3].unit, 0);
    EXPECT_EQ(fields[3].exponent, 0);
    EXPECT_EQ(fields[3].reportSize, 3);
    EXPECT_EQ(fields[3].reportCount, 1);
    EXPECT_TRUE(fields[3].items.empty());
    EXPECT_EQ(fields[4].collectionIdx, 0);
    EXPECT_EQ(fields[4].tag, hid::Tag::Input);
    EXPECT_EQ(fields[4].flags, 0);
    EXPECT_EQ(fields[4].usagePage, hid::UsagePage::Keyboard);
    EXPECT_EQ(fields[4].logicalMinimum, 0);
    EXPECT_EQ(fields[4].logicalMaximum, 101);
    EXPECT_FALSE(fields[4].physicalMinimum);
    EXPECT_FALSE(fields[4].physicalMaximum);
    EXPECT_EQ(fields[4].unit, 0);
    EXPECT_EQ(fields[4].exponent, 0);
    EXPECT_EQ(fields[4].reportSize, 8);
    EXPECT_EQ(fields[4].reportCount, 6);
    EXPECT_EQ(fields[4].items, (std::vector<hid::ReportDescriptor::LocalItem>{
        { hid::Tag::UsageMinimum, 0x00 },
        { hid::Tag::UsageMaximum, 0x65 }
    }));
}

static constexpr const uint8_t specMouse[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x01,        //   Usage (Pointer)
    0xA1, 0x00,        //   Collection (Physical)
    0x05, 0x09,        //     Usage Page (Button)
    0x19, 0x01,        //     Usage Minimum (0x01)
    0x29, 0x03,        //     Usage Maximum (0x03)
    0x15, 0x00,        //     Logical Minimum (0)
    0x25, 0x01,        //     Logical Maximum (1)
    0x95, 0x03,        //     Report Count (3)
    0x75, 0x01,        //     Report Size (1)
    0x81, 0x02,        //     Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x95, 0x01,        //     Report Count (1)
    0x75, 0x05,        //     Report Size (5)
    0x81, 0x01,        //     Input (Const,Array,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //     Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //     Usage (X)
    0x09, 0x31,        //     Usage (Y)
    0x15, 0x81,        //     Logical Minimum (-127)
    0x25, 0x7F,        //     Logical Maximum (127)
    0x75, 0x08,        //     Report Size (8)
    0x95, 0x02,        //     Report Count (2)
    0x81, 0x06,        //     Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //   End Collection
    0xC0,              // End Collection
};

TEST(HIDParser, specMouse) {
    auto description = libkeyleds::hid::parse(specMouse, sizeof(specMouse));
    ASSERT_TRUE(description);

    ASSERT_EQ(description->collections.size(), 2);
    ASSERT_EQ(description->reports.size(), 1);

    {   // Collection #0
        const auto & collection = description->collections[0];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage::Mouse);
        EXPECT_EQ(collection.children, std::vector<hid::ReportDescriptor::collection_index>{1});
    }
    {   // Collection #1
        const auto & collection = description->collections[1];
        EXPECT_EQ(collection.parent, 0);
        EXPECT_EQ(collection.type, hid::CollectionType::Physical);
        EXPECT_EQ(collection.usage, hid::Usage::Pointer);
        EXPECT_TRUE(collection.children.empty());
    }
    {   // Report #0
        const auto & report = description->reports[0];
        EXPECT_EQ(report.id, 0);
        ASSERT_EQ(report.fields.size(), 3);

        const auto & fields = report.fields;
        EXPECT_EQ(fields[0].collectionIdx, 1);
        EXPECT_EQ(fields[0].tag, hid::Tag::Input);
        EXPECT_EQ(fields[0].flags, 0x02);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage::Button);
        EXPECT_EQ(fields[0].logicalMinimum, 0);
        EXPECT_EQ(fields[0].logicalMaximum, 1);
        EXPECT_FALSE(fields[0].physicalMinimum);
        EXPECT_FALSE(fields[0].physicalMaximum);
        EXPECT_EQ(fields[0].unit, 0);
        EXPECT_EQ(fields[0].exponent, 0);
        EXPECT_EQ(fields[0].reportSize, 1);
        EXPECT_EQ(fields[0].reportCount, 3);
        EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::UsageMinimum, 0x01 },
            { hid::Tag::UsageMaximum, 0x03 }
        }));
        EXPECT_EQ(fields[1].collectionIdx, 1);
        EXPECT_EQ(fields[1].tag, hid::Tag::Input);
        EXPECT_EQ(fields[1].flags, 0x01);
        EXPECT_EQ(fields[1].usagePage, hid::UsagePage::Button);
        EXPECT_EQ(fields[1].logicalMinimum, 0);
        EXPECT_EQ(fields[1].logicalMaximum, 1);
        EXPECT_FALSE(fields[1].physicalMinimum);
        EXPECT_FALSE(fields[1].physicalMaximum);
        EXPECT_EQ(fields[1].unit, 0);
        EXPECT_EQ(fields[1].exponent, 0);
        EXPECT_EQ(fields[1].reportSize, 5);
        EXPECT_EQ(fields[1].reportCount, 1);
        EXPECT_TRUE(fields[1].items.empty());
        EXPECT_EQ(fields[2].collectionIdx, 1);
        EXPECT_EQ(fields[2].tag, hid::Tag::Input);
        EXPECT_EQ(fields[2].flags, 0x06);
        EXPECT_EQ(fields[2].usagePage, hid::UsagePage::GenericDesktopControls);
        EXPECT_EQ(fields[2].logicalMinimum, -127);
        EXPECT_EQ(fields[2].logicalMaximum, 127);
        EXPECT_FALSE(fields[2].physicalMinimum);
        EXPECT_FALSE(fields[2].physicalMaximum);
        EXPECT_EQ(fields[2].unit, 0);
        EXPECT_EQ(fields[2].exponent, 0);
        EXPECT_EQ(fields[2].reportSize, 8);
        EXPECT_EQ(fields[2].reportCount, 2);
        EXPECT_EQ(fields[2].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0x00010030 },
            { hid::Tag::Usage, 0x00010031 }
        }));
    }
}

static constexpr const uint8_t mcMouse[] = {
    0x05, 0x01,        // Usage Page (Generic Desktop Ctrls)
    0x09, 0x02,        // Usage (Mouse)
    0xA1, 0x01,        // Collection (Application)
    0x09, 0x02,        //   Usage (Mouse)
    0xA1, 0x02,        //   Collection (Logical)
    0x09, 0x01,        //     Usage (Pointer)
    0xA1, 0x00,        //     Collection (Physical)
    0x05, 0x09,        //       Usage Page (Button)
    0x19, 0x01,        //       Usage Minimum (0x01)
    0x29, 0x05,        //       Usage Maximum (0x05)
    0x15, 0x00,        //       Logical Minimum (0)
    0x25, 0x01,        //       Logical Maximum (1)
    0x75, 0x01,        //       Report Size (1)
    0x95, 0x05,        //       Report Count (5)
    0x81, 0x02,        //       Input (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x75, 0x03,        //       Report Size (3)
    0x95, 0x01,        //       Report Count (1)
    0x81, 0x03,        //       Input (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position)
    0x05, 0x01,        //       Usage Page (Generic Desktop Ctrls)
    0x09, 0x30,        //       Usage (X)
    0x09, 0x31,        //       Usage (Y)
    0x15, 0x81,        //       Logical Minimum (-127)
    0x25, 0x7F,        //       Logical Maximum (127)
    0x75, 0x08,        //       Report Size (8)
    0x95, 0x02,        //       Report Count (2)
    0x81, 0x06,        //       Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xA1, 0x02,        //       Collection (Logical)
    0x09, 0x48,        //         Usage (Resolution Multiplier)
    0x15, 0x00,        //         Logical Minimum (0)
    0x25, 0x01,        //         Logical Maximum (1)
    0x35, 0x01,        //         Physical Minimum (1)
    0x45, 0x04,        //         Physical Maximum (4)
    0x75, 0x02,        //         Report Size (2)
    0x95, 0x01,        //         Report Count (1)
    0xA4,              //         Push
    0xB1, 0x02,        //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x38,        //         Usage (Wheel)
    0x15, 0x81,        //         Logical Minimum (-127)
    0x25, 0x7F,        //         Logical Maximum (127)
    0x35, 0x00,        //         Physical Minimum (0)
    0x45, 0x00,        //         Physical Maximum (0)
    0x75, 0x08,        //         Report Size (8)
    0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //       End Collection
    0xA1, 0x02,        //       Collection (Logical)
    0xB4,              //         Pop
    0x09, 0x48,        //         Usage (Resolution Multiplier)
    0xB1, 0x02,        //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x35, 0x00,        //         Physical Minimum (0)
    0x45, 0x00,        //         Physical Maximum (0)
    0x05, 0x0C,        //         Usage Page (Consumer)
    0x0A, 0x38, 0x02,  //         Usage (AC Pan)
    0x15, 0x81,        //         Logical Minimum (-127)
    0x25, 0x7F,        //         Logical Maximum (127)
    0x75, 0x08,        //         Report Size (8)
    0x81, 0x06,        //         Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //       End Collection
    0xC0,              //     End Collection
    0xC0,              //   End Collection
    0xC0              // End Collection
};


TEST(HIDParser, mcMouse) {
    auto description = libkeyleds::hid::parse(mcMouse, sizeof(mcMouse));
    ASSERT_TRUE(description);

    ASSERT_EQ(description->collections.size(), 5);
    ASSERT_EQ(description->reports.size(), 1);

    {   // Collection #0
        const auto & collection = description->collections[0];
        EXPECT_EQ(collection.parent, no_collection);
        EXPECT_EQ(collection.type, hid::CollectionType::Application);
        EXPECT_EQ(collection.usage, hid::Usage::Mouse);
        ASSERT_EQ(collection.children, std::vector<hid::ReportDescriptor::collection_index>{1});
    }
    {   // Collection #1
        const auto & collection = description->collections[1];
        EXPECT_EQ(collection.parent, 0);
        EXPECT_EQ(collection.type, hid::CollectionType::Logical);
        EXPECT_EQ(collection.usage, hid::Usage::Mouse);
        ASSERT_EQ(collection.children, std::vector<hid::ReportDescriptor::collection_index>{2});
    }
    {   // Collection #2
        const auto & collection = description->collections[2];
        EXPECT_EQ(collection.parent, 1);
        EXPECT_EQ(collection.type, hid::CollectionType::Physical);
        EXPECT_EQ(collection.usage, hid::Usage::Pointer);
        ASSERT_EQ(collection.children, (std::vector<hid::ReportDescriptor::collection_index>{3, 4}));
    }
    {   // Collection #3
        const auto & collection = description->collections[3];
        EXPECT_EQ(collection.parent, 2);
        EXPECT_EQ(collection.type, hid::CollectionType::Logical);
        EXPECT_EQ(collection.usage, hid::Usage{0x00010000});
        ASSERT_TRUE(collection.children.empty());
    }
    {   // Collection #4
        const auto & collection = description->collections[4];
        EXPECT_EQ(collection.parent, 2);
        EXPECT_EQ(collection.type, hid::CollectionType::Logical);
        EXPECT_EQ(collection.usage, hid::Usage{0x00010000});
        ASSERT_TRUE(collection.children.empty());
    }
    {   // Report #0
        const auto & report = description->reports[0];
        EXPECT_EQ(report.id, 0);
        ASSERT_EQ(report.fields.size(), 7);

        const auto & fields = report.fields;
        EXPECT_EQ(fields[0].collectionIdx, 2);
        EXPECT_EQ(fields[0].tag, hid::Tag::Input);
        EXPECT_EQ(fields[0].flags, 0x02);
        EXPECT_EQ(fields[0].usagePage, hid::UsagePage::Button);
        EXPECT_EQ(fields[0].logicalMinimum, 0);
        EXPECT_EQ(fields[0].logicalMaximum, 1);
        EXPECT_FALSE(fields[0].physicalMinimum);
        EXPECT_FALSE(fields[0].physicalMaximum);
        EXPECT_EQ(fields[0].unit, 0);
        EXPECT_EQ(fields[0].exponent, 0);
        EXPECT_EQ(fields[0].reportSize, 1);
        EXPECT_EQ(fields[0].reportCount, 5);
        EXPECT_EQ(fields[0].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::UsageMinimum, 0x01 },
            { hid::Tag::UsageMaximum, 0x05 }
        }));
        EXPECT_EQ(fields[1].collectionIdx, 2);
        EXPECT_EQ(fields[1].tag, hid::Tag::Input);
        EXPECT_EQ(fields[1].flags, 0x03);
        EXPECT_EQ(fields[1].usagePage, hid::UsagePage::Button);
        EXPECT_EQ(fields[1].logicalMinimum, 0);
        EXPECT_EQ(fields[1].logicalMaximum, 1);
        EXPECT_FALSE(fields[1].physicalMinimum);
        EXPECT_FALSE(fields[1].physicalMaximum);
        EXPECT_EQ(fields[1].unit, 0);
        EXPECT_EQ(fields[1].exponent, 0);
        EXPECT_EQ(fields[1].reportSize, 3);
        EXPECT_EQ(fields[1].reportCount, 1);
        EXPECT_TRUE(fields[1].items.empty());
        EXPECT_EQ(fields[2].collectionIdx, 2);
        EXPECT_EQ(fields[2].tag, hid::Tag::Input);
        EXPECT_EQ(fields[2].flags, 0x06);
        EXPECT_EQ(fields[2].usagePage, hid::UsagePage::GenericDesktopControls);
        EXPECT_EQ(fields[2].logicalMinimum, -127);
        EXPECT_EQ(fields[2].logicalMaximum, 127);
        EXPECT_FALSE(fields[2].physicalMinimum);
        EXPECT_FALSE(fields[2].physicalMaximum);
        EXPECT_EQ(fields[2].unit, 0);
        EXPECT_EQ(fields[2].exponent, 0);
        EXPECT_EQ(fields[2].reportSize, 8);
        EXPECT_EQ(fields[2].reportCount, 2);
        EXPECT_EQ(fields[2].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, uint32_t(hid::Usage::X) },
            { hid::Tag::Usage, uint32_t(hid::Usage::Y) }
        }));
        EXPECT_EQ(fields[3].collectionIdx, 3);
        EXPECT_EQ(fields[3].tag, hid::Tag::Feature);
        EXPECT_EQ(fields[3].flags, 0x02);
        EXPECT_EQ(fields[3].usagePage, hid::UsagePage::GenericDesktopControls);
        EXPECT_EQ(fields[3].logicalMinimum, 0);
        EXPECT_EQ(fields[3].logicalMaximum, 1);
        EXPECT_EQ(fields[3].physicalMinimum, 1);
        EXPECT_EQ(fields[3].physicalMaximum, 4);
        EXPECT_EQ(fields[3].unit, 0);
        EXPECT_EQ(fields[3].exponent, 0);
        EXPECT_EQ(fields[3].reportSize, 2);
        EXPECT_EQ(fields[3].reportCount, 1);
        EXPECT_EQ(fields[3].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, uint32_t(hid::Usage::ResolutionMultiplier) },
        }));
        EXPECT_EQ(fields[4].collectionIdx, 3);
        EXPECT_EQ(fields[4].tag, hid::Tag::Input);
        EXPECT_EQ(fields[4].flags, 0x06);
        EXPECT_EQ(fields[4].usagePage, hid::UsagePage::GenericDesktopControls);
        EXPECT_EQ(fields[4].logicalMinimum, -127);
        EXPECT_EQ(fields[4].logicalMaximum, 127);
        EXPECT_EQ(fields[4].physicalMinimum, 0);
        EXPECT_EQ(fields[4].physicalMaximum, 0);
        EXPECT_EQ(fields[4].unit, 0);
        EXPECT_EQ(fields[4].exponent, 0);
        EXPECT_EQ(fields[4].reportSize, 8);
        EXPECT_EQ(fields[4].reportCount, 1);
        EXPECT_EQ(fields[4].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, uint32_t(hid::Usage::Wheel) },
        }));
        EXPECT_EQ(fields[5].collectionIdx, 4);
        EXPECT_EQ(fields[5].tag, hid::Tag::Feature);
        EXPECT_EQ(fields[5].flags, 0x02);
        EXPECT_EQ(fields[5].usagePage, hid::UsagePage::GenericDesktopControls);
        EXPECT_EQ(fields[5].logicalMinimum, 0);
        EXPECT_EQ(fields[5].logicalMaximum, 1);
        EXPECT_EQ(fields[5].physicalMinimum, 1);
        EXPECT_EQ(fields[5].physicalMaximum, 4);
        EXPECT_EQ(fields[5].unit, 0);
        EXPECT_EQ(fields[5].exponent, 0);
        EXPECT_EQ(fields[5].reportSize, 2);
        EXPECT_EQ(fields[5].reportCount, 1);
        EXPECT_EQ(fields[5].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, uint32_t(hid::Usage::ResolutionMultiplier) },
        }));
        EXPECT_EQ(fields[6].collectionIdx, 4);
        EXPECT_EQ(fields[6].tag, hid::Tag::Input);
        EXPECT_EQ(fields[6].flags, 0x06);
        EXPECT_EQ(fields[6].usagePage, hid::UsagePage::Consumer);
        EXPECT_EQ(fields[6].logicalMinimum, -127);
        EXPECT_EQ(fields[6].logicalMaximum, 127);
        EXPECT_EQ(fields[6].physicalMinimum, 0);
        EXPECT_EQ(fields[6].physicalMaximum, 0);
        EXPECT_EQ(fields[6].unit, 0);
        EXPECT_EQ(fields[6].exponent, 0);
        EXPECT_EQ(fields[6].reportSize, 8);
        EXPECT_EQ(fields[6].reportCount, 1);
        EXPECT_EQ(fields[6].items, (std::vector<hid::ReportDescriptor::LocalItem>{
            { hid::Tag::Usage, 0x000c0238 },
        }));
    }
}
