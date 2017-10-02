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
#ifndef KEYLEDSD_KEYDATABASE_H_E8A1B5AF
#define KEYLEDSD_KEYDATABASE_H_E8A1B5AF

#include <string>
#include <utility>
#include <vector>
#include "keyledsd/device/RenderLoop.h"

namespace keyleds {

/****************************************************************************/

/** Device key database
 *
 * Holds compiled information about all recognised keys on an active device.
 * It guarantees iterators and pointers to individual keys will remain valid
 * throughout its lifetime.
 */
class KeyDatabase final
{
public:
    class Key final
    {
    public:
        struct Rect { unsigned x0, y0, x1, y1; };
    public:
        RenderTarget::size_type index;      ///< index in render targets
        int                     keyCode;    ///< linux input event code
        std::string             name;       ///< user-readable name
        Rect                    position;   ///< physical position on keyboard
    };
    class KeyGroup;

    using key_list = std::vector<Key>;
    using value_type = key_list::value_type;
    using reference = key_list::const_reference;
    using iterator = key_list::const_iterator;
    using const_iterator = key_list::const_iterator;
    using difference_type = key_list::difference_type;
    using size_type = key_list::size_type;
public:
                    KeyDatabase(key_list keys);
    explicit        KeyDatabase(const KeyDatabase &) = default;
                    KeyDatabase(KeyDatabase &&) = default;

    const_iterator  findIndex(RenderTarget::size_type) const;
    const_iterator  findKeyCode(int keyCode) const;
    const_iterator  findName(const std::string & name) const;

    const_iterator  begin() const { return m_keys.cbegin(); }
    const_iterator  end() const { return m_keys.cend(); }
    const Key &     operator[](int idx) const { return m_keys[idx]; }
    size_type       size() const noexcept { return m_keys.size(); }

    Key::Rect       bounds() const { return m_bounds; }

    /// Builds a KeyGroup with given name; first and last define a sequence of
    /// string defining key names for the group. Invalid names are ignored.
    template<typename It> KeyGroup makeGroup(std::string name, It first, It last) const;

private:
    /// Computes m_bounds, invoked once at initialization
    static Key::Rect computeBounds(const key_list &);

private:
    const key_list  m_keys;     ///< Vector of all keys known for a device
    const Key::Rect m_bounds;   ///< Bounds of m_keys' positions
};

/****************************************************************************/

/** Key Group abstraction
 *
 * Verbose interface as this is mostly used in plugins, so some convenience
 * is welcome. It behaves as a vector of const objects, actually referencing
 * into a KeyDatabase object.
 *
 * Moving or destroying the KeyDatabase the KeyGroup's keys live in invalidates
 * the KeyGroup.
 */
class KeyDatabase::KeyGroup final
{
    using key_list = std::vector<KeyDatabase::iterator>;
public:
    using value_type = KeyDatabase::iterator;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = std::size_t;
    using difference_type = std::ptrdiff_t;

    class iterator : public std::iterator<std::bidirectional_iterator_tag,
                                          const KeyDatabase::Key>
    {
        key_list::const_iterator m_it;
    public:
                    iterator() = default;
        explicit    iterator(key_list::const_iterator it) : m_it(it) {}
                    iterator(const iterator &) = default;
        iterator &  operator=(const iterator &) = default;
        iterator &  operator++() { ++m_it; return *this; }
        iterator    operator++(int) { return iterator(m_it++); }
        iterator &  operator--() { --m_it; return *this; }
        iterator    operator--(int) { return iterator(m_it--); }
        reference   operator*() const { return **m_it; }
        void        swap(iterator & o) noexcept { std::swap(m_it, o.m_it); }

        key_list::const_iterator get() const { return m_it; }
        friend bool operator==(const iterator &, const iterator &);
    };
    using const_iterator = iterator;
public:
                    KeyGroup() = default;
                    KeyGroup(std::string, key_list keys);
                    KeyGroup(const KeyGroup &) = default;
                    KeyGroup(KeyGroup &&) noexcept = default;
    KeyGroup &      operator=(const KeyGroup &) = default;
    KeyGroup &      operator=(KeyGroup &&) = default;

    const std::string & name() const noexcept { return m_name; }

    const_iterator  begin() const { return cbegin(); }
    const_iterator  cbegin() const { return const_iterator(m_keys.cbegin()); }
    const_iterator  end() const { return cend(); }
    const_iterator  cend() const { return const_iterator(m_keys.cend()); }
    const Key &     operator[](int idx) const { return *m_keys[idx]; }

    bool            empty() const noexcept { return m_keys.empty(); }
    size_type       size() const noexcept { return m_keys.size(); }
    size_type       max_size() const { return m_keys.max_size(); }

    void            clear() { m_keys.clear(); }
    const_iterator  erase(const_iterator it)
                        { return const_iterator(m_keys.erase(it.get())); }
    const_iterator  insert(const_iterator pos, KeyDatabase::iterator it)
                        { return const_iterator(m_keys.insert(pos.get(), it)); }
    void            push_back(KeyDatabase::iterator it) { m_keys.push_back(it); }
    void            pop_back() { m_keys.pop_back(); }

    void            shrink_to_fit() { m_keys.shrink_to_fit(); }
    void            swap(KeyGroup &) noexcept;
private:
    std::string     m_name;
    key_list        m_keys;
};

/****************************************************************************/

inline bool operator==(const KeyDatabase::Key & a, const KeyDatabase::Key & b)
 { return a.index == b.index; }
inline bool operator!=(const KeyDatabase::Key & a, const KeyDatabase::Key & b)
 { return !(a == b); }

bool operator==(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b);
inline bool operator!=(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b)
 { return !(a == b); }

inline bool operator==(const KeyDatabase::KeyGroup::iterator & a,
                       const KeyDatabase::KeyGroup::iterator & b)
 { return a.m_it == b.m_it; }
inline bool operator!=(const KeyDatabase::KeyGroup::iterator & a,
                       const KeyDatabase::KeyGroup::iterator & b)
 { return !(a == b); }

template<typename It>
KeyDatabase::KeyGroup KeyDatabase::makeGroup(std::string name, It first, It last) const
{
    std::vector<iterator> result;
    for (auto it = first; it != last; ++it) {
        auto kit = findName(*it);
        if (kit != end()) { result.push_back(kit); }
    }
    return KeyGroup(std::move(name), std::move(result));
}

/****************************************************************************/

}

#endif
