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
#include <deque>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <thread>
#include <unordered_map>

namespace NanoOcp1
{
/**
 * @class NanoTimerScheduler
 * @brief One background thread that services many periodic timers and immediate posted tasks.
 *
 * @details Each registered timer costs only a map entry, so thousands of timers (e.g. one per
 *          DeviceProperty in a large matrix) share a single OS thread instead of spawning one
 *          thread each. Callbacks fire on the scheduler thread and therefore run serially: they
 *          must be short and non-blocking, otherwise one callback delays every other timer. Scheduling
 *          is fixed-delay (next deadline set one interval after each callback returns), not fixed-rate.
 *          The same thread also runs one-shot tasks submitted via PostTask() (FIFO; ahead of timers that
 *          are not yet due, and fairly interleaved with already-due timers so neither starves the other).
 *
 *          Cancellation is safe against use-after-free: StopTimer()/DestroyTimer() called from a
 *          thread other than the scheduler thread block until any in-flight callback for that timer
 *          has returned, mirroring the join-on-stop guarantee of NanoOcp1::NanoTimer. Calling them
 *          from within the callback itself does not block (that would self-deadlock).
 *
 * @warning Because that cancel/destroy waits for the callback, never call StopTimer()/DestroyTimer()
 *          (or destroy a timer/owner) while holding a lock the callback also takes — it deadlocks.
 *
 * @note Lifetime: it is safe to release the last shared owner from within a callback (e.g. a disconnect
 *       teardown that drops the controller holding the final reference). The worker thread keeps its own
 *       reference to the shared loop state, so when the scheduler is destroyed on its own thread the
 *       destructor detaches rather than self-joining: it returns at once while the loop unwinds, and any
 *       already-queued tasks still drain on the (now detached) thread. Destroyed from any other thread it
 *       joins as before. Either way, no callback outlives the state it accesses.
 */
class NanoTimerScheduler final
{
public:
    using TimerId = std::uint64_t;

    /** Value never returned by CreateTimer(); denotes "no timer". */
    static constexpr TimerId InvalidTimerId = 0;

    NanoTimerScheduler();

    /**
     * @brief Stops the scheduler thread.
     * @details Joins the thread when destroyed from another thread; when destroyed from within one of its
     *          own callbacks it detaches instead, letting the loop unwind and drain queued tasks on the
     *          detached thread. See the class-level lifetime note.
     */
    ~NanoTimerScheduler();

    /** Non-copyable and non-movable. */
    NanoTimerScheduler(const NanoTimerScheduler&) = delete;
    NanoTimerScheduler& operator=(const NanoTimerScheduler&) = delete;

    /**
     * @brief Registers a timer and its callback without scheduling it yet.
     * @param[in] callback Invoked on the scheduler thread every interval once started. Must not throw;
     *                     an escaping exception is contained (and asserts in debug), never propagated.
     * @return A non-zero id used to start/stop/destroy this timer.
     */
    TimerId CreateTimer(std::function<void()> callback);

    /**
     * @brief (Re)schedules a timer to fire periodically, resetting its next deadline to now + interval.
     * @details The timer keeps firing every @p interval (it is periodic, not one-shot) until StopTimer()
     *          or DestroyTimer() is called; a callback that wants to fire only once calls StopTimer() on
     *          its own id. Non-positive intervals are ignored, matching the existing IRecurringTimer wrappers.
     * @note Fixed-delay, not fixed-rate: the next deadline is set @p interval after each callback returns,
     *       so periods drift under callback/scheduler latency. Fine for retries/timeouts/watchdogs; do not
     *       use where an accurate cadence is required.
     * @param[in] id The identifier of the timer to start.
     * @param[in] interval The period at which the timer should fire.
     */
    void StartTimer(TimerId id, std::chrono::milliseconds interval);

    /**
     * @brief Cancels a timer.
     * @details Blocks until any in-flight callback for it has finished, unless called from within that callback.
     * @warning Don't call while holding a lock the callback also takes — the wait deadlocks.
     * @param[in] id The identifier of the timer to stop.
     */
    void StopTimer(TimerId id);

