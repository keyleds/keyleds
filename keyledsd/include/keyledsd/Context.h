#ifndef KEYLEDSD_CONTEXT_H
#define KEYLEDSD_CONTEXT_H

#include <functional>
#include <string>
#include <unordered_map>
#include <vector>

namespace keyleds {

class Context;
static bool operator==(const Context &, const Context &);
static bool operator!=(const Context &, const Context &);

class Context final
{
public:
    typedef std::unordered_map<std::string, std::string> value_map;
public:
    Context() {}
    Context(std::initializer_list<value_map::value_type> init)
     : m_values(init) {}
    explicit Context(value_map values)
     : m_values(values) {}

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
