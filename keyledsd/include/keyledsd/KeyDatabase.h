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

#include "keyledsd/RenderTarget.h"
#include <iosfwd>
#include <iterator>
#include <string>
#include <utility>
#include <vector>

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
    using position_type = unsigned int;
    struct Rect { position_type x0, y0, x1, y1; };

    struct Key final
    {
        using index_type = RenderTarget::size_type;

        index_type      index;      ///< index in render targets
        int             keyCode;    ///< linux input event code
        std::string     name;       ///< user-readable name
        Rect            position;   ///< physical position on keyboard
    };

    class KeyGroup;

private:
    struct Relation final
    {
        position_type   distance;
    };

    using key_list = std::vector<Key>;
    using relation_list = std::vector<Relation>;
public:
    using value_type = key_list::value_type;
    using const_reference = key_list::const_reference;
    using const_iterator = key_list::const_iterator;
    using difference_type = signed int;     // narrow down vector's size
    using size_type = unsigned int;         // narrow down vector's size
public:
                    KeyDatabase() = default;
    explicit        KeyDatabase(key_list keys);
                    ~KeyDatabase();

    const_iterator  findKeyCode(int keyCode) const;
    const_iterator  findName(const char * name) const;

    const_iterator  begin() const { return m_keys.cbegin(); }
    const_iterator  end() const { return m_keys.cend(); }
    bool            empty() const noexcept { return m_keys.empty(); }
    size_type       size() const noexcept { return size_type(m_keys.size()); }
    const_reference operator[](size_type idx) const { return m_keys[idx]; }

    Rect            bounds() const noexcept { return m_bounds; }
    position_type   distance(const Key &, const Key &) const noexcept;
    double          angle(const Key &, const Key &) const noexcept;

    /// Builds a KeyGroup with given name; first and last define a sequence of
    /// string defining key names for the group. Invalid names are ignored.
    template<typename It> KeyGroup makeGroup(std::string name, It first, It last) const;
    template<typename S, typename C> KeyGroup makeGroup(S && name, const C &) const;

private:
    /// Computes m_bounds, invoked once at initialization
    static Rect computeBounds(const key_list &);
    static relation_list computeRelations(const key_list &);

private:
    key_list        m_keys;         ///< Vector of all keys known for a device
    Rect            m_bounds;       ///< Bounds of m_keys' positions
    relation_list   m_relations;    ///< Pre-computed relation array
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
    using key_list = std::vector<KeyDatabase::const_iterator>;
public:
    using value_type = Key;
    using reference = value_type &;
    using const_reference = const value_type &;
    using size_type = unsigned int;
    using difference_type = signed int;

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
        iterator &  operator+=(difference_type n) { m_it += n; return *this; }
        iterator &  operator--() { --m_it; return *this; }
        iterator    operator--(int) { return iterator(m_it--); }
        iterator &  operator-=(difference_type n) { m_it -= n; return *this; }
        reference   operator*() const { return **m_it; }
        const value_type * operator->() const { return &**m_it; }
        void        swap(iterator & o) noexcept { std::swap(m_it, o.m_it); }

    private:
        friend class KeyGroup;
        friend iterator operator+(const iterator & a, difference_type b)
            { return iterator(a.m_it + b); }
        friend iterator operator+(difference_type a, const iterator & b)
            { return iterator(a + b.m_it); }
        friend iterator operator-(const iterator & a, difference_type b)
            { return iterator(a.m_it - b); }
        friend difference_type operator-(const iterator & a, const iterator & b)
            { return a.m_it - b.m_it; }
        friend bool operator==(const iterator & a, const iterator & b) { return a.m_it == b.m_it; }
        friend bool operator!=(const iterator & a, const iterator & b) { return a.m_it != b.m_it; }
    };
    using const_iterator = iterator;
public:
                    KeyGroup() = default;
                    KeyGroup(std::string, key_list keys);
                    KeyGroup(const KeyGroup &) = default;
                    KeyGroup(KeyGroup &&) noexcept = default;
                    ~KeyGroup();
    KeyGroup &      operator=(const KeyGroup &) = default;
    KeyGroup &      operator=(KeyGroup &&) = default;

    const std::string & name() const noexcept { return m_name; }

    const_iterator  begin() const { return cbegin(); }
    const_iterator  cbegin() const { return const_iterator(m_keys.cbegin()); }
    const_iterator  end() const { return cend(); }
    const_iterator  cend() const { return const_iterator(m_keys.cend()); }
    const_reference operator[](size_type idx) const { return *m_keys[idx]; }

    bool            empty() const noexcept { return m_keys.empty(); }
    size_type       size() const noexcept { return size_type(m_keys.size()); }

    void            clear() { m_keys.clear(); }
    const_iterator  erase(const_iterator it)
                        { return const_iterator(m_keys.erase(it.m_it)); }
    const_iterator  insert(const_iterator pos, key_list::value_type it)
                        { return const_iterator(m_keys.insert(pos.m_it, it)); }
    void            push_back(key_list::value_type it) { m_keys.push_back(it); }
    void            pop_back() { m_keys.pop_back(); }

    void            shrink_to_fit() { m_keys.shrink_to_fit(); }
private:
    std::string     m_name;
    key_list        m_keys;

    friend void swap(KeyGroup & lhs, KeyGroup & rhs) noexcept;
};

/****************************************************************************/

inline bool operator==(const KeyDatabase::Rect & a, const KeyDatabase::Rect & b)
 { return a.x0 == b.x0 && a.y0 == b.y0 && a.x1 == b.x1 && a.y1 == b.y1; }
inline bool operator!=(const KeyDatabase::Rect & a, const KeyDatabase::Rect & b)
 { return !(a == b); }

std::ostream & operator<<(std::ostream &, const KeyDatabase::Key &);
inline bool operator==(const KeyDatabase::Key & a, const KeyDatabase::Key & b)
 { return a.index == b.index; }
inline bool operator!=(const KeyDatabase::Key & a, const KeyDatabase::Key & b)
 { return !(a == b); }

std::ostream & operator<<(std::ostream &, const KeyDatabase::KeyGroup &);
bool operator==(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b);
inline bool operator!=(const KeyDatabase::KeyGroup & a, const KeyDatabase::KeyGroup & b)
 { return !(a == b); }

inline void swap(KeyDatabase::KeyGroup & lhs, KeyDatabase::KeyGroup & rhs) noexcept
{
    using std::swap;
    swap(lhs.m_name, rhs.m_name);
    swap(lhs.m_keys, rhs.m_keys);
}

template<typename It>
KeyDatabase::KeyGroup KeyDatabase::makeGroup(std::string name, It first, It last) const
{
    using It_value_type = typename std::iterator_traits<It>::value_type;
    static_assert(std::is_convertible_v<It_value_type, std::string>);
    std::vector<const_iterator> result;
    for (auto it = first; it != last; ++it) {
        auto kit = findName(it->c_str());
        if (kit != end()) { result.push_back(kit); }
    }
    return KeyGroup(std::move(name), std::move(result));
}

template<typename S, typename C>
KeyDatabase::KeyGroup KeyDatabase::makeGroup(S && name, const C & container) const
{
    return makeGroup(std::forward<S>(name), std::begin(container), std::end(container));
}

/****************************************************************************/

} // namespace keyleds

#endif
