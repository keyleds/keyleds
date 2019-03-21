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

#include "config.h"
#include <algorithm>
#include <stack>
#include <type_traits>

namespace libkeyleds::hid {

namespace {

/****************************************************************************/
// HID report short item value parsing
// Values are in LSB order - take special care with sign extension

template <typename T, typename = void> struct item_parser {};

template <typename T>   // Unsigned integers
struct item_parser<T, std::enable_if_t<std::is_unsigned_v<T>>> {
    static T deserialize(const uint8_t * data, unsigned size)
    {
        T val = 0;
        for (std::size_t idx = 0; idx < sizeof(T); ++idx) {
            if (size > idx) { val |= T(data[idx] << (8*idx)); }
        }
        return val;
    }
};

template <typename T>   // Signed integers
struct item_parser<T, std::enable_if_t<std::is_signed_v<T>>> {
    static T deserialize(const uint8_t * data, unsigned size)
    {
        using ST = std::make_unsigned_t<T>;
        if (size == 0) { return 0; }
        ST val = item_parser<ST>::deserialize(data, size);
        ST sign = 1u << (8 * size - 1);
        return static_cast<T>((val ^ sign) - sign);
    }
};

template <typename T>   // Extract enums using their underlying type
struct item_parser<T, std::enable_if_t<std::is_enum_v<T>>> {
    static T deserialize(const uint8_t * data, unsigned size) {
        return T { item_parser<std::underlying_type_t<T>>::deserialize(data, size)};
    }
};

/****************************************************************************/

/// HID report item type
enum class HIDType : uint8_t {
    Main = 0,                   ///< create a data field
    Global = 1,                 ///< manipulate persistent field state
    Local = 2,                  ///< add items to next data field
    Reserved = 3
};

/// Decoded item byte
struct Item
{
    HIDType         type;
    HIDTag          tag;
    const uint8_t * data;       ///< points past the item byte
    unsigned        size;       ///< how many bytes in data

    template <typename T> T value() const { return item_parser<T>::deserialize(data, size); }
};

bool isGlobal(const Item & item) { return item.type == HIDType::Global; }
bool isLocal(const Item & item) { return item.type == HIDType::Local; }

/****************************************************************************/

class HIDReportParser final
{
public:
    HIDReportParser()
     : m_root{HIDCollectionType::Physical, HIDUsage::Undefined, {}, {}},
       m_collectionStack({ &m_root })
     {}

    bool parse(const uint8_t * data, std::size_t length)
    {
        const uint8_t * ptr = data;
        const uint8_t * const endptr = data + length;

        while (ptr < endptr) {
            if (*ptr == 0xfe) { // Long item
                if (ptr + 2 > endptr) { return false; }
                ptr += 3 + *(ptr + 1);

            } else {            // Short item
                // Decode item prefix
                auto item = Item{
                    HIDType((*ptr & 0x0c) >> 2),
                    HIDTag(*ptr & 0xfc),
                    ptr + 1,
                    unsigned(*ptr & 0x03)
                };
                if (item.size == 3) { item.size = 4; }

                // Check for overflow
                if (ptr + 1 + item.size > endptr) { return false; }

                switch (item.type) {
                    case HIDType::Main:     mainItem(item); break;
                    case HIDType::Global:   globalItem(item); break;
                    case HIDType::Local:    m_state.push_back(item); break;
                    default: break;
                }

                ptr += 1 + item.size;
            }
        }

        if (m_collectionStack.size() != 1) { return false; }
        return true;
    }

    HIDCollection && result() { return std::move(m_root); }

private:
    void mainItem(Item item)
    {
        switch (item.tag) {
        case HIDTag::Input:         [[fallthrough]]
        case HIDTag::Output:        [[fallthrough]]
        case HIDTag::Feature:       dataField(item); break;
        case HIDTag::Collection:    beginCollection(item); break;
        case HIDTag::EndCollection: endCollection(); break;
        default: break;
        }
        m_state.erase(std::remove_if(m_state.begin(), m_state.end(), isLocal), m_state.end());
    }

    void beginCollection(Item item)
    {
        m_collectionStack.top()->subcollections.push_back({
            item.value<HIDCollectionType>(),
            nextUsage(),
            {}, {}
        });
        m_collectionStack.push(&m_collectionStack.top()->subcollections.back());
    }

