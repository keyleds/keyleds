#ifndef KEYLEDS_RENDER_LOOP_H
#define KEYLEDS_RENDER_LOOP_H

#include <list>
#include <memory>
#include <mutex>
#include "keyledsd/common.h"
#include "tools/AnimationLoop.h"

namespace keyleds {

class Device;

/****************************************************************************/

struct RenderTarget
{
    RenderTarget(const Device &);

    std::vector<RGBAColor>      keys;
    std::vector<RGBAColor *>    blocks;

    static const size_t         alignment;
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
    virtual             ~Renderer();

    virtual void        render(unsigned long nanosec, RenderTarget & target) = 0;
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

};

#endif
