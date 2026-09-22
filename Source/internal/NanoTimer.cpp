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

NanoTimer::NanoTimer(std::shared_ptr<NanoTimerScheduler> scheduler)
    : m_scheduler(std::move(scheduler)),
      m_timerId(m_scheduler->CreateTimer([this]() { timerCallback(); }))
{
}

NanoTimer::~NanoTimer()
{
    // Blocks until any in-flight callback has returned, then removes this timer from the scheduler.
    m_scheduler->DestroyTimer(m_timerId);
}

void NanoTimer::startTimer(int intervalMs)
{
    m_scheduler->StartTimer(m_timerId, std::chrono::milliseconds(intervalMs));
}

void NanoTimer::stopTimer()
{
    m_scheduler->StopTimer(m_timerId);
}

} // namespace NanoOcp1
