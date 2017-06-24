#ifndef TOOLS_ANIM_LOOP_H
#define TOOLS_ANIM_LOOP_H

#include <atomic>
#include <condition_variable>
#include <mutex>
#include <thread>

struct timespec;

class AnimationLoop
{
public:
                    AnimationLoop(unsigned fps);
    virtual         ~AnimationLoop() {}

    bool            paused() const { return m_paused; }
    int             error() const { return m_error; }

    void            setPaused(bool paused);
    void            stop();

protected:
    virtual void    run();
    virtual bool    render(unsigned long) = 0;
    void            scheduleNextTick(struct timespec & next, const struct timespec & prev);

private:
    static void     threadEntry(AnimationLoop &);

private:
    std::mutex      m_mRunStatus;
    std::condition_variable m_cRunStatus;

    unsigned        m_period;
    std::atomic<bool> m_paused;
    std::atomic<bool> m_abort;
    int             m_error;

    std::thread     m_thread;
};

#endif
