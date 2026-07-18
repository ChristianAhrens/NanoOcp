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

#pragma once

#include <chrono>
#include <condition_variable>
#include <mutex>
#include <thread>

namespace NanoOcp1
{

/**
 * Minimal periodic timer that mirrors the juce::Timer interface (startTimer /
 * stopTimer / timerCallback).
 *
 * The callback fires on a dedicated background thread at the requested interval.
 * Calling stopTimer() from within timerCallback() is safe: the stop flag is set
 * and the timer thread exits its loop after the callback returns; the actual join
 * happens when stopTimer() is later called from outside the timer thread (or in
 * the destructor).
 */
class NanoTimer
{
public:
    virtual ~NanoTimer() { stopTimer(); }

    NanoTimer(const NanoTimer&)            = delete;
    NanoTimer& operator=(const NanoTimer&) = delete;

    void startTimer(int intervalMs);
    void stopTimer();

    virtual void timerCallback() = 0;

protected:
    NanoTimer() = default;

private:
    std::thread              m_thread;
    std::mutex               m_mutex;
    std::condition_variable  m_cv;
    bool                     m_stop{true};
    int                      m_intervalMs{500};
};

} // namespace NanoOcp1
