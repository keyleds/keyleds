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

using libkeyleds::hid::HIDCollectionType;
using libkeyleds::hid::HIDLocalItem;
using libkeyleds::hid::HIDTag;
using libkeyleds::hid::HIDUsage;
using libkeyleds::hid::HIDUsagePage;

namespace libkeyleds::hid {
    bool operator==(const HIDLocalItem & lhs, const HIDLocalItem & rhs)
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
    auto root = libkeyleds::hid::parse(g410, sizeof(g410));
    EXPECT_TRUE(root);

    EXPECT_EQ(root->type, HIDCollectionType::Physical);
    EXPECT_EQ(root->usage, HIDUsage::Undefined);
    ASSERT_EQ(root->subcollections.size(), 4);
    EXPECT_TRUE(root->reports.empty());

    {
        const auto & collection = root->subcollections[0];
        EXPECT_EQ(collection.type, HIDCollectionType::Application);
        EXPECT_EQ(collection.usage, HIDUsage::Keyboard);
        EXPECT_TRUE(collection.subcollections.empty());
        ASSERT_EQ(collection.reports.size(), 1);
        const auto & report = collection.reports[0];
        EXPECT_EQ(report.id, 0x01);
        ASSERT_EQ(report.items.size(), 1);
        const auto & item = report.items[0];
        EXPECT_EQ(item.tag, HIDTag::Input);
        EXPECT_EQ(item.flags, 0);
        EXPECT_EQ(item.usagePage, HIDUsagePage::Keyboard);
        EXPECT_EQ(item.logicalMinimum, 0);
        EXPECT_EQ(item.logicalMaximum, 231);
        EXPECT_FALSE(item.physicalMinimum);
        EXPECT_FALSE(item.physicalMaximum);
        EXPECT_EQ(item.unit, 0);
        EXPECT_EQ(item.exponent, 0);
        EXPECT_EQ(item.reportSize, 8);
        EXPECT_EQ(item.reportCount, 20);
    }
    {
        const auto & collection = root->subcollections[1];
        EXPECT_EQ(collection.type, HIDCollectionType::Application);
        EXPECT_EQ(collection.usage, HIDUsage::ConsumerControl);
        EXPECT_TRUE(collection.subcollections.empty());
        ASSERT_EQ(collection.reports.size(), 1);
        const auto & report = collection.reports[0];
        EXPECT_EQ(report.id, 0x02);
        ASSERT_EQ(report.items.size(), 2);
        const auto & items = report.items;
        EXPECT_EQ(items[0].tag, HIDTag::Input);
        EXPECT_EQ(items[0].flags, 0x02);
        EXPECT_EQ(items[0].usagePage, HIDUsagePage::Consumer);
        EXPECT_EQ(items[0].logicalMinimum, 0);
        EXPECT_EQ(items[0].logicalMaximum, 1);
        EXPECT_FALSE(items[0].physicalMinimum);
        EXPECT_FALSE(items[0].physicalMaximum);
        EXPECT_EQ(items[0].unit, 0);
        EXPECT_EQ(items[0].exponent, 0);
        EXPECT_EQ(items[0].reportSize, 1);
        EXPECT_EQ(items[0].reportCount, 7);
        EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
            { HIDTag::Usage, 0x000c00b5 },
            { HIDTag::Usage, 0x000c00b6 },
            { HIDTag::Usage, 0x000c00b7 },
            { HIDTag::Usage, 0x000c00cd },
            { HIDTag::Usage, 0x000c00e9 },
            { HIDTag::Usage, 0x000c00ea },
            { HIDTag::Usage, 0x000c00e2 },
        }));

        EXPECT_EQ(items[1].tag, HIDTag::Input);
        EXPECT_EQ(items[1].flags, 0x01);
        EXPECT_EQ(items[1].usagePage, HIDUsagePage::Consumer);
        EXPECT_EQ(items[1].logicalMinimum, 0);
        EXPECT_EQ(items[1].logicalMaximum, 1);
        EXPECT_FALSE(items[1].physicalMinimum);
        EXPECT_FALSE(items[1].physicalMaximum);
        EXPECT_EQ(items[1].unit, 0);
        EXPECT_EQ(items[1].exponent, 0);
        EXPECT_EQ(items[1].reportSize, 1);
        EXPECT_EQ(items[1].reportCount, 1);
        EXPECT_EQ(items[1].items.size(), 0);
    }
    {
        const auto & collection = root->subcollections[2];
        EXPECT_EQ(collection.type, HIDCollectionType::Application);
        EXPECT_EQ(collection.usage, HIDUsage{0xff430602});
        EXPECT_TRUE(collection.subcollections.empty());
        ASSERT_EQ(collection.reports.size(), 1);
        const auto & report = collection.reports[0];
        EXPECT_EQ(report.id, 0x11);
        ASSERT_EQ(report.items.size(), 2);
        const auto & items = report.items;
        EXPECT_EQ(items[0].tag, HIDTag::Input);
        EXPECT_EQ(items[0].flags, 0);
        EXPECT_EQ(items[0].usagePage, HIDUsagePage{0xff43});
        EXPECT_EQ(items[0].logicalMinimum, 0);
        EXPECT_EQ(items[0].logicalMaximum, 255);
        EXPECT_FALSE(items[0].physicalMinimum);
        EXPECT_FALSE(items[0].physicalMaximum);
        EXPECT_EQ(items[0].unit, 0);
        EXPECT_EQ(items[0].exponent, 0);
        EXPECT_EQ(items[0].reportSize, 8);
        EXPECT_EQ(items[0].reportCount, 19);
        EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
            { HIDTag::Usage, 0xff430002 },
        }));

        EXPECT_EQ(items[1].tag, HIDTag::Output);
        EXPECT_EQ(items[1].flags, 0);
        EXPECT_EQ(items[0].usagePage, HIDUsagePage{0xff43});
        EXPECT_EQ(items[1].logicalMinimum, 0);
        EXPECT_EQ(items[1].logicalMaximum, 255);
        EXPECT_FALSE(items[1].physicalMinimum);
        EXPECT_FALSE(items[1].physicalMaximum);
        EXPECT_EQ(items[1].unit, 0);
        EXPECT_EQ(items[1].exponent, 0);
        EXPECT_EQ(items[1].reportSize, 8);
        EXPECT_EQ(items[1].reportCount, 19);
        EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
            { HIDTag::Usage, 0xff430002 },
        }));
    }
    {
        const auto & collection = root->subcollections[3];
        EXPECT_EQ(collection.type, HIDCollectionType::Application);
        EXPECT_EQ(collection.usage, HIDUsage{0xff430604});
        EXPECT_TRUE(collection.subcollections.empty());
        ASSERT_EQ(collection.reports.size(), 1);
        const auto & report = collection.reports[0];
        EXPECT_EQ(report.id, 0x12);
        ASSERT_EQ(report.items.size(), 2);
        const auto & items = report.items;
        EXPECT_EQ(items[0].tag, HIDTag::Input);
        EXPECT_EQ(items[0].flags, 0);
        EXPECT_EQ(items[0].usagePage, HIDUsagePage{0xff43});
        EXPECT_EQ(items[0].logicalMinimum, 0);
        EXPECT_EQ(items[0].logicalMaximum, 255);
        EXPECT_FALSE(items[0].physicalMinimum);
        EXPECT_FALSE(items[0].physicalMaximum);
        EXPECT_EQ(items[0].unit, 0);
        EXPECT_EQ(items[0].exponent, 0);
        EXPECT_EQ(items[0].reportSize, 8);
        EXPECT_EQ(items[0].reportCount, 63);
        EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
            { HIDTag::Usage, 0xff430004 },
        }));

        EXPECT_EQ(items[1].tag, HIDTag::Output);
        EXPECT_EQ(items[1].flags, 0);
        EXPECT_EQ(items[0].usagePage, HIDUsagePage{0xff43});
        EXPECT_EQ(items[1].logicalMinimum, 0);
        EXPECT_EQ(items[1].logicalMaximum, 255);
        EXPECT_FALSE(items[1].physicalMinimum);
        EXPECT_FALSE(items[1].physicalMaximum);
        EXPECT_EQ(items[1].unit, 0);
        EXPECT_EQ(items[1].exponent, 0);
        EXPECT_EQ(items[1].reportSize, 8);
        EXPECT_EQ(items[1].reportCount, 63);
        EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
            { HIDTag::Usage, 0xff430004 },
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
    auto root = libkeyleds::hid::parse(specKeyboard, sizeof(specKeyboard));
    EXPECT_TRUE(root);

    EXPECT_EQ(root->type, HIDCollectionType::Physical);
    EXPECT_EQ(root->usage, HIDUsage::Undefined);
    ASSERT_EQ(root->subcollections.size(), 1);
    EXPECT_TRUE(root->reports.empty());

    const auto & collection = root->subcollections[0];
    EXPECT_EQ(collection.type, HIDCollectionType::Application);
    EXPECT_EQ(collection.usage, HIDUsage::Keyboard);
    EXPECT_TRUE(collection.subcollections.empty());
    ASSERT_EQ(collection.reports.size(), 1);

    const auto & report = collection.reports[0];
    EXPECT_EQ(report.id, 0);
    ASSERT_EQ(report.items.size(), 5);

    const auto & items = report.items;
    EXPECT_EQ(items[0].tag, HIDTag::Input);
    EXPECT_EQ(items[0].flags, 0x02);
    EXPECT_EQ(items[0].usagePage, HIDUsagePage::Keyboard);
    EXPECT_EQ(items[0].logicalMinimum, 0);
    EXPECT_EQ(items[0].logicalMaximum, 1);
    EXPECT_FALSE(items[0].physicalMinimum);
    EXPECT_FALSE(items[0].physicalMaximum);
    EXPECT_EQ(items[0].unit, 0);
    EXPECT_EQ(items[0].exponent, 0);
    EXPECT_EQ(items[0].reportSize, 1);
    EXPECT_EQ(items[0].reportCount, 8);
    EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
        { HIDTag::UsageMinimum, 0xe0 },
        { HIDTag::UsageMaximum, 0xe7 }
    }));
    EXPECT_EQ(items[1].tag, HIDTag::Input);
    EXPECT_EQ(items[1].flags, 0x01);
    EXPECT_EQ(items[1].usagePage, HIDUsagePage::Keyboard);
    EXPECT_EQ(items[1].logicalMinimum, 0);
    EXPECT_EQ(items[1].logicalMaximum, 1);
    EXPECT_FALSE(items[1].physicalMinimum);
    EXPECT_FALSE(items[1].physicalMaximum);
    EXPECT_EQ(items[1].unit, 0);
    EXPECT_EQ(items[1].exponent, 0);
    EXPECT_EQ(items[1].reportSize, 8);
    EXPECT_EQ(items[1].reportCount, 1);
    EXPECT_TRUE(items[1].items.empty());
    EXPECT_EQ(items[2].tag, HIDTag::Output);
    EXPECT_EQ(items[2].flags, 0x02);
    EXPECT_EQ(items[2].usagePage, HIDUsagePage::LEDs);
    EXPECT_EQ(items[2].logicalMinimum, 0);
    EXPECT_EQ(items[2].logicalMaximum, 1);
    EXPECT_FALSE(items[2].physicalMinimum);
    EXPECT_FALSE(items[2].physicalMaximum);
    EXPECT_EQ(items[2].unit, 0);
    EXPECT_EQ(items[2].exponent, 0);
    EXPECT_EQ(items[2].reportSize, 1);
    EXPECT_EQ(items[2].reportCount, 5);
    EXPECT_EQ(items[2].items, (std::vector<HIDLocalItem>{
        { HIDTag::UsageMinimum, 0x01 },
        { HIDTag::UsageMaximum, 0x05 }
    }));
    EXPECT_EQ(items[3].tag, HIDTag::Output);
    EXPECT_EQ(items[3].flags, 0x01);
    EXPECT_EQ(items[3].usagePage, HIDUsagePage::LEDs);
    EXPECT_EQ(items[3].logicalMinimum, 0);
    EXPECT_EQ(items[3].logicalMaximum, 1);
    EXPECT_FALSE(items[3].physicalMinimum);
    EXPECT_FALSE(items[3].physicalMaximum);
    EXPECT_EQ(items[3].unit, 0);
    EXPECT_EQ(items[3].exponent, 0);
    EXPECT_EQ(items[3].reportSize, 3);
    EXPECT_EQ(items[3].reportCount, 1);
    EXPECT_TRUE(items[3].items.empty());
    EXPECT_EQ(items[4].tag, HIDTag::Input);
    EXPECT_EQ(items[4].flags, 0);
    EXPECT_EQ(items[4].usagePage, HIDUsagePage::Keyboard);
    EXPECT_EQ(items[4].logicalMinimum, 0);
    EXPECT_EQ(items[4].logicalMaximum, 101);
    EXPECT_FALSE(items[4].physicalMinimum);
    EXPECT_FALSE(items[4].physicalMaximum);
    EXPECT_EQ(items[4].unit, 0);
    EXPECT_EQ(items[4].exponent, 0);
    EXPECT_EQ(items[4].reportSize, 8);
    EXPECT_EQ(items[4].reportCount, 6);
    EXPECT_EQ(items[4].items, (std::vector<HIDLocalItem>{
        { HIDTag::UsageMinimum, 0x00 },
        { HIDTag::UsageMaximum, 0x65 }
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
    auto root = libkeyleds::hid::parse(specMouse, sizeof(specMouse));
    EXPECT_TRUE(root);

    EXPECT_EQ(root->type, HIDCollectionType::Physical);
    EXPECT_EQ(root->usage, HIDUsage::Undefined);
    ASSERT_EQ(root->subcollections.size(), 1);
    EXPECT_TRUE(root->reports.empty());

    const auto & collection = root->subcollections[0];
    EXPECT_EQ(collection.type, HIDCollectionType::Application);
    EXPECT_EQ(collection.usage, HIDUsage::Mouse);
    EXPECT_TRUE(collection.reports.empty());
    ASSERT_EQ(collection.subcollections.size(), 1);

    const auto & subcollection = collection.subcollections[0];
    EXPECT_EQ(subcollection.type, HIDCollectionType::Physical);
    EXPECT_EQ(subcollection.usage, HIDUsage::Pointer);
    EXPECT_TRUE(subcollection.subcollections.empty());
    ASSERT_EQ(subcollection.reports.size(), 1);

    const auto & report = subcollection.reports[0];
    EXPECT_EQ(report.id, 0);
    ASSERT_EQ(report.items.size(), 3);

    const auto & items = report.items;
    EXPECT_EQ(items[0].tag, HIDTag::Input);
    EXPECT_EQ(items[0].flags, 0x02);
    EXPECT_EQ(items[0].usagePage, HIDUsagePage::Button);
    EXPECT_EQ(items[0].logicalMinimum, 0);
    EXPECT_EQ(items[0].logicalMaximum, 1);
    EXPECT_FALSE(items[0].physicalMinimum);
    EXPECT_FALSE(items[0].physicalMaximum);
    EXPECT_EQ(items[0].unit, 0);
    EXPECT_EQ(items[0].exponent, 0);
    EXPECT_EQ(items[0].reportSize, 1);
    EXPECT_EQ(items[0].reportCount, 3);
    EXPECT_EQ(items[0].items, (std::vector<HIDLocalItem>{
        { HIDTag::UsageMinimum, 0x01 },
        { HIDTag::UsageMaximum, 0x03 }
    }));
    EXPECT_EQ(items[1].tag, HIDTag::Input);
    EXPECT_EQ(items[1].flags, 0x01);
    EXPECT_EQ(items[1].usagePage, HIDUsagePage::Button);
    EXPECT_EQ(items[1].logicalMinimum, 0);
    EXPECT_EQ(items[1].logicalMaximum, 1);
    EXPECT_FALSE(items[1].physicalMinimum);
    EXPECT_FALSE(items[1].physicalMaximum);
    EXPECT_EQ(items[1].unit, 0);
    EXPECT_EQ(items[1].exponent, 0);
    EXPECT_EQ(items[1].reportSize, 5);
    EXPECT_EQ(items[1].reportCount, 1);
    EXPECT_TRUE(items[1].items.empty());
    EXPECT_EQ(items[2].tag, HIDTag::Input);
    EXPECT_EQ(items[2].flags, 0x06);
    EXPECT_EQ(items[2].usagePage, HIDUsagePage::GenericDesktopControls);
    EXPECT_EQ(items[2].logicalMinimum, -127);
    EXPECT_EQ(items[2].logicalMaximum, 127);
    EXPECT_FALSE(items[2].physicalMinimum);
    EXPECT_FALSE(items[2].physicalMaximum);
    EXPECT_EQ(items[2].unit, 0);
    EXPECT_EQ(items[2].exponent, 0);
    EXPECT_EQ(items[2].reportSize, 8);
    EXPECT_EQ(items[2].reportCount, 2);
    EXPECT_EQ(items[2].items, (std::vector<HIDLocalItem>{
        { HIDTag::Usage, 0x00010030 },
        { HIDTag::Usage, 0x00010031 }
    }));
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
    0x09, 0x48,        //         Usage (0x48)
    0x15, 0x00,        //         Logical Minimum (0)
    0x25, 0x01,        //         Logical Maximum (1)
    0x35, 0x01,        //         Physical Minimum (1)
    0x45, 0x04,        //         Physical Maximum (4)
    0x75, 0x02,        //         Report Size (2)
    0x95, 0x01,        //         Report Count (1)
    0xA4,              //         Push
    0xB1, 0x02,        //           Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x09, 0x38,        //           Usage (Wheel)
    0x15, 0x81,        //           Logical Minimum (-127)
    0x25, 0x7F,        //           Logical Maximum (127)
    0x35, 0x00,        //           Physical Minimum (0)
    0x45, 0x00,        //           Physical Maximum (0)
    0x75, 0x08,        //           Report Size (8)
    0x81, 0x06,        //           Input (Data,Var,Rel,No Wrap,Linear,Preferred State,No Null Position)
    0xC0,              //         End Collection
    0xA1, 0x02,        //         Collection (Logical)
    0x09, 0x48,        //           Usage (0x48)
    0xB4,              //         Pop
    0xB1, 0x02,        //         Feature (Data,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
    0x35, 0x00,        //         Physical Minimum (0)
    0x45, 0x00,        //         Physical Maximum (0)
    0x75, 0x04,        //         Report Size (4)
    0xB1, 0x03,        //         Feature (Const,Var,Abs,No Wrap,Linear,Preferred State,No Null Position,Non-volatile)
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
    auto root = libkeyleds::hid::parse(mcMouse, sizeof(mcMouse));
    EXPECT_TRUE(root);

    EXPECT_EQ(root->type, HIDCollectionType::Physical);
    EXPECT_EQ(root->usage, HIDUsage::Undefined);
    ASSERT_EQ(root->subcollections.size(), 1);
    EXPECT_TRUE(root->reports.empty());

    {
        const auto & collection = root->subcollections[0];
        EXPECT_EQ(collection.type, HIDCollectionType::Application);
        EXPECT_EQ(collection.usage, HIDUsage::Mouse);
        EXPECT_TRUE(collection.reports.empty());
        ASSERT_EQ(collection.subcollections.size(), 1);
    }
    {
        const auto & collection = root->subcollections[0].subcollections[0];
        EXPECT_EQ(collection.type, HIDCollectionType::Logical);
        EXPECT_EQ(collection.usage, HIDUsage::Mouse);
        EXPECT_TRUE(collection.reports.empty());
        ASSERT_EQ(collection.subcollections.size(), 1);
    }
    {
        const auto & collection = root->subcollections[0].subcollections[0].subcollections[0];
        EXPECT_EQ(collection.type, HIDCollectionType::Physical);
        EXPECT_EQ(collection.usage, HIDUsage::Pointer);
        ASSERT_EQ(collection.reports.size(), 3);
        ASSERT_EQ(collection.subcollections.size(), 2);
    }
}
