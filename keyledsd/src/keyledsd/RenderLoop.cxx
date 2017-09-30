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
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <exception>
#include <thread>
#include <type_traits>
#include "keyledsd/Device.h"
#include "keyledsd/PluginManager.h"
#include "keyledsd/RenderLoop.h"
#include "tools/accelerated.h"
#include "keyleds.h"
#include "logging.h"

static_assert(std::is_pod<keyleds::RGBAColor>::value, "RGBAColor must be a POD type");
static_assert(sizeof(keyleds::RGBAColor) == 4, "RGBAColor must be tightly packed");

LOGGING("render-loop");

using keyleds::RenderTarget;
using keyleds::RenderLoop;

/// Returns the given value, aligned to upper bound of given aligment
static std::size_t align(std::size_t value, std::size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/****************************************************************************/

RenderTarget::RenderTarget(size_type numKeys)
 : m_colors(nullptr),
   m_nbColors(numKeys)
{
    numKeys = align(numKeys, align_colors);

    if (::posix_memalign(reinterpret_cast<void**>(&m_colors), align_bytes, numKeys * sizeof(m_colors[0])) != 0) {
        throw std::bad_alloc();
    }
}

RenderTarget::RenderTarget(RenderTarget && other) noexcept
 : m_colors(nullptr),
   m_nbColors(other.m_nbColors)
{
    using std::swap;
    swap(m_colors, other.m_colors);
}

RenderTarget & RenderTarget::operator=(RenderTarget && other) noexcept
{
    using std::swap;
    free(m_colors);
    m_colors = nullptr;
    swap(*this, other);
    return *this;
}

RenderTarget::~RenderTarget()
{
    free(m_colors);
}

void keyleds::swap(RenderTarget & lhs, RenderTarget & rhs) noexcept
{
    using std::swap;
    swap(lhs.m_colors, rhs.m_colors);
    swap(lhs.m_nbColors, rhs.m_nbColors);
}

void keyleds::blend(RenderTarget & lhs, const RenderTarget & rhs)
{
    assert(lhs.size() == rhs.size());
    ::blend(reinterpret_cast<uint8_t*>(lhs.data()),
            reinterpret_cast<const uint8_t*>(rhs.data()), rhs.size());
}

/****************************************************************************/

RenderLoop::RenderLoop(Device & device, unsigned fps)
    : AnimationLoop(fps),
      m_device(device),
      m_state(renderTargetFor(device)),
      m_buffer(renderTargetFor(device))
{
    // Ensure no allocation happens in render()
    std::size_t max = 0;
    for (const auto & block : m_device.blocks()) {
        max = std::max(max, block.keys().size());
    }
    m_directives.reserve(max);
}

RenderLoop::~RenderLoop()
{}

std::unique_lock<std::mutex> RenderLoop::lock()
{
    return std::unique_lock<std::mutex>(m_mEffects);
}

RenderTarget RenderLoop::renderTargetFor(const Device & device)
{
    return RenderTarget(std::accumulate(
        device.blocks().begin(), device.blocks().end(), RenderTarget::size_type{0},
        [](auto sum, const auto & block) { return sum + block.keys().size(); }
    ));
}

bool RenderLoop::render(unsigned long nanosec)
{
    // Run all renderers
    bool hasEffects;
    {
        std::lock_guard<std::mutex> lock(m_mEffects);
        hasEffects = !m_effects.empty();
        for (const auto & effect : m_effects) {
            effect->render(nanosec, m_buffer);
        }
    }

    if (hasEffects) {
        m_device.flush();   // Ensure another program using the device did not fill
                            // The inbound report queue.

        // Compute diff
        bool hasChanges = false;
        auto oldKeyIt = m_state.cbegin();
        auto newKeyIt = m_buffer.cbegin();

        for (const auto & block : m_device.blocks()) {
            const size_t numBlockKeys = block.keys().size();
            m_directives.clear();

            for (size_t kIdx = 0; kIdx < numBlockKeys; ++kIdx) {
                if (*oldKeyIt != *newKeyIt) {
                    m_directives.push_back({
                        block.keys()[kIdx], newKeyIt->red, newKeyIt->green, newKeyIt->blue
                    });
                }
                ++oldKeyIt;
                ++newKeyIt;
            }
            if (!m_directives.empty()) {
                m_device.setColors(block, m_directives.data(), m_directives.size());
                hasChanges = true;
            }
        }

        // Commit color changes
        if (hasChanges) { m_device.commitColors(); }

        using std::swap;
        swap(m_state, m_buffer);
    }

    return true;
}

void RenderLoop::run()
{
    try {
        getDeviceState(m_state);
    } catch (Device::error & error) {
        ERROR("device error: ", error.what());
        return;
    }

    try {
        for (;;) {
            try {
                AnimationLoop::run();
                break;
            } catch (Device::error & error) {
                // Something went wrong, we will attempt to recover
                if (error.code() == KEYLEDS_ERROR_ERRNO
                    && error.oserror() != EIO && error.oserror() != EINTR) {
                    throw;  // recovering from system errors is not an option
                }

                // Recover from error, giving some delay to the device
                WARNING("error on device: ", error.what(), " re-syncing device");
                unsigned attempt;
                for (attempt = 0; attempt < 5; ++attempt) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(attempt * 100));
                    if (m_device.resync()) { break; }
                }

                // If recovery failed, re-throw initial error
                if (attempt >= 5) { throw; }
            }
        }
    } catch (Device::error & error) {
        if (!((error.code() == KEYLEDS_ERROR_ERRNO && error.oserror() == ENODEV) ||
              error.code() == KEYLEDS_ERROR_TIMEDOUT)) {
            ERROR("device error: ", error.what());
        }
    } catch (std::exception & error) {
        ERROR(error.what());
    }
}

void RenderLoop::getDeviceState(RenderTarget & state)
{
    auto kit = state.begin();

    for (const auto & block : m_device.blocks()) {
        auto colors = m_device.getColors(block);

        assert(block.keys().size() == colors.size());
        for (const auto & color : colors) {
            kit->red = color.red;
            kit->green = color.green;
            kit->blue = color.blue;
            kit->alpha = 255;
            ++kit;
        }
    }
}
