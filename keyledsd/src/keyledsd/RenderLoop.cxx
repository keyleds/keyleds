#include <errno.h>
#include <iostream>
#include "keyledsd/Device.h"
#include "keyledsd/RenderLoop.h"

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
    for (auto it = devblocks.begin(); it != devblocks.end(); ++it) {
        totalKeys += it->keys().size();
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

RenderLoop::RenderLoop(Device & device, renderer_list && renderers, unsigned fps, QObject *parent)
    : AnimationLoop(fps, parent),
      m_device(device),
      m_renderers(std::move(renderers)),
      m_state(device),
      m_buffer(device)
{}

RenderLoop::~RenderLoop() {}

bool RenderLoop::render(unsigned long nanosec)
{
    // Run all renderers
    for (auto rit = m_renderers.begin(); rit != m_renderers.end(); ++rit) {
        (*rit)->render(nanosec, m_buffer);
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

    if (hasChanges) { m_device.commitColors(); }
    std::swap(m_state, m_buffer);
    return true;
}

void RenderLoop::run()
{
    try {
        getDeviceState(m_state);
    } catch (Device::error & error) {
        std::cerr <<"Device error: " <<error.what() <<std::endl;
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
            std::cerr <<"Device error in render loop: " <<error.what() <<std::endl;
        }
    } catch (std::exception & error) {
        std::cerr <<"Error in render loop: " <<error.what() <<std::endl;
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
