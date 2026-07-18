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

#include "Ocp1Controller.h"

#include "Ocp1Message.h"

#include <algorithm>
#include <cassert>


namespace NanoOcp1
{


// ── Construction / destruction ────────────────────────────────────────────────

Ocp1Controller::Ocp1Controller() = default;

Ocp1Controller::~Ocp1Controller()
{
    disconnect();
}


// ── Object registration ───────────────────────────────────────────────────────

void Ocp1Controller::trackObject(std::unique_ptr<Ocp1CommandDefinition> def, ValueCallback cb)
{
    // Must not be called from within a tracked-object callback (no re-entrant iteration).
    const std::uint32_t ono = def->m_targetOno;
    m_onoToIdx[ono] = m_trackedObjects.size();
    m_trackedObjects.push_back({ std::move(def), std::move(cb) });
}

void Ocp1Controller::clearTrackedObjects()
{
    // Must not be called from within a tracked-object callback (no re-entrant iteration).
    m_trackedObjects.clear();
    m_onoToIdx.clear();
}


// ── Connection lifecycle ──────────────────────────────────────────────────────

void Ocp1Controller::connect(const std::string& host, int port, int timeoutMs)
{
    if (m_state != State::Disconnected)
        return;

    m_host      = host;
    m_port      = port;
    m_timeoutMs = timeoutMs;

    m_client = std::make_unique<NanoOcp1Client>(host, port, /*callbacksOnMessageThread=*/false);

    m_client->onConnectionEstablished = [this]() {
        afterConnected();
    };

    m_client->onConnectionLost = [this]() {
        stopTimer();
        clearPendingHandles();
        // If disconnect() was called first, state is already Disconnected —
        // don't transition back to Connecting.  Otherwise the client retries
        // automatically and we reflect that in the state.
        if (m_state != State::Disconnected)
            setState(State::Connecting);
    };

    m_client->onDataReceived = [this](const ByteVector& data) {
        return processMessage(data);
    };

    setState(State::Connecting);
    m_client->start();
}

void Ocp1Controller::disconnect()
{
    // Set state first so that any callback that checks m_state (e.g. the
    // onConnectionLost lambda) sees Disconnected and does not re-enter Connecting.
    setState(State::Disconnected);

    if (m_client)
    {
        // Join the socket thread and the reconnect-retry timer thread BEFORE
        // stopping the GetValues timer and BEFORE nulling the callbacks.
        //
        // In Soundscape mode the socket thread calls queryObjectValues() (and
        // therefore NanoTimer::startTimer()) when the device GUID response
        // arrives.  If stopTimer() (which touches NanoTimer::m_thread) runs
        // concurrently with startTimer() on the socket thread, the two race on
        // m_thread — a std::thread object that is not protected by any mutex.
        // Joining the socket thread first (via stop()) guarantees it is dead
        // before stopTimer() ever looks at m_thread.
        //
        // Similarly, the reconnect-retry timer thread can be mid-way through
        // calling onConnectionEstablished when the main thread nulls it.
        // Joining that timer first (also via stop()) eliminates the race.
        m_client->stop();

        m_client->onConnectionEstablished = {};
        m_client->onConnectionLost        = {};
        m_client->onDataReceived          = {};
        m_client.reset();
    }

    // Safe: socket thread is guaranteed dead, so no concurrent startTimer()
    // call can race with this stopTimer().
    stopTimer();
    clearPendingHandles();
}


// ── SetValue ──────────────────────────────────────────────────────────────────

bool Ocp1Controller::setValue(const Ocp1CommandDefinition& def, const Variant& value)
{
    if (!m_client || m_state != State::Connected)
        return false;

    std::uint32_t handle{0};
    const bool ok = m_client->sendData(
        Ocp1CommandResponseRequired(def.SetValueCommand(value), handle).GetSerializedData());
    if (ok)
        addPendingSetValueHandle(handle, def.m_targetOno);
    return ok;
}


// ── Subscribe / query ─────────────────────────────────────────────────────────

void Ocp1Controller::afterConnected()
{
    createObjectSubscriptions();
    queryObjectValues();
}

void Ocp1Controller::onUntrackedGetValueResponse(std::uint32_t /*ono*/, const ByteVector& /*paramData*/)
{
    // Default: do nothing.  SoundscapeController overrides this to handle the GUID response.
}

bool Ocp1Controller::createObjectSubscriptions()
{
    if (!m_client || m_state == State::Disconnected)
        return false;

    bool success = true;
    for (const auto& tracked : m_trackedObjects)
    {
        std::uint32_t handle{0};
        success = m_client->sendData(
            Ocp1CommandResponseRequired(
                tracked.def->AddSubscriptionCommand(), handle).GetSerializedData()
        ) && success;
        addPendingSubscriptionHandle(handle);
    }

    if (hasPendingSubscriptions())
        setState(State::Subscribing);

    return success;
}

bool Ocp1Controller::queryObjectValues()
{
    if (!m_client || m_state == State::Disconnected)
        return false;

    bool success = true;
    for (const auto& tracked : m_trackedObjects)
        success = queryObjectValue(*tracked.def) && success;

    if (hasPendingGetValues())
    {
        setState(State::GetValues);
        startTimer(m_timeoutMs * 20);
    }
    else
    {
        // No tracked objects (or all queries failed to send) — skip GetValues.
        stopTimer();
        if (!hasPendingSubscriptions())
            setState(State::Connected);
    }

    return success;
}

bool Ocp1Controller::queryObjectValue(const Ocp1CommandDefinition& def)
{
    if (!m_client)
        return false;

    std::uint32_t handle{0};
    const bool ok = m_client->sendData(
        Ocp1CommandResponseRequired(def.GetValueCommand(), handle).GetSerializedData());
    if (ok)
        addPendingGetValueHandle(handle, def.m_targetOno);
    return ok;
}


// ── State management ──────────────────────────────────────────────────────────

void Ocp1Controller::setState(State s)
{
    if (m_state.exchange(s) == s)
        return;
    if (onStateChanged)
        onStateChanged(s);
}


// ── GetValues timeout ─────────────────────────────────────────────────────────

void Ocp1Controller::timerCallback()
{
    retryPendingGetValues();
}

void Ocp1Controller::retryPendingGetValues()
{
    // Collect stale ONos under the lock, then clear the map so re-issued
    // queries register fresh handles.
    std::vector<std::uint32_t> staleOnos;
    {
        std::lock_guard<std::mutex> lk(m_pendingMutex);
        staleOnos.reserve(m_pendingGetValueHandles.size());
        for (const auto& [handle, ono] : m_pendingGetValueHandles)
            staleOnos.push_back(ono);
        m_pendingGetValueHandles.clear();
    }

    if (staleOnos.empty())
    {
        stopTimer();
        if (!hasPendingSubscriptions())
            setState(State::Connected);
        else
            setState(State::Subscribing);
        return;
    }

    for (const auto ono : staleOnos)
    {
        auto it = m_onoToIdx.find(ono);
        if (it != m_onoToIdx.end())
            queryObjectValue(*m_trackedObjects[it->second].def);
    }
}


// ── Message dispatch ──────────────────────────────────────────────────────────

bool Ocp1Controller::processMessage(const ByteVector& data)
{
    auto msg = Ocp1Message::UnmarshalOcp1Message(data);
    if (!msg)
        return false;

    switch (msg->GetMessageType())
    {
    case Ocp1Message::Notification:
    {
        const auto* notif = static_cast<Ocp1Notification*>(msg.get());
        const auto  it    = m_onoToIdx.find(notif->GetEmitterOno());
        if (it == m_onoToIdx.end())
            return false;
        const auto& tracked = m_trackedObjects[it->second];
        if (tracked.cb)
            tracked.cb(notif->GetParameterData());
        return true;
    }

    case Ocp1Message::Response:
    {
        const auto* resp   = static_cast<Ocp1Response*>(msg.get());
        const auto  handle = resp->GetResponseHandle();

        if (resp->GetResponseStatus() != 0)
        {
            // Error response — still counts as answered for state-advancement purposes.
            const bool    wasSub      = popPendingSubscriptionHandle(handle);
            const uint32_t failedOno  = popPendingGetValueHandle(handle);
            popPendingSetValueHandle(handle);

            if (wasSub && !hasPendingSubscriptions())
            {
                setState(State::Subscribed);
                if (hasPendingGetValues())
                    setState(State::GetValues);
                else
                {
                    stopTimer();
                    setState(State::Connected);
                }
            }
            else if (failedOno != 0 && !hasPendingGetValues())
            {
                stopTimer();
                setState(hasPendingSubscriptions() ? State::Subscribing : State::Connected);
            }
            return false;
        }

        // Subscription ACK
        if (popPendingSubscriptionHandle(handle))
        {
            if (!hasPendingSubscriptions())
            {
                setState(State::Subscribed);
                if (hasPendingGetValues())
                    setState(State::GetValues);
                else
                {
                    stopTimer();
                    setState(State::Connected);
                }
            }
            return true;
        }

        // GetValue response
        const std::uint32_t getValOno = popPendingGetValueHandle(handle);
        if (getValOno != 0)
        {
            if (resp->GetParamCount() > 0)
            {
                const auto it = m_onoToIdx.find(getValOno);
                if (it != m_onoToIdx.end())
                {
                    const auto& tracked = m_trackedObjects[it->second];
                    if (tracked.cb)
                        tracked.cb(resp->GetParameterData());
                }
                else
                {
                    // ONo was queried but not registered via trackObject().
                    // Let subclasses handle it (e.g. SoundscapeController intercepts
                    // the Fixed_GUID response here to trigger subscribe+query).
                    onUntrackedGetValueResponse(getValOno, resp->GetParameterData());
                }
            }
            if (!hasPendingGetValues())
            {
                stopTimer();
                setState(hasPendingSubscriptions() ? State::Subscribing : State::Connected);
            }
            return true;
        }

        // SetValue ACK (or any other tracked response)
        popPendingSetValueHandle(handle);
        return true;
    }

    case Ocp1Message::KeepAlive:
        return true;

    default:
        return false;
    }
}


// ── Pending-handle bookkeeping ────────────────────────────────────────────────

void Ocp1Controller::addPendingSubscriptionHandle(std::uint32_t handle)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    m_pendingSubscriptionHandles.push_back(handle);
}

