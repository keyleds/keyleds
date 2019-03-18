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
#ifndef TOOLS_ACCELERATED_H_D3D3B426
#define TOOLS_ACCELERATED_H_D3D3B426

#ifdef __cplusplus
#include <cstddef>
#include <cstdint>

namespace keyleds::tools {
extern "C" {
#else
#include <stddef.h>
#include <stdint.h>
#endif

/** Blend two R8G8B8A8 color streams
 *
 * Perform a regular alpha blending, that is, compute:
 * \f$\begin{align*}
 *      a_n^{r}&=a_n^{r}(1-b_n^\alpha)+b_n^{r}b_n^\alpha \\
 *      a_n^{g}&=a_n^{g}(1-b_n^\alpha)+b_n^{g}b_n^\alpha \\
 *      a_n^{b}&=a_n^{b}(1-b_n^\alpha)+b_n^{b}b_n^\alpha \\
 * \end{align*}
 * The value of a's alpha channel after the blending is undefined.
 *
 * The blending operation uses SSE2 or MMX if available.
 *
 * @param[in|out] a An array of colors used as a destination. Must be 16-byte aligned.
 * @param b An array of colors used as a source. Must be 16-byte aligned.
 * @param length The number of colors in the arrays. Must be a multiple of 4.
 * @note Arrays must not overlap.
 */
void blend(uint8_t * a, const uint8_t * b, size_t length);

/** Multiply two R8G8B8A8 color streams
 *
 * Performs a simple multiplication.
 * \f$\begin{align*}
 * \end{align*}
 *
 * The product operation uses SSE2 or MMX if available.
 *
 * @param[in|out] a An array of colors used as a destination. Must be 16-byte aligned.
 * @param b An array of colors used as a source. Must be 16-byte aligned.
 * @param length The number of colors in the arrays. Must be a multiple of 4.
 * @note Arrays must not overlap.
 */
void multiply(uint8_t * a, const uint8_t * b, size_t length);

#ifdef __cplusplus
    namespace detail {  // exposed for testing purposes
#endif
        void blend_plain(uint8_t * a, const uint8_t * b, size_t length);
        void blend_sse2(uint8_t * a, const uint8_t * b, size_t length);
        void blend_avx2(uint8_t * a, const uint8_t * b, size_t length);
        void multiply_plain(uint8_t * a, const uint8_t * b, size_t length);
        void multiply_sse2(uint8_t * a, const uint8_t * b, size_t length);
        void multiply_avx2(uint8_t * a, const uint8_t * b, size_t length);
#ifdef __cplusplus
    } // namespace detail

    namespace architecture {
        struct plain {
            static inline void blend(uint8_t * a, const uint8_t * b, size_t length)
                { detail::blend_plain(a, b, length); }
            static inline void multiply(uint8_t * a, const uint8_t * b, size_t length)
                { detail::multiply_plain(a, b, length); }
        };
        struct sse2 {
            static inline void blend(uint8_t * a, const uint8_t * b, size_t length)
                { detail::blend_sse2(a, b, length); }
            static inline void multiply(uint8_t * a, const uint8_t * b, size_t length)
                { detail::multiply_sse2(a, b, length); }
        };
        struct avx2 {
            static inline void blend(uint8_t * a, const uint8_t * b, size_t length)
                { detail::blend_avx2(a, b, length); }
            static inline void multiply(uint8_t * a, const uint8_t * b, size_t length)
                { detail::multiply_avx2(a, b, length); }
        };
    } // namespace architecture

} // extern "C"
} // namespace keyleds::tools
#endif

#endif
