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

#include "NanoOcp1.h"
#include "Ocp1ObjectDefinitions.h"
#include "Variant.h"
#include "internal/NanoTimer.h"

#include <atomic>
#include <functional>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <unordered_map>
#include <vector>


namespace NanoOcp1
{


/**
 * @brief Generic OCA/OCP.1 session controller.
 *
 * Manages the connection lifecycle for any OCA-compliant device:
 * TCP connection with auto-retry, subscribe/query/set command dispatch,
 * response-handle tracking, and value-change notification delivery.
 *
 * ## Usage
 * 1. Call trackObject() for each OCA parameter of interest.
 * 2. Call connect(host, port) to start.  The controller transitions through
 *    Connecting → Subscribing → Subscribed → GetValues → Connected automatically.
 *    On connection loss the underlying NanoOcp1Client retries automatically and
 *    the controller re-subscribes when it reconnects.
 * 3. Use setValue() to send SetValue commands while Connected.
 * 4. Call disconnect() to stop.
 *
 * ## Threading
 * By default (`callbacksOnMessageThread = true`, the constructor parameter),
 * all ValueCallbacks and onStateChanged are posted to a dedicated
 * `NanoAsyncDispatcher` worker thread rather than firing directly on the
 * NanoOcp1 socket thread — see `Ocp1Connection`'s constructor documentation.
 * Pass `false` to receive callbacks synchronously on the socket thread instead.
 * Either way, callers that need to marshal onto a specific thread of their own
 * (e.g. a GUI thread) must still do so inside their callback implementations.
 *
 * ## Subclassing
 * Override afterConnected() to insert a device-specific handshake before the
 * standard subscribe/query sequence (e.g. SoundscapeController queries the device
 * GUID first to determine the OCA revision before subscribing).  The override
 * must eventually call createObjectSubscriptions() and queryObjectValues() to
 * advance the state machine.
 */
class Ocp1Controller : private NanoTimer
{
public:
    enum class State
    {
        Disconnected, ///< No TCP connection; no resources allocated.
        Connecting,   ///< TCP connect attempted; client retries automatically.
        Subscribing,  ///< AddSubscription commands sent; awaiting ACKs.
        Subscribed,   ///< All subscription ACKs received; GetValue queries outstanding.
        GetValues,    ///< GetValue responses being collected.
        Connected     ///< All subscriptions and initial values confirmed.
    };

    /** Callback invoked with raw OCA parameter bytes when a tracked object changes. */
    using ValueCallback = std::function<void(const ByteVector& paramData)>;

    /**
     * @param callbacksOnMessageThread  See "Threading" above. Forwarded to the
     *                                  internal `NanoOcp1Client` on every connect().
     */
    explicit Ocp1Controller(bool callbacksOnMessageThread = true);
    virtual ~Ocp1Controller();

    //==========================================================================
    /**
     * Register an OCA object to be subscribed and queried on every connection.
     *
     * The supplied callback is invoked on the socket thread whenever the device
     * reports a new value for this object, via Notification or GetValue response.
     * May only be called while Disconnected; adding objects while connected is
     * not supported.
     *
     * @param def  Heap-allocated object definition (ownership transferred).
     * @param cb   Called with raw parameter bytes on each value update.
     */
    void trackObject(std::unique_ptr<Ocp1CommandDefinition> def, ValueCallback cb);

    /**
     * Remove all tracked objects.  May only be called while Disconnected.
     */
    void clearTrackedObjects();

    //==========================================================================
    /**
     * Send a SetValue command for the given definition.
     * Only succeeds when the controller is in the Connected state.
     * @return true if the command was sent successfully.
     */
    bool setValue(const Ocp1CommandDefinition& def, const Variant& value);

    //==========================================================================
    /**
     * Start the connection lifecycle.  No-op if the controller is not Disconnected.
     * The underlying NanoOcp1Client retries the TCP connection automatically until
     * it succeeds or disconnect() is called.
     */
    void connect(const std::string& host, int port, int timeoutMs = 150);

    /** Stop the connection and reset to Disconnected state. */
    void disconnect();

    State getState() const { return m_state.load(); }

