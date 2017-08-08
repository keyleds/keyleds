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
#include <errno.h>
#include <chrono>
#include <functional>
#include "tools/AnimationLoop.h"
#include "logging.h"

LOGGING("anim-loop");

AnimationLoop::AnimationLoop(unsigned fps)
    : m_period(1000 / fps),
      m_paused(true),
      m_abort(false),
      m_error(0),
      m_thread(threadEntry, std::ref(*this))
{
}

AnimationLoop::~AnimationLoop()
{}

void AnimationLoop::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_abort.store(true, std::memory_order_relaxed);
        m_cRunStatus.notify_one();
    }

    m_thread.join();
}

void AnimationLoop::setPaused(bool paused)
{
    if (paused != m_paused.load(std::memory_order_relaxed)) {
        std::lock_guard<std::mutex> lock(m_mRunStatus);
        m_paused.store(paused, std::memory_order_relaxed);
        m_cRunStatus.notify_one();
    }
}

void AnimationLoop::run()
{
    DEBUG("AnimationLoop(", this, ") started");
    auto nextDraw = std::chrono::steady_clock::time_point();
    auto period = std::chrono::milliseconds(m_period);

    while (!m_abort.load(std::memory_order_relaxed)) {

        if (!m_paused.load(std::memory_order_relaxed)) {
            auto now = std::chrono::steady_clock::now();
            if (now >= nextDraw) {
                if (!render(m_period)) { break; }

                nextDraw += period;
                if (now >= nextDraw) {
                    nextDraw = now + period;
                }
            }
        }

        std::unique_lock<std::mutex> lock(m_mRunStatus);
        if (m_abort.load(std::memory_order_relaxed)) { break; }

        if (m_paused.load(std::memory_order_relaxed)) {
            DEBUG("AnimationLoop(", this, ") paused");
            m_cRunStatus.wait(lock);
            DEBUG("AnimationLoop(", this, ") resumed");
        } else {
            m_cRunStatus.wait_until(lock, nextDraw);
        }
    }
    DEBUG("AnimationLoop(", this, ") exiting");
}

void AnimationLoop::threadEntry(AnimationLoop & loop)
{
    loop.run();
}
