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
#include <stdint.h>
#include <stdlib.h>
#include "tools/accelerated.h"
#include "config.h"

void blend_plain(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);
void blend_mmx(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);
void blend_sse2(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);

static void (*resolve_blend(void))(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
#if defined __GNUC__ && !defined __clang__
    __builtin_cpu_init();
#endif
#ifdef KEYLEDSD_USE_SSE2
    if (__builtin_cpu_supports("sse2")) { return blend_sse2; }
#endif
#ifdef KEYLEDSD_USE_MMX
    if (__builtin_cpu_supports("mmx")) { return blend_mmx; }
#endif
    return blend_plain;
}


#if defined __GNUC__ && (!defined __clang__ || __has_attribute(ifunc))
void blend(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
    __attribute__((ifunc("resolve_blend")));
#elif defined __clang__
static void (*resolved_blend)(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);
void blend(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    if (resolved_blend == NULL) { resolved_blend = resolve_blend(); }
    (*resolved_blend)(dst, src, length);
}
#else
#error Runtime CPU detection no implemented for this compiler
#endif
