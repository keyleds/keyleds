#include <time.h>
#include "tools/AnimationLoop.h"
#include "config.h"
#include "logging.h"

LOGGING("anim-loop");

AnimationLoop::AnimationLoop(unsigned fps)
    : m_period(1000000000 / fps),
      m_paused(true),
      m_abort(false),
      m_error(0),
      m_thread(threadEntry, std::ref(*this))
{
}

void AnimationLoop::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_abort = true;
        m_cRunStatus.notify_one();
    }

    m_thread.join();
}

void AnimationLoop::setPaused(bool paused)
{
    if (paused != m_paused) {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_paused = paused;
        m_cRunStatus.notify_one();
    }
}

void AnimationLoop::run()
{
    struct timespec lastTick, nextTick;

    while (!m_abort.load(std::memory_order_relaxed)) {
        {
            std::unique_lock<std::mutex> lock(m_mRunStatus);
            if (m_paused) { DEBUG("AnimationLoop(", this, ") paused"); }
            m_cRunStatus.wait(lock, [this]{ return !m_paused || m_abort; });
        }

        if (m_abort) { break; }
        DEBUG("AnimationLoop(", this, ") resumed");

        clock_gettime(CLOCK_REALTIME, &nextTick);
        if (!render(0)) { return; }

        while (!m_paused.load(std::memory_order_relaxed) && !m_abort.load(std::memory_order_relaxed)) {
            int err;
            lastTick = nextTick;
            scheduleNextTick(nextTick, lastTick);

            if (!render(m_period)) { return; }

            /* Wait until next tick, an error or told to pause/terminate */
            do {
                err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &nextTick, nullptr);
            } while (err == EINTR &&
                     !m_paused.load(std::memory_order_relaxed) &&
                     !m_abort.load(std::memory_order_relaxed));

            if (err != 0 && err != EINTR) {
                m_error = err;
                return;
            }
        }
    }
    DEBUG("AnimationLoop(", this, ") exiting");
}

void AnimationLoop::scheduleNextTick(struct timespec & next, const struct timespec & prev)
{
    next.tv_nsec = prev.tv_nsec + m_period;
    next.tv_sec = prev.tv_sec;
    if (next.tv_nsec >= 1000000000) {
        next.tv_sec += 1;
        next.tv_nsec -= 1000000000;
    }
}

void AnimationLoop::threadEntry(AnimationLoop & loop)
{
    loop.run();
}
