#ifndef KEYLEDSD_COMMON_H
#define KEYLEDSD_COMMON_H

#include <stdint.h>
#include <iosfwd>

namespace keyleds {

struct RGBColor {
    typedef uint8_t channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;

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


struct RGBAColor {
    typedef uint8_t channel_type;

    channel_type red;
    channel_type green;
    channel_type blue;
    channel_type alpha;

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

};

#endif
