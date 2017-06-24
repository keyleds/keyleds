#include <errno.h>
#include <iostream>
#include "keyledsd/Device.h"
#include "keyledsd/RenderLoop.h"
#include "logging.h"

LOGGING("render-loop");

using keyleds::RenderTarget;
using keyleds::Renderer;
using keyleds::RenderLoop;

static size_t align(size_t value, size_t alignment)
{
    return (value + alignment - 1) & ~(alignment - 1);
}

/****************************************************************************/

const size_t RenderTarget::alignment = 4;

RenderTarget::RenderTarget(const Device & device)
{
    const auto & devblocks = device.blocks();

    size_t totalKeys = 0;
    for (const auto & block : devblocks) {
        totalKeys += block.keys().size();
        totalKeys = align(totalKeys, alignment);
    }
    keys.resize(totalKeys);

    totalKeys = 0;
    blocks.resize(devblocks.size());
    for (size_t idx = 0; idx < devblocks.size(); idx += 1) {
        blocks[idx] = &keys.data()[totalKeys];
        totalKeys += devblocks[idx].keys().size();
        totalKeys = align(totalKeys, alignment);
    }
}

/****************************************************************************/

Renderer::~Renderer() {}

/****************************************************************************/

RenderLoop::RenderLoop(Device & device, renderer_list && renderers, unsigned fps)
    : AnimationLoop(fps),
      m_device(device),
      m_renderers(std::move(renderers)),
      m_state(device),
      m_buffer(device)
{}

RenderLoop::~RenderLoop() {}

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
            if (m_state.blocks[bidx][idx] != m_buffer.blocks[bidx][idx]) {
                directives[nDirectives++] = Device::ColorDirective{
                    blocks[bidx].keys().at(idx),
                    m_buffer.blocks[bidx][idx].red,
                    m_buffer.blocks[bidx][idx].green,
                    m_buffer.blocks[bidx][idx].blue
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
            state.blocks[block_idx][idx].red = colors[idx].red;
            state.blocks[block_idx][idx].green = colors[idx].green;
            state.blocks[block_idx][idx].blue = colors[idx].blue;
            state.blocks[block_idx][idx].alpha = 255;
        }
    }
}