bool Ocp1Controller::popPendingSubscriptionHandle(std::uint32_t handle)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    auto it = std::find(m_pendingSubscriptionHandles.begin(),
                        m_pendingSubscriptionHandles.end(), handle);
    if (it == m_pendingSubscriptionHandles.end())
        return false;
    m_pendingSubscriptionHandles.erase(it);
    return true;
}

bool Ocp1Controller::hasPendingSubscriptions()
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    return !m_pendingSubscriptionHandles.empty();
}

void Ocp1Controller::addPendingGetValueHandle(std::uint32_t handle, std::uint32_t ono)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    m_pendingGetValueHandles.emplace(handle, ono);
}

std::uint32_t Ocp1Controller::popPendingGetValueHandle(std::uint32_t handle)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    auto it = m_pendingGetValueHandles.find(handle);
    if (it == m_pendingGetValueHandles.end())
        return 0;
    const auto ono = it->second;
    m_pendingGetValueHandles.erase(it);
    return ono;
}

bool Ocp1Controller::hasPendingGetValues()
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    return !m_pendingGetValueHandles.empty();
}

void Ocp1Controller::addPendingSetValueHandle(std::uint32_t handle, std::uint32_t ono)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    m_pendingSetValueHandles.emplace(handle, ono);
}

std::uint32_t Ocp1Controller::popPendingSetValueHandle(std::uint32_t handle)
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    auto it = m_pendingSetValueHandles.find(handle);
    if (it == m_pendingSetValueHandles.end())
        return 0;
    const auto ono = it->second;
    m_pendingSetValueHandles.erase(it);
    return ono;
}

void Ocp1Controller::clearPendingHandles()
{
    std::lock_guard<std::mutex> lk(m_pendingMutex);
    m_pendingSubscriptionHandles.clear();
    m_pendingGetValueHandles.clear();
    m_pendingSetValueHandles.clear();
}


} // namespace NanoOcp1
