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
#include <algorithm>
#include <ostream>
#include "keyledsd/Context.h"

using keyleds::Context;

Context::Context(std::initializer_list<value_map::value_type> init)
 : m_values(init)
{}

Context::Context(value_map values)
 : m_values(std::move(values))
{}

void Context::merge(const Context & other)
{
    for (auto & oitem : other) {
        auto it = std::find_if(
            m_values.begin(), m_values.end(),
            [&oitem](const auto & item) { return item.first == oitem.first; }
        );
        if (it != m_values.end()) {
            // Key exists
            if (oitem.second.empty()) {
                // Value is empty, destroy key
                std::iter_swap(it, m_values.end() - 1);
                m_values.pop_back();
            } else {
                // Update with new value
                it->second = oitem.second;
            }
        } else if (!oitem.second.empty()) {
            // Create key
            m_values.emplace_back(oitem.first, oitem.second);
        }
    }
}

const std::string & Context::operator[](const std::string & key) const
{
    static const std::string empty;
    auto it = std::find_if(m_values.begin(), m_values.end(),
                           [&key](const auto & item) { return item.first == key; });
    if (it == m_values.end()) { return empty; }
    return it->second;
}

void Context::print(std::ostream & out) const
{
    bool first = true;
    out <<'(';
    for (const auto & entry : m_values) {
        if (!first) { out <<", "; }
        first = false;
        out <<entry.first <<'=' <<entry.second;
    }
    out <<')';
}
