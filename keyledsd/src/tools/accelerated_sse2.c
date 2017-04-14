#include <assert.h>
#include <stdint.h>
#include <emmintrin.h>
#include "config.h"

void blend_sse2(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    __m128i * restrict dstv = (__m128i *)__builtin_assume_aligned(dst, 16);
    const __m128i * restrict srcv = (__m128i *)__builtin_assume_aligned(src, 16);

    const __m128i zero = _mm_setzero_si128();
    const __m128i one = _mm_set1_epi16(1);
    const __m128i max = _mm_set1_epi16(256);

    assert((uintptr_t)dst % 16 == 0);
    assert((uintptr_t)src % 16 == 0);
    assert(length % 16 == 0);
    length /= 16;

    while (length-- > 0) {
        __m128i packed_dst = _mm_load_si128(dstv);
        __m128i packed_src = _mm_load_si128(srcv);

        __m128i dst0 = _mm_unpacklo_epi8(packed_dst, zero); /* A1B1G1R1A0B0G0R0 */
        __m128i dst1 = _mm_unpackhi_epi8(packed_dst, zero); /* A3B3G3R3A2B2G2R2 */
        __m128i src0 = _mm_unpacklo_epi8(packed_src, zero); /* A1B1G1R1A0B0G0R0 */
        __m128i src1 = _mm_unpackhi_epi8(packed_src, zero); /* A3B3G3R3A2B2G2R2 */

        __m128i alpha0 = _mm_shufflelo_epi16(_mm_shufflehi_epi16(src0, 0xff), 0xff);
        alpha0 = _mm_add_epi16(alpha0, _mm_add_epi16(_mm_cmpeq_epi16(alpha0, zero), one));
        __m128i alpha1 = _mm_shufflelo_epi16(_mm_shufflehi_epi16(src1, 0xff), 0xff);
        alpha1 = _mm_add_epi16(alpha1, _mm_add_epi16(_mm_cmpeq_epi16(alpha1, zero), one));


        __m128i weighted_dst0 = _mm_mullo_epi16(dst0, _mm_sub_epi16(max, alpha0));
        __m128i weighted_dst1 = _mm_mullo_epi16(dst1, _mm_sub_epi16(max, alpha1));
        __m128i weighted_src0 = _mm_mullo_epi16(src0, alpha0);
        __m128i weighted_src1 = _mm_mullo_epi16(src1, alpha1);

        __m128i final_dst0 = _mm_srli_epi16(_mm_add_epi16(weighted_dst0, weighted_src0), 8);
        __m128i final_dst1 = _mm_srli_epi16(_mm_add_epi16(weighted_dst1, weighted_src1), 8);

        _mm_store_si128(dstv, _mm_packus_epi16(final_dst0, final_dst1));
        srcv += 1;
        dstv += 1;
    }
}
