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
#include "keyledsd/service/RenderLoop.h"

#include "keyledsd/device/Device.h"
#include "keyledsd/logging.h"
#include <algorithm>
#include <cassert>
#include <cerrno>
#include <chrono>
#include <exception>
#include <numeric>
#include <thread>

LOGGING("render-loop");

using keyleds::service::RenderLoop;
using namespace std::literals::chrono_literals;

static constexpr auto errorGracePeriod = 60s;
struct commitDelay {    // Delay between sending color data and commit command.
    static constexpr std::chrono::microseconds initial = 0ms;
    static constexpr std::chrono::microseconds increment = 1000us;
    static constexpr std::chrono::microseconds max = 8ms;
};

/****************************************************************************/

RenderLoop::RenderLoop(device::Device & device, unsigned fps)
    : AnimationLoop(fps),
      m_device(device),
      m_commitDelay(commitDelay::initial),
      m_forceRefresh(false),
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

RenderLoop::~RenderLoop() = default;

/** Lock render loop, to synchronize renderer list access.
 * @return Mutex lock preventing the animation from using renderers until it is destroyed.
 */
std::unique_lock<std::mutex> RenderLoop::lock()
{
    return std::unique_lock<std::mutex>(m_mRenderers);
}

/** Create render target for a device.
 * @param device Device to create a render target for.
 * @return Newly created render target.
 */
keyleds::RenderTarget RenderLoop::renderTargetFor(const device::Device & device)
{
    return RenderTarget(std::accumulate(
        device.blocks().begin(), device.blocks().end(), RenderTarget::size_type{0},
        [](auto sum, const auto & block) { return sum + block.keys().size(); }
    ));
}

/** Rendering method
 * Invoked on a regular basis as long as the animation is not paused.
 * @param elapsed Time since last invocation.
 * @return `true` if animation should be continued, else `false`.
 */
bool RenderLoop::render(milliseconds elapsed)
{
    // Run all renderers
    bool hasRenderers;
    {
        std::lock_guard<std::mutex> lock(m_mRenderers);
        hasRenderers = !m_renderers.empty();
        for (const auto & effect : m_renderers) {
            effect->render(elapsed, m_buffer);
        }
    }

    if (hasRenderers) {
        m_device.flush();   // Ensure another program using the device did not fill
                            // The inbound report queue.

        // Compute diff between old LED state and new LED state
        bool forceRefresh = m_forceRefresh.exchange(false, std::memory_order_relaxed);
        bool hasChanges = false;
        auto oldKeyIt = m_state.cbegin();
        auto newKeyIt = m_buffer.cbegin();

        for (const auto & block : m_device.blocks()) {

            // Look for changed lights within current block
            const size_t numBlockKeys = block.keys().size();
            m_directives.clear();
            for (size_t kIdx = 0; kIdx < numBlockKeys; ++kIdx) {
                if (forceRefresh || *oldKeyIt != *newKeyIt) {
                    m_directives.push_back({
                        block.keys()[kIdx], newKeyIt->red, newKeyIt->green, newKeyIt->blue
                    });
                }
                ++oldKeyIt;
                ++newKeyIt;
            }

            // If some lights have changed within current block, send directives to device
            if (!m_directives.empty()) {
                m_device.setColors(block, m_directives.data(),
                                   static_cast<device::Device::size_type>(m_directives.size()));
                hasChanges = true;
            }
        }

        // Commit color changes, if any
        if (hasChanges) {
            std::this_thread::sleep_for(m_commitDelay);
            m_device.commitColors();
        }

        using std::swap;
        swap(m_state, m_buffer);
    }

    return true;
}

/** Main render loop loop.
 * Handle error recovery around AnimationLoop::run().
 */
void RenderLoop::run()
{
    try {
        getDeviceState(m_state);
    } catch (device::Device::error & error) {
        ERROR("device error: ", error.what());
        return;
    }
    std::copy(m_state.cbegin(), m_state.cend(), m_buffer.begin());

    try {
        for (;;) {
            try {
                AnimationLoop::run();
                break;
            } catch (device::Device::error & error) {
                // Something went wrong, we will attempt to recover
                if (!error.recoverable()) { throw; }
                ERROR("error on device: ", error.what(), ", re-syncing device");

                // If errors happen in succession, increase commit delay.
                // Some devices are slow and need significant time before commit.
                auto now = clock::now();
                if (now - m_lastErrorTime < errorGracePeriod && m_commitDelay < commitDelay::max)
                {
                    m_commitDelay += commitDelay::increment;
                    WARNING("increased commit delay to ", m_commitDelay.count(), "us");
                }
                m_lastErrorTime = now;

                // Recover from error, giving some delay to the device
                bool success = false;
                for (unsigned attempt = 0; !success && attempt < 5; ++attempt) {
                    std::this_thread::sleep_for(std::chrono::milliseconds(attempt * 100));
                    success = m_device.resync();
                }

                if (!success) { throw; }
            }
        }
    } catch (device::Device::error & error) {
        if (!error.expected()) { ERROR("device error: ", error.what(), ", stopping animation"); }
    } catch (std::exception & error) {
        ERROR(error.what());
    }
}

/** Read current state of all device lights
 * @param [out] state Buffer into which color values will be written.
 */
void RenderLoop::getDeviceState(RenderTarget & state)
{
    auto kit = state.begin();

    for (const auto & block : m_device.blocks()) {
        std::vector<device::Device::ColorDirective> colors(block.keys().size());
        m_device.getColors(block, colors.data());

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
