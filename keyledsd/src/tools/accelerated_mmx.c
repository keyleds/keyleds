#include <assert.h>
#include <stdint.h>
#include <mmintrin.h>
#include "config.h"

void blend_mmx(uint8_t * restrict a, const uint8_t * restrict b, unsigned length)
{
    a = (uint8_t*)__builtin_assume_aligned(a, 16);
    b = (const uint8_t*)__builtin_assume_aligned(b, 16);

    assert((uintptr_t)a % 16 == 0);
    assert((uintptr_t)b % 16 == 0);
    assert(length % 16 == 0);

    for (unsigned idx = 0; idx < length; idx += 8) {
        __m64 packed_a = *(__m64*)(a + idx);
        __m64 packed_b = *(__m64*)(b + idx);

        __m64 a1v = _mm_unpacklo_pi8(packed_a, _mm_setzero_si64());
        __m64 a2v = _mm_unpackhi_pi8(packed_a, _mm_setzero_si64());
        __m64 b1v = _mm_unpacklo_pi8(packed_b, _mm_setzero_si64());
        __m64 b2v = _mm_unpackhi_pi8(packed_b, _mm_setzero_si64());

        uint16_t alpha1 = b[idx + 3];
        uint16_t alpha2 = b[idx + 7];
        if (alpha1 != 0) { alpha1 += 1; }
        if (alpha2 != 0) { alpha2 += 1; }

        __m64 weighted_a1 = _mm_mullo_pi16(a1v, _mm_set1_pi16(256 - alpha1));
        __m64 weighted_a2 = _mm_mullo_pi16(a2v, _mm_set1_pi16(256 - alpha2));
        __m64 weighted_b1 = _mm_mullo_pi16(b1v, _mm_set1_pi16(alpha1));
        __m64 weighted_b2 = _mm_mullo_pi16(b2v, _mm_set1_pi16(alpha2));

        __m64 final_a1 = _m_psrlwi(_mm_add_pi16(weighted_a1, weighted_b1), 8);
        __m64 final_a2 = _m_psrlwi(_mm_add_pi16(weighted_a2, weighted_b2), 8);

        *(__m64*)(a + idx) = _mm_packs_pu16(final_a1, final_a2);
    }
    _mm_empty();
}
