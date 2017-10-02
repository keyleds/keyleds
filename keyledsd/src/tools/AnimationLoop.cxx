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
#include "tools/AnimationLoop.h"

#include <cerrno>
#include <chrono>
#include <functional>
#include "logging.h"

LOGGING("anim-loop");

using tools::AnimationLoop;

/****************************************************************************/

AnimationLoop::AnimationLoop(unsigned fps)
    : m_period(1000 / fps),
      m_paused(true),
      m_abort(false),
      m_error(0)
{
}

AnimationLoop::~AnimationLoop()
{}

void AnimationLoop::start()
{
    m_thread = std::thread(threadEntry, std::ref(*this));
}

void AnimationLoop::stop()
{
#ifndef NDEBUG
    auto now = std::chrono::steady_clock::now();
#endif
    {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_abort = true;
        m_cRunStatus.notify_one();
    }

    m_thread.join();
#ifndef NDEBUG
    DEBUG("stop request fulfilled in ",
          std::chrono::duration_cast<std::chrono::microseconds>(
              std::chrono::steady_clock::now() - now
          ).count(), "us");
#endif
}

void AnimationLoop::setPaused(bool paused)
{
    if (paused != m_paused) {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_paused = paused;
        m_cRunStatus.notify_one();
    }
}

/* Some assumptions are made in this loop regarding runstatus:
 * 1) m_abort is a one-time thing, it cannot return to false
 *    once it has been set to true.
 * 2) m_paused does not require precise timing. Its purpose
 *    is only to halt the loop after current iteration.
 */
void AnimationLoop::run()
{
    DEBUG("AnimationLoop(", this, ") started");
    auto now = std::chrono::steady_clock::now();
    auto nextDraw = now;
    const auto period = std::chrono::milliseconds(m_period);

    std::unique_lock<std::mutex> lock(m_mRunStatus);
    for (;;) {
        while (m_paused || now < nextDraw) {
            if (m_abort) {
                DEBUG("AnimationLoop(", this, ") stopped");
                return;
            }
            if (m_paused) {
                DEBUG("AnimationLoop(", this, ") paused");
                m_cRunStatus.wait(lock);
                DEBUG("AnimationLoop(", this, ") resumed");
            } else {
                // Work around libstdc++ bug:
                //    https://gcc.gnu.org/bugzilla/show_bug.cgi?id=41861
                // Actual intention here would be:
                //     m_cRunStatus.wait_until(lock, nextDraw);
                lock.unlock();
                std::this_thread::sleep_for(nextDraw - std::chrono::steady_clock::now());
                lock.lock();
            }
            now = std::chrono::steady_clock::now();
        }

        lock.unlock();
        if (!render(m_period)) { break; }
        lock.lock();

        nextDraw += period;
        if (nextDraw <= now) { nextDraw = now + period; }
    }
    DEBUG("AnimationLoop(", this, ") exiting");
}

void AnimationLoop::threadEntry(AnimationLoop & loop)
{
    loop.run();
}
