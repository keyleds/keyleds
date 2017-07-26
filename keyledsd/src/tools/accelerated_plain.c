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