    void endCollection()
    {
        m_collectionStack.pop();
    }

    void dataField(Item main)
    {
        HIDMainItem field = {
            main.tag,
            main.value<uint32_t>(),
            HIDUsagePage::Undefined,
            0, 0,
            std::nullopt, std::nullopt,
            0, 0,
            0, 0,
            {}
        };
        uint8_t reportId = 0;

        for (const auto & item : m_state) {
            switch (item.tag) {
                case HIDTag::UsagePage:         field.usagePage = item.value<HIDUsagePage>(); break;
                case HIDTag::LogicalMinimum:    field.logicalMinimum = item.value<int32_t>(); break;
                case HIDTag::LogicalMaximum:    field.logicalMaximum = item.value<int32_t>(); break;
                case HIDTag::PhysicalMinimum:   field.physicalMinimum = item.value<int32_t>(); break;
                case HIDTag::PhysicalMaximum:   field.physicalMaximum = item.value<int32_t>(); break;
                case HIDTag::Unit:              field.unit = item.value<uint32_t>(); break;
                case HIDTag::ReportSize:        field.reportSize = item.value<uint8_t>(); break;
                case HIDTag::ReportCount:       field.reportCount = item.value<uint8_t>(); break;
                case HIDTag::ReportId:          reportId = item.value<uint8_t>(); break;
                case HIDTag::UnitExponent: {
                    auto val = item.value<uint8_t>();
                    field.exponent = val < 8 ? signed(val) : signed(val-16);
                    break;
                }
                case HIDTag::Usage: {
                    auto usage = item.value<uint32_t>();
                    if ((usage & uint32_t(HIDUsage::PageMask)) == 0) {
                        usage |= uint32_t(field.usagePage) << 16;
                    }
                    field.items.push_back({ HIDTag::Usage, usage });
                    break;
                }
                default:
                    field.items.push_back({ item.tag, item.value<uint32_t>() });
                    break;
            }
        }

        auto & collection = *m_collectionStack.top();
        auto it = std::find_if(collection.reports.begin(), collection.reports.end(),
                               [reportId](const auto & report) { return report.id == reportId; });
        if (it != collection.reports.end()) {
            it->items.push_back(std::move(field));
        } else {
            collection.reports.push_back({
                reportId,
                { std::move(field) }
            });
        }
    }

    void globalItem(Item item)
    {
        switch (item.tag) {
        case HIDTag::Push:  pushState(); return;
        case HIDTag::Pop:   popState(); return;
        default: break;
        }
        m_state.push_back(item);
    }

private:
    HIDUsage nextUsage()
    {
        auto page = HIDUsagePage::Undefined;
        for (auto && it = m_state.begin(); it != m_state.end(); ++it) {
            if (it->tag == HIDTag::UsagePage) { page = it->value<HIDUsagePage>(); }
            if (it->tag == HIDTag::Usage) {
                auto usage = it->value<uint32_t>();
                if ((usage & uint32_t(HIDUsage::PageMask)) == 0) {
                    usage |= uint32_t(page) << 16;
                }
                m_state.erase(it);
                return HIDUsage{usage};
            }
        }
        return HIDUsage::Undefined;
    }

    void pushState()
    {
        auto state = std::vector<Item>();
        state.reserve(m_state.size());
        std::copy_if(m_state.begin(), m_state.end(), std::back_inserter(state), isGlobal);
        m_stateStack.emplace(std::move(state));
    }

    void popState()
    {
        m_state = std::move(m_stateStack.top());
        m_stateStack.pop();
    }

private:
    HIDCollection                   m_root;             ///< implicit top-level collection
    std::stack<HIDCollection *>     m_collectionStack;  ///< path to collection being parsed
    std::vector<Item>               m_state;            ///< both local and global items for next field
    std::stack<std::vector<Item>>   m_stateStack;       ///< capture of m_state on PUSH/POP directives
};

} // namespace

KEYLEDS_EXPORT std::optional<HIDCollection> parse(const uint8_t * data, std::size_t length)
{
    HIDReportParser parser;
    if (!parser.parse(data, length)) { return std::nullopt; }
    return parser.result();
}

/****************************************************************************/

} // namespace libkeyleds::hid
