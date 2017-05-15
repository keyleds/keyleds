#include <iostream>
#include <time.h>
#include "config.h"
#include "tools/AnimationLoop.h"


AnimationLoop::AnimationLoop(unsigned fps, QObject * parent)
    : QThread(parent),
      m_fps(fps),
      m_paused(false),
      m_abort(false),
      m_error(0)
{
}

AnimationLoop::~AnimationLoop()
{
}

void AnimationLoop::stop()
{
    m_mRunStatus.lock();
    m_abort = true;
    m_cRunStatus.wakeOne();
    m_mRunStatus.unlock();

    wait();
}

void AnimationLoop::setPaused(bool paused)
{
    if (paused != m_paused) {
        m_mRunStatus.lock();
        m_paused = paused;
        m_cRunStatus.wakeOne();
        m_mRunStatus.unlock();
        emit pausedChanged(paused);
    }
}

void AnimationLoop::run()
{
    struct timespec lastTick, nextTick;

    while (!m_abort) {
        m_mRunStatus.lock();
        while (m_paused && !m_abort) {
            m_cRunStatus.wait(&m_mRunStatus);
        }
        m_mRunStatus.unlock();

        if (m_abort) { break; }

        clock_gettime(CLOCK_REALTIME, &lastTick);
        if (!render(0)) { return; }

        while (!m_paused && !m_abort) {
            int err;
            scheduleNextTick(nextTick, lastTick);

            if (!render(1000000000 / m_fps)) { return; }

            /* Wait until next tick, an error or told to pause/terminate */
            do {
                err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &nextTick, NULL);
            } while (err == EINTR && !m_paused && !m_abort);

            if (err != 0 && err != EINTR) {
                m_error = err;
                return;
            }
        }
    }
}

void AnimationLoop::scheduleNextTick(struct timespec & next, const struct timespec & prev)
{
    next.tv_nsec = prev.tv_nsec + (1000000000 / m_fps);
    next.tv_sec = prev.tv_sec;
    if (next.tv_nsec >= 1000000000) {
        next.tv_sec += 1;
        next.tv_nsec -= 1000000000;
    }
}
