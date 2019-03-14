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
#include "keyledsd/RenderTarget.h"

#include "keyledsd/tools/accelerated.h"
#include <benchmark/benchmark.h>

using keyleds::RenderTarget;
using keyleds::RGBColor;
using keyleds::RGBAColor;
namespace architecture = keyleds::tools::architecture;


template <typename Architecture> static void BM_blend(benchmark::State & state)
{
    auto target = RenderTarget(RenderTarget::size_type(state.range(0)));
    auto source = RenderTarget(RenderTarget::size_type(state.range(0)));
    std::fill(target.begin(), target.end(), RGBAColor{0, 0, 0, 255});
    std::fill(target.begin(), target.end(), RGBAColor{255, 255, 255, 32});

    for (auto _ : state) {
        keyleds::blend<Architecture>(target, source);
    }
}
BENCHMARK_TEMPLATE(BM_blend, architecture::plain)->RangeMultiplier(2)->Range(32, 2<<16);
BENCHMARK_TEMPLATE(BM_blend, architecture::sse2)->RangeMultiplier(2)->Range(32, 2<<16);
BENCHMARK_TEMPLATE(BM_blend, architecture::avx2)->RangeMultiplier(2)->Range(32, 2<<16);

template <typename Architecture> static void BM_multiply(benchmark::State & state)
{
    auto target = RenderTarget(RenderTarget::size_type(state.range(0)));
    auto source = RenderTarget(RenderTarget::size_type(state.range(0)));
    std::fill(target.begin(), target.end(), RGBAColor{0, 0, 0, 255});
    std::fill(target.begin(), target.end(), RGBAColor{255, 255, 255, 32});

    for (auto _ : state) {
        keyleds::multiply<Architecture>(target, source);
    }
}
BENCHMARK_TEMPLATE(BM_multiply, architecture::plain)->RangeMultiplier(2)->Range(32, 2<<16);
BENCHMARK_TEMPLATE(BM_multiply, architecture::sse2)->RangeMultiplier(2)->Range(32, 2<<16);
BENCHMARK_TEMPLATE(BM_multiply, architecture::avx2)->RangeMultiplier(2)->Range(32, 2<<16);

BENCHMARK_MAIN();
