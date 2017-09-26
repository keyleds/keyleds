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
#ifndef KEYLEDS_RENDER_LOOP_H_D7E4709F
#define KEYLEDS_RENDER_LOOP_H_D7E4709F

#include <cstddef>
#include <mutex>
#include <utility>
#include <vector>
#include "keyledsd/common.h"
#include "keyledsd/Device.h"
#include "tools/AnimationLoop.h"

namespace keyleds {

class EffectPlugin;

/****************************************************************************/

/** Rendering buffer for key colors
 *
 * Holds RGBA color entries for all keys of a device. All key blocks are in the
 * same memory area. Each block is contiguous, but padding keys may be inserted
 * in between blocks so blocks are SSE2-aligned. The buffers is addressed through
 * a 2-tuple containing the block index and key index within block. No ordering
 * is enforce on blocks or keys, but the for_device static method uses the same
 * order that is detected on the device by the keyleds::Device object.
 */
class RenderTarget final
{
    static constexpr std::size_t   align_bytes = 32;
    static constexpr std::size_t   align_colors = align_bytes / sizeof(RGBAColor);
public:
    typedef RGBAColor *         iterator;
    typedef std::pair<std::size_t, std::size_t> key_descriptor;
public:
                                RenderTarget(const std::vector<std::size_t> & block_sizes);
                                RenderTarget(RenderTarget &&) noexcept;
                                ~RenderTarget();

    iterator                    begin() noexcept { return &m_colors[0]; }
    iterator                    end() noexcept { return &m_colors[m_nbColors]; }
    std::size_t                 size() const noexcept { return m_nbColors; }
    RGBAColor *                 data() noexcept { return m_colors; }
    const RGBAColor *           data() const noexcept { return m_colors; }
    RGBAColor &                 operator[](std::size_t idx) noexcept { return m_colors[idx]; }
    const RGBAColor &           operator[](std::size_t idx) const noexcept { return m_colors[idx]; }

    RGBAColor &                 get(std::size_t bidx, std::size_t kidx) noexcept
                                { return *(m_blocks[bidx] + kidx); }
    RGBAColor &                 get(const key_descriptor & desc) noexcept
                                { return *(m_blocks[desc.first] + desc.second); }
private:
    RGBAColor *                 m_colors;       ///< Color buffer. RGBAColor is a POD type
    std::size_t                 m_nbColors;     ///< Number of items in m_colors
    std::vector<RGBAColor *>    m_blocks;       ///< Block pointers withing m_colors

    friend void swap(RenderTarget &, RenderTarget &) noexcept;
};

void swap(RenderTarget &, RenderTarget &) noexcept;
void blend(RenderTarget &, const RenderTarget &);

/****************************************************************************/

/** Device render loop
 *
 * An AnimationLoop that runs a set of Renderers and sends the resulting
 * RenderTarget state to a Device. It assumes entier control of the device.
 * That is, no other thread is allowed to call Device's manipulation methods
 * while a RenderLoop for it exists.
 */
class RenderLoop final : public AnimationLoop
{
public:
    typedef std::vector<EffectPlugin *> effect_plugin_list;
public:
                    RenderLoop(Device &, unsigned fps);
                    ~RenderLoop() override;

    std::unique_lock<std::mutex>    lock();
    effect_plugin_list &            effects() { return m_effects; }

    static RenderTarget renderTargetFor(const Device &);

private:
    bool            render(unsigned long) override;
    void            run() override;

    void            getDeviceState(RenderTarget & state);

private:
    Device &            m_device;               ///< The device to render to
    effect_plugin_list  m_effects;              ///< Current list of effect plugins (unowned)
    std::mutex          m_mEffects;             ///< Controls access to m_effects

    RenderTarget        m_state;                ///< Current state of the device
    RenderTarget        m_buffer;               ///< Buffer to render into, avoids re-creating it
                                                ///  on every render
    std::vector<Device::ColorDirective> m_directives;   ///< Buffer of directives, avoids new/delete on
                                                        ///< every render
};

/****************************************************************************/

};

#endif
