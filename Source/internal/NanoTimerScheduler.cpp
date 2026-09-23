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

#include "NanoTimerScheduler.h"

#include <cassert>


namespace NanoOcp1
{
NanoTimerScheduler::NanoTimerScheduler()
{
    m_thread = std::thread([this]() { Run(); }); // Start the scheduler thread
    m_threadId = m_thread.get_id();
}

NanoTimerScheduler::~NanoTimerScheduler()
{
    // Lifetime contract: the last shared owner must be released off the scheduler thread. Destroying the
    // scheduler from within its own callback would run this destructor on m_thread, making join() a fatal self-join.
    assert(std::this_thread::get_id() != m_threadId
           && "NanoTimerScheduler destroyed from its own callback thread (self-join); keep it owned outside its callbacks.");

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        m_running = false;
    }
    m_scheduleChanged.notify_all(); // Notify the scheduler thread to wake up and exit.

    if (m_thread.joinable())
        m_thread.join();
}

NanoTimerScheduler::TimerId NanoTimerScheduler::CreateTimer(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const TimerId id = ++m_nextId;
    m_timers[id].callback = std::move(callback);
    return id;
}

void NanoTimerScheduler::StartTimer(TimerId id, std::chrono::milliseconds interval)
{
    // Mirror the existing IRecurringTimer wrappers, which ignore non-positive intervals.
    if (interval.count() <= 0)
        return;

    std::lock_guard<std::mutex> lock(m_mutex);

    // Ensure the timer exists before attempting to start it.
    auto it = m_timers.find(id);
    if (it == m_timers.end())
        return;

    // Unschedule the timer if it was previously scheduled.
    Unschedule(id);

    const auto deadline = std::chrono::steady_clock::now() + interval;
    it->second.interval = interval;
    it->second.deadline = deadline;
    it->second.enabled = true;
    it->second.scheduled = true;
    m_schedule.emplace(deadline, id);

    m_scheduleChanged.notify_all(); // Notify the scheduler thread that the schedule has changed.
}

void NanoTimerScheduler::StopTimer(TimerId id)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_timers.find(id);
    if (it == m_timers.end())
        return;

    Unschedule(id);
    it->second.enabled = false;

    WaitForCallbackToFinish(lock, id); // Wait until any in-flight callback for this timer has finished.
    m_scheduleChanged.notify_all(); // Notify the scheduler thread that the schedule has changed.
}

void NanoTimerScheduler::DestroyTimer(TimerId id)
{
    std::unique_lock<std::mutex> lock(m_mutex);

    auto it = m_timers.find(id);
    if (it == m_timers.end())
        return;

    Unschedule(id);
    it->second.enabled = false;

    WaitForCallbackToFinish(lock, id); // Wait until any in-flight callback for this timer has finished.

    m_timers.erase(id);
    m_scheduleChanged.notify_all(); // Notify the scheduler thread that the schedule has changed.
}

void NanoTimerScheduler::Unschedule(TimerId id)
{
    // If the timer does not exist or is not currently scheduled, there is nothing to do.
    auto it = m_timers.find(id);
    if (it == m_timers.end() || !it->second.scheduled)
        return;

    // m_schedule is a multimap keyed by deadline, so several timers can share the same deadline;
    // equal_range yields all the entries at this timer's deadline.
    auto range = m_schedule.equal_range(it->second.deadline);

    // Scan that range for the entry belonging to this id and erase only it — erasing by the deadline
    // key alone would remove every timer that happens to share this deadline.
    for (auto sit = range.first; sit != range.second; ++sit)
    {
        if (sit->second == id)
        {
            m_schedule.erase(sit);
            break;
        }
    }
    it->second.scheduled = false;
}

void NanoTimerScheduler::WaitForCallbackToFinish(std::unique_lock<std::mutex>& lock, TimerId id)
{
    // A callback that cancels its own timer runs on the scheduler thread; it can never wait for itself.
    if (std::this_thread::get_id() == m_threadId)
        return;

    // Block until this timer is no longer the one firing, so any in-flight callback has fully
    // returned before Stop/Destroy proceeds. The wait releases m_mutex, letting the scheduler thread
    // finish the callback, reset m_firingId, and notify m_callbackDone.
    m_callbackDone.wait(lock, [this, id]() { return m_firingId != id; });
}

void NanoTimerScheduler::Run()
{
    // Held only during short bookkeeping: the wait/wait_until calls below release m_mutex while idle
    // or sleeping, and we unlock explicitly around each callback, so other methods can take the lock.
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_running)
    {
        if (m_schedule.empty())
        {
            // Wait (and unlock m_mutex) until there is a timer scheduled or the scheduler is shutting down.
            m_scheduleChanged.wait(lock, [this]() { return !m_running || !m_schedule.empty(); });
            continue;
        }

        const auto earliest = m_schedule.begin()->first;
        if (std::chrono::steady_clock::now() < earliest)
        {
            // Not yet reached the earliest deadline; wait (and unlock m_mutex) until it arrives or the schedule changes.
            // Wake early if we are shutting down or a sooner deadline appears.
            m_scheduleChanged.wait_until(lock, earliest, [this, earliest]() {
                return !m_running || m_schedule.empty() || m_schedule.begin()->first < earliest;
            });
            continue;
        }

        const TimerId id = m_schedule.begin()->second;
        m_schedule.erase(m_schedule.begin());

        auto it = m_timers.find(id);
        if (it == m_timers.end() || !it->second.enabled)
            continue; // Cancelled or destroyed after it was scheduled.

        it->second.scheduled = false; // Mark as no longer scheduled since we are about to run its callback.
        auto callback = it->second.callback; // Copy so it stays valid while we run it unlocked.
        const auto interval = it->second.interval;

        // Mark this timer as currently firing. Drop the lock only while actually running a callback,
        // so an empty callback needs no unlock/relock.
        m_firingId = id;
        if (callback)
        {
            lock.unlock();
            callback();
            lock.lock();
        }
        m_firingId = InvalidTimerId;
        m_callbackDone.notify_all();

        // Reschedule for periodic firing, unless the callback stopped, destroyed or restarted this timer.
        it = m_timers.find(id);
        if (it != m_timers.end() && it->second.enabled && !it->second.scheduled && interval.count() > 0)
        {
            const auto next = std::chrono::steady_clock::now() + interval;
            it->second.deadline = next;
            it->second.scheduled = true;
            m_schedule.emplace(next, id);
        }
    }
}
} // namespace NanoOcp1