    //==========================================================================
    /** Fired on the socket thread whenever the connection state changes. */
    std::function<void(State)> onStateChanged;

protected:
    //==========================================================================
    /**
     * Called on the socket thread immediately after the TCP connection is
     * established.
     *
     * The default implementation calls createObjectSubscriptions() followed by
     * queryObjectValues(), which is appropriate for devices that do not require
     * a pre-subscription handshake.  Override in device-specific subclasses to
     * perform additional steps (e.g. querying a device GUID) before the standard
     * sequence.  Overrides must eventually call createObjectSubscriptions() and
     * queryObjectValues() to advance the state machine to Connected.
     */
    virtual void afterConnected();

    /**
     * Called from processMessage() when a successful GetValue response arrives
     * for an ONo that is not registered in m_onoToIdx (i.e. the ONo was queried
     * via queryObjectValue() but was never registered via trackObject()).
     *
     * SoundscapeController overrides this to intercept the Fixed_GUID response that
     * arrives before any tracked objects are subscribed: after reading the GUID
     * it rebuilds the tracked-object list and triggers subscribe+query.
     *
     * The base-class state-advancement logic (checking hasPendingGetValues() after
     * this call) still runs normally, so the override must not call setState()
     * itself — instead it should enqueue new pending handles via
     * createObjectSubscriptions() / queryObjectValues() so that the state machine
     * does not prematurely advance to Connected.
     *
     * @param ono        OCA Object Number of the untracked object.
     * @param paramData  Raw OCA parameter bytes from the response.
     */
    virtual void onUntrackedGetValueResponse(std::uint32_t ono, const ByteVector& paramData);

    /**
     * Send AddSubscription commands for every tracked object.
     * Transitions to Subscribing state.  Safe to call from the socket thread.
     * @return true if all commands were sent without error.
     */
    bool createObjectSubscriptions();

    /**
     * Send GetValue commands for every tracked object.
     * Transitions to GetValues state and starts the response-timeout timer.
     * If no objects are tracked, transitions directly to Connected.
     * @return true if all commands were sent without error.
     */
    bool queryObjectValues();

    /**
     * Send a GetValue command for a single object definition.
     * The response is dispatched to the object's registered ValueCallback.
     */
    bool queryObjectValue(const Ocp1CommandDefinition& def);

    /** Direct access to the underlying client for subclasses (e.g. to send raw commands). */
    NanoOcp1Client* client() const { return m_client.get(); }

    //==========================================================================
    // Pending-handle bookkeeping.
    // All methods are mutex-protected; safe to call from any thread.
    //
    void addPendingSubscriptionHandle(std::uint32_t handle);
    bool popPendingSubscriptionHandle(std::uint32_t handle);
    bool hasPendingSubscriptions();

    void addPendingGetValueHandle(std::uint32_t handle, std::uint32_t ono);
    std::uint32_t popPendingGetValueHandle(std::uint32_t handle);
    bool hasPendingGetValues();

    void addPendingSetValueHandle(std::uint32_t handle, std::uint32_t ono);
    std::uint32_t popPendingSetValueHandle(std::uint32_t handle);

    void clearPendingHandles();

private:
    //==========================================================================
    bool processMessage(const ByteVector& data);
    void setState(State s);
    void retryPendingGetValues();

    // NanoTimer override — fired when the GetValues response-timeout elapses.
    void timerCallback() override;

    //==========================================================================
    struct TrackedObject
    {
        std::unique_ptr<Ocp1CommandDefinition> def;
        ValueCallback                           cb;
    };

    std::vector<TrackedObject>             m_trackedObjects;
    std::unordered_map<uint32_t, size_t>   m_onoToIdx;      ///< ONo → index in m_trackedObjects

    std::unique_ptr<NanoOcp1Client>        m_client;
    std::string                            m_host;
    int                                    m_port{50014};
    int                                    m_timeoutMs{150};
    bool                                   m_callbacksOnMessageThread;

    std::atomic<State>                     m_state{State::Disconnected};

    std::mutex                             m_pendingMutex;
    std::vector<std::uint32_t>             m_pendingSubscriptionHandles;
    std::map<std::uint32_t, std::uint32_t> m_pendingGetValueHandles; ///< handle → ONo
    std::map<std::uint32_t, std::uint32_t> m_pendingSetValueHandles; ///< handle → ONo
};


} // namespace NanoOcp1
