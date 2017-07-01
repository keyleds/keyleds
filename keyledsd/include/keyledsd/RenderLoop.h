#ifndef KEYLEDS_RENDER_LOOP_H
#define KEYLEDS_RENDER_LOOP_H

#include <mutex>
#include <vector>
#include "keyledsd/common.h"
#include "tools/AnimationLoop.h"

namespace keyleds {

class Device;
class Renderer;

/****************************************************************************/

class RenderTarget final
{
    static constexpr std::size_t   align_bytes = 32;
    static constexpr std::size_t   align_colors = align_bytes / sizeof(RGBAColor);
public:
    typedef RGBAColor *         iterator;
    typedef std::pair<std::size_t, std::size_t> key_descriptor;
public:
                                RenderTarget(const std::vector<std::size_t> & block_sizes);
                                RenderTarget(RenderTarget &&);
                                ~RenderTarget();

    iterator                    begin() noexcept { return &m_colors[0]; }
    iterator                    end() noexcept { return &m_colors[m_nbColors]; }
    RGBAColor &                 get(std::size_t bidx, std::size_t kidx) noexcept
                                { return *(m_blocks[bidx] + kidx); }
    RGBAColor &                 get(const key_descriptor & desc) noexcept
                                { return *(m_blocks[desc.first] + desc.second); }

    static RenderTarget         for_device(const Device &);
private:
    RGBAColor *                 m_colors;
    std::size_t                 m_nbColors;
    std::vector<RGBAColor *>    m_blocks;

    friend void std::swap<RenderTarget>(RenderTarget &, RenderTarget &);
};

/****************************************************************************/
/** Render loop
 */
class RenderLoop final : public AnimationLoop
{
public:
    typedef std::vector<Renderer *> renderer_list;
public:
                    RenderLoop(Device & device, renderer_list && renderers, unsigned fps);
                    ~RenderLoop() override;

          renderer_list & renderers() { return m_renderers; }
    const renderer_list & renderers() const { return m_renderers; }
    std::unique_lock<std::mutex> renderersLock() { return std::unique_lock<std::mutex>(m_mRenderers); }

private:
    bool            render(unsigned long) override;
    void            run() override;

    void            getDeviceState(RenderTarget & state);

private:
    Device &        m_device;
    renderer_list   m_renderers;
    std::mutex      m_mRenderers;

    RenderTarget    m_state;
    RenderTarget    m_buffer;
};

/****************************************************************************/
/** Renderer interface
 *
 * An instance of a class supporting that interface is created for every link
 * in the rendering chain.
 */
class Renderer
{
public:
    virtual         ~Renderer();
    virtual void    render(unsigned long nanosec, RenderTarget & target) = 0;
};

/****************************************************************************/

};

namespace std {
    template<> void swap<keyleds::RenderTarget>(keyleds::RenderTarget & lhs, keyleds::RenderTarget & rhs);
}

#endif
