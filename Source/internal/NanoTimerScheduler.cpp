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
    : m_state(std::make_shared<State>())
{
    // The worker captures its OWN strong reference to the shared state, so the loop can unwind safely even
    // if the last public owner is released on this thread (the destructor then detaches instead of joining).
    m_thread = std::thread([state = m_state]() { state->Run(); });
    m_state->m_threadId = m_thread.get_id();
}

NanoTimerScheduler::~NanoTimerScheduler()
{
    {
        std::lock_guard<std::mutex> lock(m_state->m_mutex);
        m_state->m_running = false;
    }
    m_state->m_scheduleChanged.notify_all(); // Wake the scheduler thread so it observes shutdown and exits.

    // If the last owner was released on the scheduler thread (a callback dropping the final reference) this
    // runs on m_thread; joining ourselves would throw std::system_error -> std::terminate. Detach instead:
    // the worker still references m_state, so Run() unwinds and frees the state on the worker thread.
    if (std::this_thread::get_id() == m_thread.get_id())
        m_thread.detach();
    else if (m_thread.joinable())
        m_thread.join();
}

NanoTimerScheduler::TimerId NanoTimerScheduler::CreateTimer(std::function<void()> callback)
{
    return m_state->CreateTimer(std::move(callback));
}

void NanoTimerScheduler::StartTimer(TimerId id, std::chrono::milliseconds interval)
{
    m_state->StartTimer(id, interval);
}

void NanoTimerScheduler::StopTimer(TimerId id)
{
    m_state->StopTimer(id);
}

void NanoTimerScheduler::DestroyTimer(TimerId id)
{
    m_state->DestroyTimer(id);
}

void NanoTimerScheduler::PostTask(std::function<void()> task)
{
    m_state->PostTask(std::move(task));
}

NanoTimerScheduler::TimerId NanoTimerScheduler::State::CreateTimer(std::function<void()> callback)
{
    std::lock_guard<std::mutex> lock(m_mutex);

    const TimerId id = ++m_nextId;
    m_timers[id].callback = std::move(callback);
    return id;
}

void NanoTimerScheduler::State::StartTimer(TimerId id, std::chrono::milliseconds interval)
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

void NanoTimerScheduler::State::PostTask(std::function<void()> task)
{
    if (!task)
        return;

    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_running)
            return; // Reject once shutting down.

        m_immediateTasks.push_back(std::move(task));
    }

    m_scheduleChanged.notify_all(); // Wake the scheduler thread to run the task.
}

void NanoTimerScheduler::State::StopTimer(TimerId id)
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

void NanoTimerScheduler::State::DestroyTimer(TimerId id)
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

void NanoTimerScheduler::State::Unschedule(TimerId id)
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

void NanoTimerScheduler::State::WaitForCallbackToFinish(std::unique_lock<std::mutex>& lock, TimerId id)
{
    // A callback that cancels its own timer runs on the scheduler thread; it can never wait for itself.
    if (std::this_thread::get_id() == m_threadId)
        return;

    // Block until this timer is no longer the one firing, so any in-flight callback has fully
    // returned before Stop/Destroy proceeds. The wait releases m_mutex, letting the scheduler thread
    // finish the callback, reset m_firingId, and notify m_callbackDone.
    m_callbackDone.wait(lock, [this, id]() { return m_firingId != id; });
}

void NanoTimerScheduler::State::InvokeUnlocked(std::unique_lock<std::mutex>& lock, const std::function<void()>& task)
{
    lock.unlock();
    try
    {
        task();
    }
    catch (...)
    {
        // A throwing callback/task must not kill the shared scheduler thread (stopping every other
        // timer) nor leave scheduler state inconsistent. Contain it here.
        assert(false && "NanoTimerScheduler: callback/task threw; must not throw.");
    }
    lock.lock();
}

void NanoTimerScheduler::State::Run()
{
    // Held only during short bookkeeping: the wait/wait_until calls below release m_mutex while idle
    // or sleeping, and we unlock explicitly around each callback, so other methods can take the lock.
    std::unique_lock<std::mutex> lock(m_mutex);

    while (m_running)
    {
        const bool haveTask = !m_immediateTasks.empty();
        const bool timerDue = !m_schedule.empty() && std::chrono::steady_clock::now() >= m_schedule.begin()->first;

        // A posted task and an already-due timer both want this single thread. Servicing either one to
        // exhaustion starves the other: a continuously non-empty task queue (e.g. sustained socket traffic)
        // would block due reconnect/GetValues-timeout timers, while a backlog of perpetually-due timers
        // (e.g. many devices reconnecting at once) would block posted tasks such as the OnDeviceLost
        // dispatched via PostTask. So when both are ready we alternate (m_serviceTaskNext); when only one
        // side has ready work we run it. With no due timer, posted tasks run ASAP, ahead of future timers.
        const bool serviceTask = haveTask && (!timerDue || m_serviceTaskNext);
        if (haveTask && timerDue)
            m_serviceTaskNext = !serviceTask; // flip only while contending, so the other side goes next

        if (serviceTask)
        {
            auto task = std::move(m_immediateTasks.front());
            m_immediateTasks.pop_front();
            if (task)
                InvokeUnlocked(lock, task);
            continue;
        }

        if (!timerDue)
        {
            if (m_schedule.empty())
            {
                // Wait (and unlock m_mutex) until there is a timer scheduled, a task is posted, or the scheduler is shutting down.
                m_scheduleChanged.wait(lock, [this]() { return !m_running || !m_schedule.empty() || !m_immediateTasks.empty(); });
            }
            else
            {
                const auto earliest = m_schedule.begin()->first;
                // Not yet reached the earliest deadline; wait (and unlock m_mutex) until it arrives or the schedule changes.
                // Wake early if we are shutting down, a sooner deadline appears, or a task is posted.
                m_scheduleChanged.wait_until(lock, earliest, [this, earliest]() {
                    return !m_running || m_schedule.empty() || m_schedule.begin()->first < earliest || !m_immediateTasks.empty();
                });
            }
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
            InvokeUnlocked(lock, callback);
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

    // Shutdown: drain tasks queued before stop so none is silently dropped (parity with the old
    // dispatcher). Timers are intentionally not run on shutdown; PostTask() rejects new tasks now.
    while (!m_immediateTasks.empty())
    {
        auto task = std::move(m_immediateTasks.front());
        m_immediateTasks.pop_front();
        if (task)
            InvokeUnlocked(lock, task);
    }
}
} // namespace NanoOcp1
