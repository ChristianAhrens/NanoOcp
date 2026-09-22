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

#include "NanoTimerScheduler.h"

#include <memory>

namespace NanoOcp1
{

/**
 * Minimal periodic timer that mirrors the juce::Timer interface (startTimer /
 * stopTimer / timerCallback), backed by a shared NanoTimerScheduler.
 *
 * The timer owns no thread of its own: its callback fires on the injected
 * scheduler's single background thread, shared with every other timer on that
 * scheduler. Calling startTimer() (re)schedules a periodic tick; calling it again
 * while running just resets the next deadline. stopTimer() and the destructor block
 * until any in-flight callback has returned, and calling stopTimer() from within
 * timerCallback() (or startTimer()/stopTimer() concurrently from other threads) is
 * safe — the scheduler serializes and self-detects the callback thread.
 *
 * The scheduler is supplied by the owning (higher) layer; NanoOcp provides no
 * default/singleton instance.
 */
class NanoTimer
{
public:
    virtual ~NanoTimer();

    NanoTimer(const NanoTimer&)            = delete;
    NanoTimer& operator=(const NanoTimer&) = delete;

    /**
     * @brief Starts the timer with the specified interval in milliseconds.
     * @param intervalMs Interval in milliseconds between timer callbacks.
     */
    void startTimer(int intervalMs);

    /**
     * @brief Stops the timer.
     */
    void stopTimer();

    /**
     * @brief Callback function that is called when the timer interval elapses.
     * This function must be implemented by derived classes.
     */
    virtual void timerCallback() = 0;

protected:
    /** 
     * @brief Constructs a NanoTimer with the given shared scheduler.
     * @param scheduler Shared scheduler that runs this timer's callback; must not be null. 
     */
    explicit NanoTimer(std::shared_ptr<NanoTimerScheduler> scheduler);

private:
    std::shared_ptr<NanoTimerScheduler> m_scheduler; // Shared scheduler that runs this timer's callback.
    NanoTimerScheduler::TimerId         m_timerId;   // Identifier for this timer within the scheduler.
};

} // namespace NanoOcp1
