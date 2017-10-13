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
#include "keyledsd/device/Device.h"
#include "keyledsd/colors.h"
#include "tools/AnimationLoop.h"
#include "config.h"

namespace keyleds { namespace device {

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
    static constexpr std::size_t   align_bytes = 32;    // 16 is minimum for SSE2, 32 for AVX2
    static constexpr std::size_t   align_colors = align_bytes / sizeof(RGBAColor);
public:
    using value_type = RGBAColor;
    using size_type = unsigned int;                     // we don't need size_t extra range
    using difference_type = signed int;
    using reference = value_type &;
    using const_reference = const value_type &;
    using iterator = value_type *;
    using const_iterator = const value_type *;
public:
                                RenderTarget(size_type);
                                RenderTarget(RenderTarget &&) noexcept;
    RenderTarget &              operator=(RenderTarget &&) noexcept;
                                ~RenderTarget();

    iterator                    begin() { return &m_colors[0]; }
    const_iterator              begin() const { return &m_colors[0]; }
    const_iterator              cbegin() const { return &m_colors[0]; }
    iterator                    end() { return &m_colors[m_size]; }
    const_iterator              end() const { return &m_colors[m_size]; }
    const_iterator              cend() const { return &m_colors[m_size]; }
    bool                        empty() const noexcept { return false; }
    size_type                   size() const noexcept { return m_size; }
    size_type                   capacity() const noexcept { return m_capacity; }
    value_type *                data() { return m_colors; }
    const value_type *          data() const { return m_colors; }
    reference                   operator[](size_type idx) { return m_colors[idx]; }
    const_reference             operator[](size_type idx) const { return m_colors[idx]; }

private:
    RGBAColor *                 m_colors;       ///< Color buffer. RGBAColor is a POD type
    size_type                   m_size;         ///< Number of color entries
    size_type                   m_capacity;     ///< Number of allocated color entries

    friend void swap(RenderTarget &, RenderTarget &) noexcept;
};

KEYLEDSD_EXPORT void swap(RenderTarget &, RenderTarget &) noexcept;
KEYLEDSD_EXPORT void blend(RenderTarget &, const RenderTarget &);

/****************************************************************************/

/** Renderer interface
 *
 * The interface an object must expose should it want to draw within a
 * RenderLoop
 */
class Renderer
{
protected:
    using RenderTarget = keyleds::device::RenderTarget;
public:
    /// Modifies the target to reflect effect's display once the specified time has elapsed
    virtual void    render(unsigned long nanosec, RenderTarget & target) = 0;
protected:
    // Protect the destructor so we can leave it non-virtual
    ~Renderer() {}
};

/****************************************************************************/

/** Device render loop
 *
 * An AnimationLoop that runs a set of Renderers and sends the resulting
 * RenderTarget state to a Device. It assumes entire control of the device.
 * That is, no other thread is allowed to call Device's manipulation methods
 * while a RenderLoop for it exists.
 */
class RenderLoop final : public tools::AnimationLoop
{
    using renderer_list = std::vector<Renderer *>;
public:
                        RenderLoop(Device &, unsigned fps);
                        ~RenderLoop() override;

    /// Returns a lock that bars the render loop from using renderers while it is held
    /// Holding it is mandatory for modifying any renderer or the list itself
    std::unique_lock<std::mutex>    lock();

    /// Renderer list accessor. When using it to modify renderers, a lock must be held.
    /// The list only holds pointers, which must be valid as long as they remain
    /// in the list. RenderLoop will not destroy them or interact in any way but
    /// calling their render method.
    renderer_list &     renderers() { return m_renderers; }

    /// Creates a new render target matching the layout of given device
    static RenderTarget renderTargetFor(const Device &);

private:
    bool                render(unsigned long) override;
    void                run() override;

    /// Reads current device led state into the render target
    void                getDeviceState(RenderTarget & state);

private:
    Device &            m_device;               ///< The device to render to
    renderer_list       m_renderers;            ///< Current list of renderers (unowned)
    std::mutex          m_mRenderers;           ///< Controls access to m_renderers

    RenderTarget        m_state;                ///< Current state of the device
    RenderTarget        m_buffer;               ///< Buffer to render into, avoids re-creating it
                                                ///  on every render
    std::vector<Device::ColorDirective> m_directives;   ///< Buffer of directives, avoids new/delete on
                                                        ///< every render
};

/****************************************************************************/

} } // namespace keyleds::device

#endif
