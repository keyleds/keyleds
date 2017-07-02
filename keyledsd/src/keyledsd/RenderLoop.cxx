#include <errno.h>
#include <cstdlib>
#include <iostream>
#include <type_traits>
#include "keyledsd/Device.h"
#include "keyledsd/RenderLoop.h"
#include "keyleds.h"
#include "logging.h"

static_assert(std::is_pod<keyleds::RGBAColor>::value, "RGBAColor must be a POD type");

LOGGING("render-loop");

using keyleds::RenderTarget;
using keyleds::Renderer;
using keyleds::RenderLoop;

static size_t align(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/****************************************************************************/

RenderTarget::RenderTarget(const std::vector<std::size_t> & block_sizes)
 : m_colors(nullptr)
{
    m_blocks.reserve(block_sizes.size());

    std::size_t totalColors = 0;
    for (auto nbColors : block_sizes) {
        totalColors = align(totalColors + nbColors, align_colors);
    }

    if (::posix_memalign(reinterpret_cast<void**>(&m_colors), align_bytes, totalColors * sizeof(m_colors[0])) != 0) {
        throw std::bad_alloc();
    }
    m_nbColors = totalColors;

    totalColors = 0;
    for (auto nbColors : block_sizes) {
        m_blocks.push_back(&m_colors[totalColors]);
        totalColors = align(totalColors + nbColors, align_colors);
    }
}

RenderTarget::RenderTarget(RenderTarget && other)
{
    m_colors = other.m_colors;
    m_nbColors = other.m_nbColors;
    m_blocks = std::move(other.m_blocks);
    other.m_colors = nullptr;
}

RenderTarget::~RenderTarget()
{
    free(m_colors);
}

RenderTarget RenderTarget::for_device(const Device & device)
{
    std::vector<std::size_t> block_sizes;
    block_sizes.reserve(device.blocks().size());
    for (const auto & block : device.blocks()) {
        block_sizes.push_back(block.keys().size());
    }
    return RenderTarget(block_sizes);
}

template <> void std::swap<RenderTarget>(RenderTarget & lhs, RenderTarget & rhs)
{
    std::swap(lhs.m_colors, rhs.m_colors);
    std::swap(lhs.m_nbColors, rhs.m_nbColors);
    std::swap(lhs.m_blocks, rhs.m_blocks);
}

/****************************************************************************/

Renderer::~Renderer() {}

/****************************************************************************/

RenderLoop::RenderLoop(Device & device, renderer_list && renderers, unsigned fps)
    : AnimationLoop(fps),
      m_device(device),
      m_renderers(std::move(renderers)),
      m_state(RenderTarget::for_device(device)),
      m_buffer(RenderTarget::for_device(device))
{}

RenderLoop::~RenderLoop() {}

void RenderLoop::setRenderers(renderer_list && renderers)
{
    std::lock_guard<std::mutex> lock(m_mRenderers);
    m_renderers = std::move(renderers);
    DEBUG("enabled ", m_renderers.size(), " renderers for loop ", this);
}

bool RenderLoop::render(unsigned long nanosec)
{
    // Run all renderers
    {
        std::lock_guard<std::mutex> lock(m_mRenderers);
        for (const auto & renderer : m_renderers) {
            renderer->render(nanosec, m_buffer);
        }
    }

    // Compute diff
    bool hasChanges = false;
    const auto & blocks = m_device.blocks();
    for (size_t bidx = 0; bidx < blocks.size(); ++bidx) {
        const size_t nKeys = blocks[bidx].keys().size();
        Device::ColorDirective directives[nKeys];
        size_t nDirectives = 0;

        for (size_t idx = 0; idx < nKeys; ++idx) {
            const auto & color = m_buffer.get(bidx, idx);
            if (color != m_state.get(bidx, idx)) {
                directives[nDirectives++] = Device::ColorDirective{
                    blocks[bidx].keys()[idx], color.red, color.green, color.blue
                };
            }
        }
        if (nDirectives > 0) {
            m_device.setColors(blocks[bidx], directives, nDirectives);
            hasChanges = true;
        }
    }

    // Commit color changes
    if (hasChanges) { m_device.commitColors(); }
    std::swap(m_state, m_buffer);

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

    m_device.setTimeout(0); // disable timeout detection

    try {
        for (;;) {
            try {
                AnimationLoop::run();
                break;
            } catch (Device::error & error) {
                if (!m_device.resync()) { throw; }
            }
        }
    } catch (Device::error & error) {
        if (!((error.code() == KEYLEDS_ERROR_ERRNO && errno == ENODEV) ||
              error.code() == KEYLEDS_ERROR_TIMEDOUT)) {
            ERROR("device error: ", error.what());
        }
    } catch (std::exception & error) {
        ERROR(error.what());
    }
}

void RenderLoop::getDeviceState(RenderTarget & state)
{
    const auto & blocks = m_device.blocks();

    for (size_t block_idx = 0; block_idx < blocks.size(); ++block_idx) {
        auto colors = m_device.getColors(blocks[block_idx]);

        for (size_t idx = 0; idx < colors.size(); ++idx) {
            auto & color = state.get(block_idx, idx);
            color.red = colors[idx].red;
            color.green = colors[idx].green;
            color.blue = colors[idx].blue;
            color.alpha = 255;
        }
    }
}
