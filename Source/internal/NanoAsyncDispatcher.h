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

#include <condition_variable>
#include <deque>
#include <functional>
#include <mutex>
#include <thread>

namespace NanoOcp1
{

/**
 * @class NanoAsyncDispatcher
 * @brief Single-worker-thread task queue used to decouple callback execution
 * from the thread that posts the callback.
 *
 * Replacement for the `juce::MessageManager::callAsync()` / posted-message
 * mechanism the pre-refactor implementation relied on to marshal callbacks
 * onto the JUCE message thread. Since this library no longer depends on
 * JUCE, there is no generic "the app's message thread" to marshal onto —
 * instead, posted tasks run on a dedicated background thread owned by this
 * object, off of whichever thread called post() (typically the socket I/O
 * thread).
 *
 * Tasks queued before stop() is called are guaranteed to run (in FIFO order)
 * before the worker thread exits; no task is ever silently dropped once
 * accepted by post(). Tasks submitted after stop() has begun are rejected.
 */
class NanoAsyncDispatcher
{
public:
    NanoAsyncDispatcher();
    ~NanoAsyncDispatcher();

    NanoAsyncDispatcher(const NanoAsyncDispatcher&)            = delete;
    NanoAsyncDispatcher& operator=(const NanoAsyncDispatcher&) = delete;

    /**
     * @brief Queues a task for asynchronous execution on the worker thread.
     * Silently ignored if the dispatcher is stopping/stopped.
     */
    void post(std::function<void()> task);

    /**
     * @brief Stops accepting new tasks and joins the worker thread once all
     * already-queued tasks have run. Safe to call from within a task running
     * on the worker thread itself (detaches instead of joining to avoid a
     * self-join deadlock). Idempotent.
     */
    void stop();

private:
    void run();

    std::thread                       m_thread;
    std::mutex                        m_mutex;
    std::condition_variable           m_cv;
    std::deque<std::function<void()>> m_queue;
    bool                              m_accepting{true};
};

} // namespace NanoOcp1
