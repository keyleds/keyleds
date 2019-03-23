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

static Usage combine(UsagePage page, uint32_t usage)
{
    if ((usage & uint32_t(Usage::PageMask)) == 0) {
        usage |= uint32_t(page) << 16;
    }
    return Usage{usage};
}

/****************************************************************************/

/// HID report item type
enum class Type : uint8_t {
    Main = 0,                   ///< create a data field
    Global = 1,                 ///< manipulate persistent field state
    Local = 2,                  ///< add items to next data field
    Reserved = 3
};

/// Decoded item byte
struct Item
{
    Type            type;
    Tag             tag;
    const uint8_t * data;       ///< points past the item byte
    unsigned        size;       ///< how many bytes in data

    template <typename T> T value() const { return item_parser<T>::deserialize(data, size); }
};

bool isGlobal(const Item & item) { return item.type == Type::Global; }
bool isLocal(const Item & item) { return item.type == Type::Local; }

/****************************************************************************/

class ReportParser final
{
    using Collection = ReportDescriptor::Collection;
    using Field = ReportDescriptor::Field;
    using Report = ReportDescriptor::Report;
public:
    bool parse(const uint8_t * data, std::size_t length)
    {
        const uint8_t * ptr = data;
        const uint8_t * const endptr = data + length;

        while (ptr < endptr && !m_error) {
            if (*ptr == 0xfe) { // Long item
                if (ptr + 2 > endptr) { return false; }
                ptr += 3 + *(ptr + 1);

            } else {            // Short item
                // Decode item prefix
                auto item = Item{
                    Type((*ptr & 0x0c) >> 2),
                    Tag(*ptr & 0xfc),
                    ptr + 1,
                    unsigned(*ptr & 0x03)
                };
                if (item.size == 3) { item.size = 4; }

                // Check for overflow
                if (ptr + 1 + item.size > endptr) { return false; }

                switch (item.type) {
                    case Type::Main:    mainItem(item); break;
                    case Type::Global:  globalItem(item); break;
                    case Type::Local:   m_state.push_back(item); break;
                    default: break;
                }

                ptr += 1 + item.size;
            }
        }

        if (m_currentCollection != ReportDescriptor::no_collection) {
            m_error = "missing endCollection item";
        }
        return !m_error;
    }

    ReportDescriptor result() {
        return { std::move(m_collections), std::move(m_reports) };
    }

private:
    void mainItem(Item item)
    {
        switch (item.tag) {
        case Tag::Input:            [[fallthrough]];
        case Tag::Output:           [[fallthrough]];
        case Tag::Feature:          dataField(item); break;
        case Tag::Collection:       beginCollection(item); break;
        case Tag::EndCollection:    endCollection(); break;
        default: break;
        }
        m_state.erase(std::remove_if(m_state.begin(), m_state.end(), isLocal), m_state.end());
    }

    void beginCollection(Item item)
    {
        const auto idx = ReportDescriptor::collection_index(m_collections.size());
        m_collections.push_back({
            m_currentCollection,
            item.value<CollectionType>(),
            nextUsage(),
            {}
        });
        if (m_currentCollection != ReportDescriptor::no_collection) {
            m_collections[m_currentCollection].children.push_back(idx);
        }
        m_currentCollection = idx;
    }

    void endCollection()
    {
        if (m_currentCollection == ReportDescriptor::no_collection) {
            m_error = "unexpected endCollection item";
            return;
        }
        m_currentCollection = m_collections[m_currentCollection].parent;
    }

    void dataField(Item main)
    {
        auto && [reportId, field] = aggregateFieldItems(main);
        field.collectionIdx = m_currentCollection;

        auto rit = std::find_if(m_reports.begin(), m_reports.end(),
                                [id=reportId](const auto & report) { return report.id == id; });
        if (rit == m_reports.end()) {
            m_reports.push_back({reportId, {}});
            rit = m_reports.end() - 1;
        }

        rit->fields.push_back(std::move(field));
    }

    void globalItem(Item item)
    {
        switch (item.tag) {
        case Tag::Push: pushState(); return;
        case Tag::Pop:  popState(); return;
        default: break;
        }
        m_state.push_back(item);
    }

private:
    std::pair<uint8_t, Field> aggregateFieldItems(Item main)
    {
        auto result = std::pair<uint8_t, Field>{0, {}};
        auto & [reportId, field] = result;

        field.collectionIdx = ReportDescriptor::no_collection;
        field.tag = main.tag;
        field.flags = main.value<uint32_t>();

        for (const auto & item : m_state) {
            switch (item.tag) {
                case Tag::UsagePage:        field.usagePage = item.value<UsagePage>(); break;
                case Tag::LogicalMinimum:   field.logicalMinimum = item.value<int32_t>(); break;
                case Tag::LogicalMaximum:   field.logicalMaximum = item.value<int32_t>(); break;
                case Tag::PhysicalMinimum:  field.physicalMinimum = item.value<int32_t>(); break;
                case Tag::PhysicalMaximum:  field.physicalMaximum = item.value<int32_t>(); break;
                case Tag::Unit:             field.unit = item.value<uint32_t>(); break;
                case Tag::ReportSize:       field.reportSize = item.value<uint8_t>(); break;
                case Tag::ReportCount:      field.reportCount = item.value<uint8_t>(); break;
                case Tag::ReportId:         reportId = item.value<uint8_t>(); break;
                case Tag::UnitExponent: {
                    auto val = item.value<uint8_t>();
                    field.exponent = val < 8 ? signed(val) : signed(val-16);
                    break;
                }
                case Tag::Usage:
                    field.items.push_back({
                        Tag::Usage,
                        uint32_t(combine(field.usagePage, item.value<uint32_t>()))
                    });
                    break;
                default:
                    field.items.push_back({ item.tag, item.value<uint32_t>() });
                    break;
            }
        }
        return result;
    }

    Usage nextUsage()
    {
        auto page = UsagePage::Undefined;
        for (auto && it = m_state.begin(); it != m_state.end(); ++it) {
            if (it->tag == Tag::UsagePage) { page = it->value<UsagePage>(); }
            if (it->tag == Tag::Usage) {
                auto usage = it->value<uint32_t>();
                m_state.erase(it);
                return combine(page, usage);
            }
        }
        return combine(page, 0);
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
        if (m_stateStack.empty()) {
            m_error = "unexpected Pop item";
            return;
        }
        m_state = std::move(m_stateStack.top());
        m_stateStack.pop();
    }

private:
    std::vector<Collection>         m_collections;      ///< collections in descriptor
    std::vector<Report>             m_reports;          ///< reports in descriptor
    unsigned                        m_currentCollection = ReportDescriptor::no_collection;
                                                        ///< index into m_collections
    std::vector<Item>               m_state;            ///< both local and global items for next field
    std::stack<std::vector<Item>>   m_stateStack;       ///< capture of m_state on PUSH/POP directives
    const char *                    m_error = nullptr;
};

} // namespace

KEYLEDS_EXPORT std::optional<ReportDescriptor> parse(const uint8_t * data, std::size_t length)
{
    ReportParser parser;
    if (!parser.parse(data, length)) { return std::nullopt; }
    return parser.result();
}

/****************************************************************************/

} // namespace libkeyleds::hid
