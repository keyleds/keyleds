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
#include "keyledsd/accelerated.h"
#include "config.h"

/****************************************************************************/
/* blend */

#ifdef HAVE_BUILTIN_CPU_SUPPORTS
static void (*resolve_blend(void))(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
#  if defined __GNUC__ && !defined __clang__
    __builtin_cpu_init();
#  endif
#  ifdef KEYLEDSD_USE_AVX2
    if (__builtin_cpu_supports("avx2")) { return blend_avx2; }
#  endif
#  ifdef KEYLEDSD_USE_SSE2
    if (__builtin_cpu_supports("sse2")) { return blend_sse2; }
#  endif
    return blend_plain;
}

#  ifdef HAVE_IFUNC_ATTRIBUTE
void blend(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
    __attribute__((ifunc("resolve_blend")));
#  else
static void (*resolved_blend)(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);
void blend(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    if (resolved_blend == 0) { resolved_blend = resolve_blend(); }
    (*resolved_blend)(dst, src, length);
}
#  endif
#else
void blend(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
    { blend_plain(dst, src, length); }
#endif

/****************************************************************************/
/* multiply */

#ifdef HAVE_BUILTIN_CPU_SUPPORTS
static void (*resolve_multiply(void))(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
#  if defined __GNUC__ && !defined __clang__
    __builtin_cpu_init();
#  endif
#  ifdef KEYLEDSD_USE_AVX2
    if (__builtin_cpu_supports("avx2")) { return multiply_avx2; }
#  endif
#  ifdef KEYLEDSD_USE_SSE2
    if (__builtin_cpu_supports("sse2")) { return multiply_sse2; }
#  endif
    return multiply_plain;
}

#  ifdef HAVE_IFUNC_ATTRIBUTE
void multiply(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
    __attribute__((ifunc("resolve_multiply")));
#  else
static void (*resolved_multiply)(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length);
void multiply(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    if (resolved_multiply == 0) { resolved_multiply = resolve_multiply(); }
    (*resolved_multiply)(dst, src, length);
}
#  endif
#else
void multiply(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
    { multiply_plain(dst, src, length); }
#endif
