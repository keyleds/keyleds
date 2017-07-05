#include <assert.h>
#include <stdint.h>
#include <mmintrin.h>
#include "config.h"

void blend_mmx(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    __m64 * restrict dstv = (__m64 *)__builtin_assume_aligned(dst, 16);
    const __m64 * restrict srcv = (const __m64 *)__builtin_assume_aligned(src, 16);

    const __m64 zero = _mm_setzero_si64();
    const __m64 one = _mm_set1_pi16(1);
    const __m64 max = _mm_set1_pi16(256);

    assert((uintptr_t)dst % 16 == 0);
    assert((uintptr_t)src % 16 == 0);
    assert(length % 4 == 0);

    length /= 2;

    do {
        __m64 packed_dst = *dstv;
        __m64 packed_src = *srcv;

        __m64 dst0 = _mm_unpacklo_pi8(packed_dst, zero); /* A0B0G0R0 */
        __m64 dst1 = _mm_unpackhi_pi8(packed_dst, zero); /* A1B1G1R1 */
        __m64 src0 = _mm_unpacklo_pi8(packed_src, zero); /* A0B0G0R0 */
        __m64 src1 = _mm_unpackhi_pi8(packed_src, zero); /* A1B1G1R1 */

        __m64 alpha0 = _mm_or_si64(src0, _mm_srli_si64(src0, 16));
        alpha0 = _mm_or_si64(alpha0, _mm_srli_si64(alpha0, 32));
        alpha0 = _mm_add_pi16(alpha0, _mm_add_pi16(_mm_cmpeq_pi16(alpha0, zero), one));
        __m64 alpha1 = _mm_or_si64(src1, _mm_srli_si64(src1, 16));
        alpha1 = _mm_or_si64(alpha1, _mm_srli_si64(alpha1, 32));
        alpha1 = _mm_add_pi16(alpha1, _mm_add_pi16(_mm_cmpeq_pi16(alpha1, zero), one));

        __m64 weighted_dst0 = _mm_mullo_pi16(dst0, _mm_sub_pi16(max, alpha0));
        __m64 weighted_dst1 = _mm_mullo_pi16(dst1, _mm_sub_pi16(max, alpha1));
        __m64 weighted_src0 = _mm_mullo_pi16(src0, alpha0);
        __m64 weighted_src1 = _mm_mullo_pi16(src1, alpha1);

/*        uint16_t alpha1 = b[idx + 3];
        uint16_t alpha2 = b[idx + 7];
        if (alpha1 != 0) { alpha1 += 1; }
        if (alpha2 != 0) { alpha2 += 1; }

        __m64 weighted_dst0 = _mm_mullo_pi16(dst0, _mm_set1_pi16(256 - alpha1));
        __m64 weighted_dst1 = _mm_mullo_pi16(dst1, _mm_set1_pi16(256 - alpha2));
        __m64 weighted_src0 = _mm_mullo_pi16(src0, _mm_set1_pi16(alpha1));
        __m64 weighted_src1 = _mm_mullo_pi16(src1, _mm_set1_pi16(alpha2));*/

        __m64 final_dst0 = _m_psrlwi(_mm_add_pi16(weighted_dst0, weighted_src0), 8);
        __m64 final_dst1 = _m_psrlwi(_mm_add_pi16(weighted_dst1, weighted_src1), 8);

        *dstv = _mm_packs_pu16(final_dst0, final_dst1);
        srcv += 1;
        dstv += 1;
    } while (--length > 0);

    _mm_empty();
}
