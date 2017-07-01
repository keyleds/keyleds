#ifndef KEYLEDSD_COMMON_H
#define KEYLEDSD_COMMON_H

#include <iosfwd>
#include <limits>

namespace keyleds {

/****************************************************************************/

struct RGBColor final {
    typedef unsigned char channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;

    RGBColor() = default;
    RGBColor(channel_type r, channel_type g, channel_type b)
     : red(r), green(g), blue(b) {}

    static RGBColor parse(const std::string &);
    void print(std::ostream &) const;
};

static inline bool operator==(const RGBColor & a, const RGBColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBColor & a, const RGBColor & b) { return !(a == b); }

static inline std::ostream & operator<<(std::ostream & out, const RGBColor & obj)
{
    obj.print(out);
    return out;
}

/****************************************************************************/

struct RGBAColor final {
    typedef unsigned char channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;
    channel_type alpha;

    RGBAColor() = default;
    RGBAColor(channel_type r, channel_type g, channel_type b, channel_type a)
     : red(r), green(g), blue(b), alpha(a) {}
    explicit RGBAColor(const RGBColor & c, channel_type a = std::numeric_limits<channel_type>::max())
     : red(c.red), green(c.green), blue(c.blue), alpha(a) {}

    static RGBAColor parse(const std::string &);
    void print(std::ostream &) const;
};

static inline bool operator==(const RGBAColor & a, const RGBAColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBAColor & a, const RGBAColor & b) { return !(a == b); }

static inline std::ostream & operator<<(std::ostream & out, const RGBAColor & obj)
{
    obj.print(out);
    return out;
}

/****************************************************************************/

};

#endif
