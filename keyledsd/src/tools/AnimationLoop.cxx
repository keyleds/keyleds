#include <time.h>
#include "config.h"
#include "tools/AnimationLoop.h"

using keyleds::AnimationLoop;

AnimationLoop::AnimationLoop(unsigned fps, QObject * parent)
    : QThread(parent), fps(fps)
{
    paused = false;
    abort = false;
    error = 0;
}

AnimationLoop::~AnimationLoop()
{
    mRunStatus.lock();
    abort = true;
    cRunStatus.wakeOne();
    mRunStatus.unlock();

    wait();
}

void AnimationLoop::setPaused(bool val)
{
    if (val != paused) {
        mRunStatus.lock();
        paused = val;
        cRunStatus.wakeOne();
        mRunStatus.unlock();
        emit pausedChanged(paused);
    }
}

void AnimationLoop::run()
{
    struct timespec nexttick;

    while (!abort) {
        mRunStatus.lock();
        while (paused && !abort) {
            cRunStatus.wait(&mRunStatus);
        }
        mRunStatus.unlock();

        while (!paused && !abort) {
            int err;
            scheduleNextTick(&nexttick);

            if (!render()) { return; }
            emit rendered();

            /* Wait until next tick, an error or told to pause/terminate */
            do {
                err = clock_nanosleep(CLOCK_REALTIME, TIMER_ABSTIME, &nexttick, NULL);
            } while (err == EINTR && !paused && !abort);

            if (err != 0 && err != EINTR) {
                error = err;
                return;
            }
        }
    }
}

void AnimationLoop::scheduleNextTick(struct timespec * result)
{
    clock_gettime(CLOCK_REALTIME, result);
    result->tv_nsec += 1000000000 / fps;
    if (result->tv_nsec >= 1000000000) {
        result->tv_sec += 1;
        result->tv_nsec -= 1000000000;
    }
}

bool AnimationLoop::render()
{
    /* TODO */
    return true;
}

