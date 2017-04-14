#include <assert.h>
#include <stdint.h>
#include "config.h"

void blend_plain(uint8_t * __restrict a, const uint8_t * __restrict b, unsigned length)
{
    a = (uint8_t*)__builtin_assume_aligned(a, 16);
    b = (const uint8_t*)__builtin_assume_aligned(b, 16);

    assert((uintptr_t)a % 16 == 0);
    assert((uintptr_t)b % 16 == 0);
    assert(length % 16 == 0);

    while (length-- > 0) {
        uint16_t alpha = b[3];
        if (alpha != 0) { alpha += 1; }
        a[0] = ((uint16_t)a[0] * ((uint16_t)256 - alpha) + (uint16_t)b[0] * alpha) / 256;
        a[1] = ((uint16_t)a[1] * ((uint16_t)256 - alpha) + (uint16_t)b[1] * alpha) / 256;
        a[2] = ((uint16_t)a[2] * ((uint16_t)256 - alpha) + (uint16_t)b[2] * alpha) / 256;
        a += 4;
        b += 4;
    }
}
