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
    // Stop any already-running timer first.
    // Safe to call here since we are NOT on the timer thread.
    stopTimer();

    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stop       = false;
        m_intervalMs = intervalMs;
    }

    m_thread = std::thread([this]() {
        while (true)
        {
            std::unique_lock<std::mutex> lk(m_mutex);
            // Wait for the interval or until stop is signalled.
            m_cv.wait_for(lk,
                          std::chrono::milliseconds(m_intervalMs),
                          [this]() { return m_stop; });
            if (m_stop)
                break;
            lk.unlock();
            timerCallback();
            // After callback, loop around and check m_stop again before waiting.
        }
    });
}

void NanoTimer::stopTimer()
{
    {
        std::lock_guard<std::mutex> lk(m_mutex);
        m_stop = true;
    }
    m_cv.notify_all();

    // Only join from outside the timer thread.
    // If stopTimer() is called from within timerCallback(), the thread will
    // exit its loop naturally after the callback returns and can be joined
    // by the destructor or the next startTimer() call.
    if (m_thread.joinable() && m_thread.get_id() != std::this_thread::get_id())
        m_thread.join();
}

} // namespace NanoOcp1
