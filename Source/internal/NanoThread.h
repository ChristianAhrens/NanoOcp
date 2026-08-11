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

#include <atomic>
#include <chrono>
#include <string>
#include <thread>

#if defined(_WIN32) || defined(_WIN64)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
#elif defined(__APPLE__) || defined(__linux__) || defined(__unix__)
  #include <pthread.h>
  #include <sched.h>
#endif

namespace NanoOcp1
{

enum class ThreadPriority { low, normal, high, realtime };

/**
 * Minimal std::thread wrapper that mirrors the juce::Thread interface used
 * inside NanoOcp1: startThread / stopThread / threadShouldExit / signalThreadShouldExit / wait.
 */
class NanoThread
{
public:
    explicit NanoThread(const std::string& /*name*/) {}

    virtual ~NanoThread()
    {
        // Derived classes must ensure stopThread() was called before reaching here.
    }

    NanoThread(const NanoThread&)            = delete;
    NanoThread& operator=(const NanoThread&) = delete;

    void startThread(ThreadPriority priority = ThreadPriority::normal)
    {
        m_shouldExit.store(false);
        m_running.store(true);
        m_thread = std::thread([this, priority]() {
            applyPriority(priority);
            run();
            m_running.store(false);
        });
    }

    void stopThread(int timeoutMs)
    {
        m_shouldExit.store(true);
        if (!m_thread.joinable())
            return;

        // Poll until thread exits or timeout expires, then join unconditionally.
        auto deadline = std::chrono::steady_clock::now()
                      + std::chrono::milliseconds(timeoutMs);
        while (m_running.load() && std::chrono::steady_clock::now() < deadline)
            std::this_thread::sleep_for(std::chrono::milliseconds(5));

        m_thread.join();
    }

    bool threadShouldExit() const noexcept { return m_shouldExit.load(); }
    void signalThreadShouldExit()          { m_shouldExit.store(true); }

    void wait(int ms)
    {
        std::this_thread::sleep_for(std::chrono::milliseconds(ms));
    }

    virtual void run() = 0;

private:
    std::thread          m_thread;
    std::atomic<bool>    m_shouldExit{false};
    std::atomic<bool>    m_running{false};

    static void applyPriority(ThreadPriority priority)
    {
#if defined(_WIN32) || defined(_WIN64)
        int wp = THREAD_PRIORITY_NORMAL;
        switch (priority)
        {
            case ThreadPriority::low:      wp = THREAD_PRIORITY_BELOW_NORMAL; break;
            case ThreadPriority::high:     wp = THREAD_PRIORITY_ABOVE_NORMAL; break;
            case ThreadPriority::realtime: wp = THREAD_PRIORITY_HIGHEST;      break;
            default: break;
        }
        SetThreadPriority(GetCurrentThread(), wp);
#elif defined(_POSIX_THREAD_PRIORITY_SCHEDULING) && _POSIX_THREAD_PRIORITY_SCHEDULING > 0
        int policy = SCHED_OTHER;
        sched_param param{};
        switch (priority)
        {
            case ThreadPriority::realtime:
                policy = SCHED_FIFO;
                param.sched_priority = sched_get_priority_max(SCHED_FIFO);
                break;
            case ThreadPriority::high:
                param.sched_priority = sched_get_priority_max(SCHED_OTHER) / 2;
                break;
            default:
                break;
        }
        pthread_setschedparam(pthread_self(), policy, &param);
#else
        (void)priority;
#endif
    }
};

} // namespace NanoOcp1
