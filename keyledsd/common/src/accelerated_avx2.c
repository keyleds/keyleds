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
#include <immintrin.h>
#include "config.h"

void blend_avx2(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    assert((uintptr_t)dst % 32 == 0);   // AVX2 requires 32-bytes aligned data
    assert((uintptr_t)src % 32 == 0);   // AVX2 requires 32-bytes aligned data
    assert(length != 0);                // allows inverting loop condition, makes gcc generate
                                        // better loop code
    assert(length % 8 == 0);            // we'll process entries 8 by 8 and don't want to be
                                        // slowed by boundary checks

    __m256i * restrict dstv = (__m256i *)__builtin_assume_aligned(dst, 32);
    const __m256i * restrict srcv = (const __m256i *)__builtin_assume_aligned(src, 32);

    const __m256i zero = _mm256_setzero_si256();
    const __m256i one = _mm256_set1_epi16(1);
    const __m256i max = _mm256_set1_epi16(256);

    length /= 8;

    do {
        __m256i packed_dst = _mm256_load_si256(dstv);
        __m256i packed_src = _mm256_load_si256(srcv);

        __m256i dst0 = _mm256_unpacklo_epi8(packed_dst, zero); /* A3B3G3R3A2B2G2R2A1B1G1R1A0B0G0R0 */
        __m256i dst1 = _mm256_unpackhi_epi8(packed_dst, zero); /* A7B7G7R7A6B6G6R6A5B5G5R5A4B4G4R4 */
        __m256i src0 = _mm256_unpacklo_epi8(packed_src, zero); /* A3B3G3R3A2B2G2R2A1B1G1R1A0B0G0R0 */
        __m256i src1 = _mm256_unpackhi_epi8(packed_src, zero); /* A7B7G7R7A6B6G6R6A5B5G5R5A4B4G4R4 */

        __m256i alpha0 = _mm256_shufflelo_epi16(_mm256_shufflehi_epi16(src0, 0xff), 0xff);
        alpha0 = _mm256_add_epi16(alpha0, _mm256_add_epi16(_mm256_cmpeq_epi16(alpha0, zero), one));
        __m256i alpha1 = _mm256_shufflelo_epi16(_mm256_shufflehi_epi16(src1, 0xff), 0xff);
        alpha1 = _mm256_add_epi16(alpha1, _mm256_add_epi16(_mm256_cmpeq_epi16(alpha1, zero), one));


        __m256i weighted_dst0 = _mm256_mullo_epi16(dst0, _mm256_sub_epi16(max, alpha0));
        __m256i weighted_dst1 = _mm256_mullo_epi16(dst1, _mm256_sub_epi16(max, alpha1));
        __m256i weighted_src0 = _mm256_mullo_epi16(src0, alpha0);
        __m256i weighted_src1 = _mm256_mullo_epi16(src1, alpha1);

        __m256i final_dst0 = _mm256_srli_epi16(_mm256_add_epi16(weighted_dst0, weighted_src0), 8);
        __m256i final_dst1 = _mm256_srli_epi16(_mm256_add_epi16(weighted_dst1, weighted_src1), 8);

        _mm256_store_si256(dstv, _mm256_packus_epi16(final_dst0, final_dst1));
        srcv += 1;
        dstv += 1;
    } while (--length > 0);
}

void multiply_avx2(uint8_t * restrict dst, const uint8_t * restrict src, unsigned length)
{
    assert((uintptr_t)dst % 32 == 0);   // AVX2 requires 32-bytes aligned data
    assert((uintptr_t)src % 32 == 0);   // AVX2 requires 32-bytes aligned data
    assert(length != 0);                // allows inverting loop condition, makes gcc generate
                                        // better loop code
    assert(length % 8 == 0);            // we'll process entries 8 by 8 and don't want to be
                                        // slowed by boundary checks

    __m256i * restrict dstv = (__m256i *)__builtin_assume_aligned(dst, 32);
    const __m256i * restrict srcv = (const __m256i *)__builtin_assume_aligned(src, 32);

    const __m256i zero = _mm256_setzero_si256();
    const __m256i one = _mm256_set1_epi16(1);

    length /= 8;

    do {
        __m256i packed_dst = _mm256_load_si256(dstv);
        __m256i packed_src = _mm256_load_si256(srcv);

        __m256i dst0 = _mm256_unpacklo_epi8(packed_dst, zero); /* A3B3G3R3A2B2G2R2A1B1G1R1A0B0G0R0 */
        __m256i dst1 = _mm256_unpackhi_epi8(packed_dst, zero); /* A7B7G7R7A6B6G6R6A5B5G5R5A4B4G4R4 */
        __m256i src0 = _mm256_unpacklo_epi8(packed_src, zero); /* A3B3G3R3A2B2G2R2A1B1G1R1A0B0G0R0 */
        __m256i src1 = _mm256_unpackhi_epi8(packed_src, zero); /* A7B7G7R7A6B6G6R6A5B5G5R5A4B4G4R4 */

        dst0 = _mm256_mullo_epi16(src0, _mm256_add_epi16(dst0, one));
        dst1 = _mm256_mullo_epi16(src1, _mm256_add_epi16(dst1, one));

        dst0 = _mm256_srli_epi16(dst0, 8);
        dst1 = _mm256_srli_epi16(dst1, 8);

        _mm256_store_si256(dstv, _mm256_packus_epi16(dst0, dst1));
        srcv += 1;
        dstv += 1;
    } while (--length > 0);
}
