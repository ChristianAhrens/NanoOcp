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

#include "NanoAsyncDispatcher.h"

namespace NanoOcp1
{

NanoAsyncDispatcher::NanoAsyncDispatcher()
{
    m_thread = std::thread([this]() { run(); });
}

NanoAsyncDispatcher::~NanoAsyncDispatcher()
{
    stop();
}

void NanoAsyncDispatcher::post(std::function<void()> task)
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_accepting)
            return;
        m_queue.push_back(std::move(task));
    }
    m_cv.notify_one();
}

void NanoAsyncDispatcher::stop()
{
    {
        std::lock_guard<std::mutex> lock(m_mutex);
        if (!m_accepting)
            return; // idempotent
        m_accepting = false;
    }
    m_cv.notify_one();

    if (!m_thread.joinable())
        return;

    // A task running on the worker thread may itself trigger a teardown path
    // that calls stop() (e.g. an async-dispatched onConnectionLost callback
    // calling disconnect()). Joining from within the worker thread would
    // deadlock, so detach instead — the thread finishes its current task,
    // observes m_accepting == false with an empty queue, and exits on its own.
    if (std::this_thread::get_id() == m_thread.get_id())
        m_thread.detach();
    else
        m_thread.join();
}

void NanoAsyncDispatcher::run()
{
    for (;;)
    {
        std::function<void()> task;
        {
            std::unique_lock<std::mutex> lock(m_mutex);
            m_cv.wait(lock, [this]() { return !m_accepting || !m_queue.empty(); });

            if (m_queue.empty())
                return; // nothing left to run and no longer accepting new tasks

            task = std::move(m_queue.front());
            m_queue.pop_front();
        }
        task();
    }
}

} // namespace NanoOcp1