    /**
     * @brief Cancels and forgets a timer.
     * @details Same in-flight guarantee (and lock-deadlock caveat) as StopTimer(); after this returns 
     *          from outside the callback, the callback is guaranteed not to run again.
     * @param[in] id The identifier of the timer to destroy.
     */
    void DestroyTimer(TimerId id);

    /**
     * @brief Queues a task to run once, as soon as possible, on the scheduler thread.
     * @details Fire-and-forget (no id, not cancellable), FIFO, and serviced ahead of timers that are not yet
     *          due. When timers are already due the thread alternates between posted tasks and due timers, so
     *          neither starves the other. Runs on the same shared thread as timer callbacks, so it must be
     *          short and non-throwing (an escaping exception is contained, and asserts in debug). Ignored once
     *          the scheduler is shutting down.
     * @param[in] task The task to run once on the scheduler thread.
     */
    void PostTask(std::function<void()> task);

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
     * @brief Worker-thread state shared (via std::shared_ptr) by the public scheduler and its own thread.
     * @details The loop state lives here rather than directly in NanoTimerScheduler so the worker thread can
     *          hold a strong reference to it for the whole duration of Run(). If the last public owner is
     *          released on the scheduler thread (destroying NanoTimerScheduler from within one of its own
     *          callbacks), the destructor detaches the thread instead of self-joining and this State stays
     *          alive - kept by the worker's reference - until Run() unwinds, so the loop never touches freed memory.
     */
    struct State
    {
        /// @copydoc NanoTimerScheduler::CreateTimer
        TimerId CreateTimer(std::function<void()> callback);
        /// @copydoc NanoTimerScheduler::StartTimer
        void StartTimer(TimerId id, std::chrono::milliseconds interval);
        /// @copydoc NanoTimerScheduler::StopTimer
        void StopTimer(TimerId id);
        /// @copydoc NanoTimerScheduler::DestroyTimer
        void DestroyTimer(TimerId id);
        /// @copydoc NanoTimerScheduler::PostTask
        void PostTask(std::function<void()> task);

        /**
         * @brief Main loop of the scheduler thread.
         * @details Services due timers and posted tasks until m_running is cleared, then drains any
         *          remaining posted tasks so none is silently dropped.
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

        /**
         * @brief Runs @p task with @p lock released, containing any exception, then re-acquires @p lock.
         * @param[in] lock The unique lock holding m_mutex (held on entry and on return).
         * @param[in] task The task/callback to invoke.
         */
        void InvokeUnlocked(std::unique_lock<std::mutex>& lock, const std::function<void()>& task);

        mutable std::mutex m_mutex;                 //< Protects access to the timer data structures.
        std::condition_variable m_scheduleChanged;  //< Wakes the scheduler thread on schedule change / shutdown.
        std::condition_variable m_callbackDone;     //< Wakes Stop/Destroy waiting on an in-flight callback.

        std::unordered_map<TimerId, Entry> m_timers; //< Index of timers by their identifier.
        std::multimap<std::chrono::steady_clock::time_point, TimerId> m_schedule; //< Timers indexed by their next scheduled deadline.
        std::deque<std::function<void()>> m_immediateTasks; //< One-shot tasks posted via PostTask(); FIFO, ahead of not-yet-due timers and interleaved with due ones.

        TimerId m_nextId{InvalidTimerId};   //< Next available timer identifier.
        TimerId m_firingId{InvalidTimerId}; //< Timer whose callback is currently running (InvalidTimerId = none).
        bool m_serviceTaskNext{false};      //< Fairness toggle: when a posted task and a due timer contend, whose turn is next.
        bool m_running{true};               //< Whether the scheduler thread should keep running.
        std::thread::id m_threadId;         //< Identifier of the scheduler thread (set once at construction).
    };

    std::shared_ptr<State> m_state; //< Shared loop state; the worker thread holds its own reference (see State).
    std::thread m_thread;           //< The scheduler thread.
};
} // namespace NanoOcp1
