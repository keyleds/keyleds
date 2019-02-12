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
#include <assert.h>
#include <stdint.h>
#include "keyledsd/accelerated.h"
#include "config.h"


KEYLEDSD_EXPORT void blend_plain(uint8_t * restrict a, const uint8_t * restrict b, unsigned length)
{
    assert((uintptr_t)a % 8 == 0);    // Not a requirement, but lets compiler optimize stuff
    assert((uintptr_t)b % 8 == 0);    // Not a requirement, but lets compiler optimize stuff
    assert(length != 0);              // allows inverting loop condition

    a = (uint8_t * restrict)__builtin_assume_aligned(a, 8);
    b = (const uint8_t * restrict)__builtin_assume_aligned(b, 8);

    do {
        uint16_t alpha = b[3];
        if (alpha != 0) { alpha += 1; }
        a[0] = ((uint16_t)a[0] * ((uint16_t)256 - alpha) + (uint16_t)b[0] * alpha) / 256;
        a[1] = ((uint16_t)a[1] * ((uint16_t)256 - alpha) + (uint16_t)b[1] * alpha) / 256;
        a[2] = ((uint16_t)a[2] * ((uint16_t)256 - alpha) + (uint16_t)b[2] * alpha) / 256;
        a[3] = ((uint16_t)a[3] * ((uint16_t)256 - alpha) + (uint16_t)b[3] * alpha) / 256;
        a += 4;
        b += 4;
    } while (--length > 0);
}

KEYLEDSD_EXPORT void multiply_plain(uint8_t * restrict a, const uint8_t * restrict b, unsigned length)
{
    assert((uintptr_t)a % 8 == 0);    // Not a requirement, but lets compiler optimize stuff
    assert((uintptr_t)b % 8 == 0);    // Not a requirement, but lets compiler optimize stuff
    assert(length != 0);              // allows inverting loop condition

    a = (uint8_t * restrict)__builtin_assume_aligned(a, 8);
    b = (const uint8_t * restrict)__builtin_assume_aligned(b, 8);

    do {
        a[0] = ((uint16_t)a[0] * ((uint16_t)b[0] + 1)) / 256;
        a[1] = ((uint16_t)a[1] * ((uint16_t)b[1] + 1)) / 256;
        a[2] = ((uint16_t)a[2] * ((uint16_t)b[2] + 1)) / 256;
        a[3] = ((uint16_t)a[3] * ((uint16_t)b[3] + 1)) / 256;
        a += 4;
        b += 4;
    } while (--length > 0);
}
