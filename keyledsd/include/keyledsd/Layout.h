#ifndef KEYLEDSD_LAYOUT_H
#define KEYLEDSD_LAYOUT_H

#include <iosfwd>
#include <string>
#include <vector>

namespace keyleds {

/****************************************************************************/

class Layout final
{
public:
    struct Rect { unsigned x0, y0, x1, y1; };
    class Key;
    class ParseError;
    typedef std::vector<Key> key_list;
public:
                        Layout(std::istream & stream) { parse(stream); }
                        Layout(std::string name, key_list && keys)
                            : m_name(name), m_keys(keys), m_bounds(computeBounds()) {}

    const std::string & name() const { return m_name; }
    const key_list &    keys() const { return m_keys; }
    const Rect &        bounds() const { return m_bounds; }

private:
    void                parse(std::istream &);
    Rect                computeBounds();

private:
    std::string         m_name;
    key_list            m_keys;
    Rect                m_bounds;
};

/****************************************************************************/

class Layout::Key final {
public:
    typedef unsigned int block_type;
    typedef unsigned int code_type;
public:
                Key(block_type block, code_type code, Rect position, std::string name)
                    : block(block), code(code), position(position), name(name) {}
public:
    block_type  block;
    code_type   code;
    Rect        position;
    std::string name;
};

/****************************************************************************/

class Layout::ParseError : public std::runtime_error
{
public:
                        ParseError(const std::string & what, int line)
                            : std::runtime_error(what), m_line(line) {}
    int         line() const { return m_line; }
private:
    int         m_line;
};

/****************************************************************************/

};

#endif
