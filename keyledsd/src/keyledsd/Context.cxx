#include <ostream>
#include "keyledsd/Context.h"

using keyleds::Context;

const std::string & Context::operator[](const std::string & key) const
{
    static const std::string empty;
    auto it = m_values.find(key);
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
