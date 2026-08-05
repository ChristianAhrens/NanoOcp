/* Copyright (c) 2026, Christian Ahrens
 *
 * This file is part of NanoOcp <https://github.com/ChristianAhrens/NanoOcp>
 *
 * This library is free software; you can redistribute it and/or modify it under
 * the terms of the GNU Lesser General Public License version 3.0 as published
 * by the Free Software Foundation.
 *
 * This library is distributed in the hope that it will be useful, but WITHOUT
 * ANY WARRANTY; without even the implied warranty of MERCHANTABILITY or FITNESS
 * FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public License for more
 * details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with this library; if not, write to the Free Software Foundation, Inc.,
 * 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.
 */

#include "NanoTimer.h"

namespace NanoOcp1
{

void NanoTimer::startTimer(int intervalMs)
{
    // Serializes m_thread create/join/detach against concurrent startTimer()/stopTimer()
    // calls from other threads. Not held while the worker thread's wait_until() runs, so
    // joining it here can never deadlock against it re-locking m_mutex to wake up.
    std::lock_guard<std::mutex> lifecycleLock(m_lifecycleMutex);

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_intervalMs = intervalMs;
        m_deadline   = std::chrono::steady_clock::now() + std::chrono::milliseconds(intervalMs);

        if (!m_stop && m_thread.joinable())
        {
            // Already running: just push the deadline out, no thread churn.
            m_cv.notify_all();
            return;
        }

        m_stop = false;
    }

    // Not currently running: clean up any leftover thread before spinning up a fresh one.
    if (m_thread.joinable())
    {
        if (m_thread.get_id() != std::this_thread::get_id())
            m_thread.join();
        else
            m_thread.detach(); // reentrant restart from within our own timerCallback()
    }

    m_thread = std::thread([this]() {
        std::unique_lock<std::mutex> lk(m_mutex);
        while (true)
        {
            // Wait until deadline or until stop is signalled.
            const auto waitedDeadline = m_deadline;
            m_cv.wait_until(lk, waitedDeadline, [this, waitedDeadline]() { return m_stop || (m_deadline != waitedDeadline); });
            if (m_stop)
                break;
            if (m_deadline != waitedDeadline)
                continue; // restarted while waiting: loop around and wait on the new deadline

            lk.unlock();
            timerCallback();
            lk.lock();

            if (m_stop)
                break;

            // Reschedule relative to now, unless the callback itself already restarted us.
            if (m_deadline == waitedDeadline)
                m_deadline = std::chrono::steady_clock::now() + std::chrono::milliseconds(m_intervalMs);
        }
    });
}

void NanoTimer::stopTimer()
{
    std::lock_guard<std::mutex> lifecycleLock(m_lifecycleMutex);

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();

    // Only join from outside the timer thread.
    // If stopTimer() is called from within timerCallback(), the thread will
    // exit its loop naturally after the callback returns and can be joined
    // by the destructor or the next startTimer()/stopTimer() call.
    if (m_thread.joinable() && m_thread.get_id() != std::this_thread::get_id())
        m_thread.join();
}

} // namespace NanoOcp1
