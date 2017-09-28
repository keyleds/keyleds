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
#ifndef KEYLEDSD_CONTEXT_H_2CFCE45E
#define KEYLEDSD_CONTEXT_H_2CFCE45E

#include <initializer_list>
#include <iosfwd>
#include <string>
#include <utility>
#include <vector>

namespace keyleds {

class Context;
static bool operator==(const Context &, const Context &);
static bool operator!=(const Context &, const Context &);

/** Current service running context
 *
 * Represents the full environment state as watched by one (or more) context
 * watchers. Context is the primary source of configuration and runtime selection
 * of plugins by the service. As such, context changes are the main events
 * it deals with. A context takes the form of a set of key-value pairs.
 */
class Context final
{
public:
    using value_map = std::vector<std::pair<std::string, std::string>>;
public:
    Context() = default;
    Context(std::initializer_list<value_map::value_type> init);
    explicit Context(value_map values);

    void merge(const Context &);

    const std::string & operator[](const std::string & key) const;

    value_map::const_iterator begin() const noexcept { return m_values.cbegin(); }
    value_map::const_iterator end() const noexcept { return m_values.cend(); }

    void             print(std::ostream &) const;

private:
    value_map        m_values;

    friend bool operator==(const Context &, const Context &);
    friend bool operator!=(const Context &, const Context &);
};

static inline std::ostream & operator<<(std::ostream & out, const Context & ctx) { ctx.print(out); return out; }
static inline bool operator==(const Context & lhs, const Context & rhs) { return lhs.m_values == rhs.m_values; }
static inline bool operator!=(const Context & lhs, const Context & rhs) { return lhs.m_values != rhs.m_values; }

};

#endif
