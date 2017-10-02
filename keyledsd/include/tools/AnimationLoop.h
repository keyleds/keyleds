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
#ifndef TOOLS_ANIM_LOOP_H_A32C4648
#define TOOLS_ANIM_LOOP_H_A32C4648

/****************************************************************************/

#include <condition_variable>
#include <mutex>
#include <thread>

namespace tools {

/****************************************************************************/


/** Generic thread-based animation loop
 *
 * Starts a thread that invokes a virtual method at a predefined frequency.
 * Supports asynchronous pausing and resuming, and synchronous stop().
 *
 * The loop starts in paused state. That is, the run method starts immediately
 * but goes into sleep without calling render until setPaused(false) is called.
 *
 * The loop must be stopped before the object is deleted.
 */
class AnimationLoop
{
public:
                    AnimationLoop(unsigned fps);
    virtual         ~AnimationLoop();

    bool            paused() const { return m_paused; }
    int             error() const { return m_error; }

    void            start();
    void            setPaused(bool paused);
    void            stop();

protected:
    virtual void    run();
    virtual bool    render(unsigned long) = 0;

private:
    /// Simply calls the animation loop's run method
    static void     threadEntry(AnimationLoop &);

private:
    std::mutex      m_mRunStatus;           ///< Controls access to m_cRunStats, m_paused and m_abort
    std::condition_variable m_cRunStatus;   ///< Used to wait on m_paused and m_abort changes

    unsigned        m_period;               ///< Animation period in milliseconds
    bool            m_paused;               ///< If set, the animation loop thread goes into sleep
    bool            m_abort;                ///< If set, the animation loop thread exits
    int             m_error;                ///< Error code from animation loop thread, errno-style

    std::thread     m_thread;               ///< Actual thread instance
};

/****************************************************************************/

}

#endif
