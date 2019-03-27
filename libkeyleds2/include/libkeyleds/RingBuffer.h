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
#ifndef LIBKEYLEDS_RINGBUFFER_H_289A3A05
#define LIBKEYLEDS_RINGBUFFER_H_289A3A05

#include <new>
#include <type_traits>


namespace libkeyleds {

template <typename T, std::size_t Slots>
class RingBuffer final
{
    using slot_type = typename std::aligned_storage<sizeof(T), alignof(T)>::type;
public:
    using value_type = T;
    using size_type = std::size_t;
    using reference = T &;
    using const_reference = T const &;

public:
    RingBuffer() = default;
    ~RingBuffer() noexcept(noexcept(clear()))
    {
        this->clear();
    }

    RingBuffer(const RingBuffer &) = delete;
    RingBuffer & operator=(const RingBuffer &) = delete;

#ifdef __cpp_lib_launder
    reference       front() { return *std::launder(reinterpret_cast<T*>(m_read)); }
    const_reference front() const { return *std::launder(reinterpret_cast<const T*>(m_read)); };
#else
    reference       front() { return *reinterpret_cast<T*>(m_read); }
    const_reference front() const { return *reinterpret_cast<const T*>(m_read); };
#endif

    template <typename U = T>
    std::enable_if_t<std::is_copy_constructible_v<U>, void>
    push(const T & val) noexcept(std::is_nothrow_copy_constructible_v<U>)
    {
        new (m_write) T(val);
        if (++m_write >= storageEnd()) { m_write = storageBegin(); }
        if (m_write == m_read) { m_write = nullptr; }
    }

    template <typename U = T>
    std::enable_if_t<std::is_move_constructible_v<U>, void>
    push(T && val) noexcept(std::is_nothrow_move_constructible_v<U>)
    {
        new (m_write) T(std::move(val));
        if (++m_write >= storageEnd()) { m_write = storageBegin(); }
        if (m_write == m_read) { m_write = nullptr; }
    }

    template<typename ... Args>
    void emplace(Args && ... args) noexcept(std::is_nothrow_constructible_v<T, Args...>)
    {
        new (m_write) T(std::forward<Args>(args)...);
        if (++m_write >= storageEnd()) { m_write = storageBegin(); }
        if (m_write == m_read) { m_write = nullptr; }
    }

    void pop() noexcept(std::is_nothrow_destructible_v<T>)
    {
#ifdef __cpp_lib_launder
        std::launder(reinterpret_cast<T*>(m_read))->~T();
#else
        reinterpret_cast<T*>(m_read)->~T();
#endif
        if (!m_write) { m_write = m_read; }
        if (++m_read >= storageEnd()) { m_read = storageBegin(); }
    }

    void clear() noexcept(std::is_nothrow_destructible_v<T>)
    {
        if constexpr (std::is_trivially_destructible_v<T>) {
            m_write = m_read;
        } else {
            while (!this->empty()) { this->pop(); }
        }
    }

    bool empty() const noexcept
    {
        return m_read == m_write;
    }

    size_type size() const noexcept
    {
        return m_write == nullptr ? Slots
             : m_read <= m_write  ? size_type(m_write - m_read)
                                  : size_type(storageEnd() - m_read) +
                                    size_type(m_write - storageBegin());
    }

    size_type capacity() const noexcept { return Slots; }
    size_type max_size() const noexcept { return Slots; }

private:
    slot_type *         storageBegin() { return &m_storage[0]; }
    const slot_type *   storageBegin() const { return &m_storage[0]; }
    slot_type *         storageEnd() { return storageBegin() + Slots; }
    const slot_type *   storageEnd() const { return storageBegin() + Slots; }
private:
    slot_type   m_storage[Slots];
    slot_type * m_read = &m_storage[0];
    slot_type * m_write = &m_storage[0];    ///< nullptr when buffer is full
};


} // namespace libkeyleds

#endif
