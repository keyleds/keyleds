#ifndef KEYLEDSD_COMMON_H
#define KEYLEDSD_COMMON_H

#include <stdint.h>

namespace keyleds {

struct RGBColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
};

static inline bool operator==(const RGBColor & a, const RGBColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBColor & a, const RGBColor & b) { return !(a == b); }


struct RGBAColor {
    uint8_t red;
    uint8_t green;
    uint8_t blue;
    uint8_t alpha;
};

static inline bool operator==(const RGBAColor & a, const RGBAColor & b) {
    return (a.red == b.red &&
            a.green == b.green &&
            a.blue == b.blue);
}
static inline bool operator!=(const RGBAColor & a, const RGBAColor & b) { return !(a == b); }

};

#endif
