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
    if (__builtin_cpu_supports("sse2")) { return blend_sse2; }
    if (__builtin_cpu_supports("mmx")) { return blend_mmx; }
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
