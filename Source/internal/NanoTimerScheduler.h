/* Copyright (c) 2026, Bernardo Escalona
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
#include <cstdint>
#include <functional>
#include <map>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace NanoOcp1
{
/**
 * @class NanoTimerScheduler
 * @brief One background thread that services many periodic timers.
 *
 * @details Each registered timer costs only a map entry, so thousands of timers (e.g. one per
 *          DeviceProperty in a large matrix) share a single OS thread instead of spawning one
 *          thread each. Callbacks fire on the scheduler thread and therefore run serially: they
 *          must be short and non-blocking, otherwise one callback delays every other timer.
 *
 *          Cancellation is safe against use-after-free: StopTimer()/DestroyTimer() called from a
 *          thread other than the scheduler thread block until any in-flight callback for that timer
 *          has returned, mirroring the join-on-stop guarantee of NanoOcp1::NanoTimer. Calling them
 *          from within the callback itself does not block (that would self-deadlock).
 */
class NanoTimerScheduler final
{
public:
    using TimerId = std::uint64_t;

    /** Value never returned by CreateTimer(); denotes "no timer". */
    static constexpr TimerId InvalidTimerId = 0;

    NanoTimerScheduler();
    ~NanoTimerScheduler();

    /** Non-copyable and non-movable. */
    NanoTimerScheduler(const NanoTimerScheduler&) = delete;
    NanoTimerScheduler& operator=(const NanoTimerScheduler&) = delete;

    /**
     * @brief Registers a timer and its callback without scheduling it yet.
     * @param[in] callback Invoked on the scheduler thread every interval once started.
     * @return A non-zero id used to start/stop/destroy this timer.
     */
    TimerId CreateTimer(std::function<void()> callback);

    /**
     * @brief (Re)schedules a timer to fire periodically, resetting its next deadline to now + interval.
     * @details The timer keeps firing every @p interval (it is periodic, not one-shot) until StopTimer()
     *          or DestroyTimer() is called; a callback that wants to fire only once calls StopTimer() on
     *          its own id. Non-positive intervals are ignored, matching the existing IRecurringTimer wrappers.
     * @param[in] id The identifier of the timer to start.
     * @param[in] interval The period at which the timer should fire.
     */
    void StartTimer(TimerId id, std::chrono::milliseconds interval);

    /**
     * @brief Cancels a timer.
     * @details Blocks until any in-flight callback for it has finished, unless called from within that callback.
     * @param[in] id The identifier of the timer to stop.
     */
    void StopTimer(TimerId id);

    /**
     * @brief Cancels and forgets a timer.
     * @details Same in-flight guarantee as StopTimer(); after this returns from outside the callback, the callback is guaranteed not to run again.
     * @param[in] id The identifier of the timer to destroy.
     */
    void DestroyTimer(TimerId id);

private:
    /**
     * @brief Represents an entry for a single timer in the scheduler.
     * @details @c enabled and @c scheduled answer different questions and can disagree while a callback is running:
     *          - @c enabled is intent — "should this timer keep firing?" — set by StartTimer, cleared by StopTimer/DestroyTimer.
     *          - @c scheduled is bookkeeping — "is this entry currently present in m_schedule?".
     *          While a timer fires, Run() has popped its entry from m_schedule (scheduled=false) but
     *          it is still enabled, so the periodic re-arm happens only when enabled && !scheduled:
     *          still wanted, and not already re-inserted by a callback that restarted itself.
     */
    struct Entry
    {
        std::chrono::milliseconds interval{0};              //< Interval at which the timer fires.
        std::chrono::steady_clock::time_point deadline{};   //< Next scheduled deadline for the timer.
        std::function<void()> callback;                     //< Callback to invoke when the timer fires.
        bool enabled{false};                                //< Intent: should keep firing. Set by StartTimer, cleared by Stop/Destroy.
        bool scheduled{false};                              //< Bookkeeping: currently present in m_schedule.
    };

    /**
     * @brief Main loop of the scheduler thread.
     * @details Continuously checks for timers that have reached their deadline and executes their callbacks.
     */
    void Run();

    /** 
     * @brief Removes a timer from the deadline index.
     * @details Requires m_mutex held.
     * @param[in] id The identifier of the timer to unschedule.
     */
    void Unschedule(TimerId id);

    /** 
     * @brief Waits until no callback for the specified timer is running.
     * @details Returns immediately when called on the scheduler thread. Requires m_mutex held via @p lock.
     * @param[in] lock The unique lock holding m_mutex.
     * @param[in] id The identifier of the timer to wait for.
     */
    void WaitForCallbackToFinish(std::unique_lock<std::mutex>& lock, TimerId id);

    mutable std::mutex m_mutex;                 //< Protects access to the timer data structures.
    std::condition_variable m_scheduleChanged;  //< Wakes the scheduler thread on schedule change / shutdown.
    std::condition_variable m_callbackDone;     //< Wakes Stop/Destroy waiting on an in-flight callback.

    std::unordered_map<TimerId, Entry> m_timers; //< Index of timers by their identifier.
    std::multimap<std::chrono::steady_clock::time_point, TimerId> m_schedule; //< Timers indexed by their next scheduled deadline.

    TimerId m_nextId{InvalidTimerId};   //< Next available timer identifier.
    TimerId m_firingId{InvalidTimerId}; //< Timer whose callback is currently running (InvalidTimerId = none).
    bool m_running{true};               //< Indicates whether the scheduler thread should keep running.

    std::thread m_thread;               //< The scheduler thread.
    std::thread::id m_threadId;         //< Identifier of the scheduler thread.
};
} // namespace NanoOcp1
