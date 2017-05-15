#ifndef KEYLEDS_RENDER_LOOP_H
#define KEYLEDS_RENDER_LOOP_H

#include <QObject>
#include <list>
#include <memory>
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

    virtual bool        isFilter() const = 0;
    virtual void        render(unsigned long nanosec, RenderTarget & target) = 0;
};

/****************************************************************************/
/** Render loop
 */
class RenderLoop : public AnimationLoop
{
    Q_OBJECT
public:
    typedef std::unique_ptr<Renderer> renderer_ptr;
    typedef std::list<renderer_ptr> renderer_list;
public:
                    RenderLoop(Device & device, renderer_list && renderers,
                               unsigned fps, QObject *parent = 0);
                    ~RenderLoop() override;

    renderer_list & renderers() { return m_renderers; }

protected:
    bool            render(unsigned long) override;
    void            run() override;

    void            getDeviceState(RenderTarget & state);

protected:
    Device &        m_device;
    renderer_list   m_renderers;

    RenderTarget    m_state;
    RenderTarget    m_buffer;
};

/****************************************************************************/

};

#endif
