/* Copyright (c) 2022-2023, Christian Ahrens
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

#include <functional>
#include <memory>
#include <string>

#include "Ocp1Connection.h"
#include "Ocp1ConnectionServer.h"
#include "Ocp1DataTypes.h"
#include "internal/NanoTimer.h"


/**
 * @namespace NanoOcp1
 * @brief Minimal AES70 / OCP.1 TCP client/server library.
 *
 * ## Overview
 * NanoOcp is a lightweight implementation of the OCA (Open Control Architecture) wire
 * protocol **OCP.1** (TCP framing) from the **AES70** standard.  It is intentionally
 * "nano" — no object database, no root-block management, no full AES70 compliance — just
 * enough to send and receive OCA commands, responses, notifications, and keep-alives over
 * a plain TCP socket.
 *
 * ## Key concepts
 * | Concept | Description |
 * |---|---|
 * | **ONo** (Object Number) | 32-bit identifier that uniquely addresses one controllable parameter on an OCA device. Generated via `GetONo()` / `GetONoTy2()` from type, record, channel, and box/object numbers. |
 * | **Command** | A client-to-device message. `Ocp1CommandResponseRequired` carries a handle so the response can be correlated. |
 * | **Response** | Device reply to a command. The handle in the response matches the originating command handle. |
 * | **Notification** | Unsolicited device-to-client message fired when a subscribed property changes. Matched to a subscription via ONo + def-level + property-index. |
 * | **KeepAlive** | Heartbeat exchanged in both directions to detect dropped connections. |
 * | **AddSubscription** | Command that registers interest in a property: the device will send Notifications whenever that property changes. |
 *
 * ## Threading model
 * `NanoOcp1Client` runs its socket I/O on a dedicated `Ocp1Connection::ConnectionThread`.
 * All three callbacks (`onDataReceived`, `onConnectionEstablished`, `onConnectionLost`)
 * fire on the socket thread when `callbacksOnMessageThread = false`. When it is `true`
 * (the default), they are instead posted to a dedicated `NanoAsyncDispatcher` worker
 * thread — see `Ocp1Connection`'s constructor documentation.
 *
 * ## File map
 * | Header | Contents |
 * |---|---|
 * | `NanoOcp1.h` | `NanoOcp1Client`, `NanoOcp1Server`, `NanoOcp1Base` |
 * | `Ocp1Connection.h` | Raw TCP socket management (abstract) |
 * | `Ocp1ConnectionServer.h` | Accept-loop server |
 * | `Ocp1Message.h` | Message structs and factory; `Ocp1CommandDefinition` |
 * | `Ocp1DataTypes.h` | `ByteVector`, `Ocp1DataType`, marshal/unmarshal helpers |
 * | `Variant.h` | Type-erased OCA value with marshal/unmarshal |
 * | `Ocp1ObjectDefinitions.h` | Generic d&b amp object definitions (AmpGeneric/DxDy/5D) |
 * | `Ocp1DS100ObjectDefinitions.h` | DS100-specific object definitions (namespace `DS100`) |
 */
namespace NanoOcp1
{

/**
 * @class NanoOcp1Base
 * @brief Abstract base class shared by `NanoOcp1Client` and `NanoOcp1Server`.
 *
 * Holds the target address/port, exposes the three user-facing callbacks, and
 * provides `processReceivedData()` which invokes `onDataReceived`.
 */
class NanoOcp1Base
{
public:
    //==============================================================================
    NanoOcp1Base(const std::string& address, const int port);
    virtual ~NanoOcp1Base();

    /** @brief Sets the IP address or hostname of the remote OCA device. */
    void setAddress(const std::string& address);
    /** @brief Returns the current target address. */
    const std::string& getAddress() const;

    /** @brief Sets the TCP port number of the remote OCA device. DS100 default: 50014. */
    void setPort(const int port);
    /** @brief Returns the current target port number. */
    int getPort() const;

    //==============================================================================
    /** @brief Starts the client/server.  For the client, begins periodic reconnect attempts. */
    virtual bool start() = 0;
    /** @brief Stops the client/server and closes any open TCP connection. */
    virtual bool stop() = 0;

    //==============================================================================
    /**
     * @brief Sends raw bytes over the active TCP connection.
     * @param data  Serialized OCP.1 message bytes (from `Ocp1Message::GetSerializedData()`).
     * @return      True if the bytes were handed to the socket layer successfully.
     */
    virtual bool sendData(const ByteVector& data) = 0;

    //==============================================================================
    /**
     * @brief Fired when a complete OCP.1 frame is received.
     *
     * The caller should unmarshal the bytes with `Ocp1Message::UnmarshalOcp1Message()`,
     * then dispatch on `GetMessageType()`.
     *
     * @return Return true to indicate the data was handled; returning false has no
     *         special effect in the current implementation.
     */
    std::function<bool(const ByteVector&)> onDataReceived;

    /**
     * @brief Fired once after a successful TCP connection is established.
     */
    std::function<void()> onConnectionEstablished;

    /**
     * @brief Fired when the TCP connection is dropped or a connect attempt fails.
     */
    std::function<void()> onConnectionLost;

protected:
    //==============================================================================
    /**
     * @brief Called by derived classes when bytes arrive from the socket.
     * Invokes `onDataReceived` if set.
     */
    bool processReceivedData(const ByteVector& data);

private:
    //==============================================================================
    std::string m_address;   ///< Target IP address or hostname.
    int         m_port{ 0 }; ///< Target TCP port number.
};

/**
 * @class NanoOcp1Client
 * @brief OCP.1 TCP client with automatic reconnection.
 *
 * Inherits socket I/O from `Ocp1Connection` and reconnect timing from `NanoTimer`.
 * When `start()` is called, a timer fires periodically and attempts
 * `connectToSocket()` until it succeeds.  Once connected, `connectionMade()` calls
 * `onConnectionEstablished`.  On disconnect (detected by the read thread),
 * `connectionLost()` calls `onConnectionLost` and the timer resumes retrying.
 */
class NanoOcp1Client : public NanoOcp1Base, public Ocp1Connection, public NanoTimer
{
public:
    //==============================================================================
    /**
     * @brief Constructs a client without an initial address/port.
     *        Call `setAddress()` and `setPort()` before `start()`.
     * @param callbacksOnMessageThread  See `Ocp1Connection`'s constructor: if true
     *                                  (the default), callbacks are posted to a
     *                                  dedicated worker thread instead of firing
     *                                  directly on the socket thread.
     * @param threadPriority            OS thread priority for the socket I/O thread.
     */
    NanoOcp1Client(bool callbacksOnMessageThread,
                   ThreadPriority threadPriority = ThreadPriority::normal);

    /**
     * @brief Constructs a client with address and port pre-configured.
     * @param address               IP address or hostname of the OCA device.
     * @param port                  TCP port number (DS100 default: 50014).
     * @param callbacksOnMessageThread  See other constructor.
     * @param threadPriority            See other constructor.
     */
    NanoOcp1Client(const std::string& address, int port,
                   bool callbacksOnMessageThread,
                   ThreadPriority threadPriority = ThreadPriority::normal);
    ~NanoOcp1Client() override;

    //==============================================================================
    /**
     * @brief Starts the reconnect timer and begins attempting to connect.
     * @return True always; actual connection success is signalled via `onConnectionEstablished`.
     */
    bool start() override;

    /**
     * @brief Stops the reconnect timer and closes the TCP socket.
     * @return True always.
     */
    bool stop() override;

    /** @brief Returns true if `start()` has been called and `stop()` has not. */
    bool isRunning();

    //==============================================================================
    /**
     * @brief Sends serialized OCP.1 bytes over the active TCP connection.
     */
    bool sendData(const ByteVector& data) override;

    //==============================================================================
    /** @brief Called by `Ocp1Connection` when TCP connect succeeds — invokes `onConnectionEstablished`. */
    void connectionMade() override;
    /** @brief Called by `Ocp1Connection` when TCP connection is lost — invokes `onConnectionLost`. */
    void connectionLost() override;
    /** @brief Called by `Ocp1Connection` for each received OCP.1 frame — invokes `onDataReceived`. */
    void messageReceived(const ByteVector& message) override;

protected:
    //==============================================================================
    /** @brief Timer callback — attempts `connectToSocket()` when not yet connected. */
    void timerCallback() override;

private:
    //==============================================================================
    bool m_running{ false }; ///< Set true by start(), false by stop().
};

/**
 * @class NanoOcp1Server
 * @brief OCP.1 TCP server that accepts a single incoming connection at a time.
 *
 * Internally wraps `Ocp1ConnectionServer` (accept loop) and creates a
 * `NanoOcp1Client` peer object when a connection arrives.  The same three
 * callbacks (`onDataReceived`, `onConnectionEstablished`, `onConnectionLost`) are
 * available and behave identically to `NanoOcp1Client`.
 *
 * Only one simultaneous connection is supported.
 */
class NanoOcp1Server : public NanoOcp1Base, public Ocp1ConnectionServer
{
public:
    //==============================================================================
    /**
     * @brief Constructs a server without an initial bind address/port.
     * @param callbacksOnMessageThread  Propagated to the accepted peer `NanoOcp1Client`;
     *                                  see `Ocp1Connection`'s constructor.
     * @param threadPriority            OS thread priority for the accept thread.
     */
    NanoOcp1Server(bool callbacksOnMessageThread,
                   ThreadPriority threadPriority = ThreadPriority::normal);

    /**
     * @brief Constructs a server with bind address and port pre-configured.
     * @param address               Local address to bind to (empty = all interfaces).
     * @param port                  TCP port to listen on.
     * @param callbacksOnMessageThread  See other constructor.
     * @param threadPriority            See other constructor.
     */
    NanoOcp1Server(const std::string& address, int port,
                   bool callbacksOnMessageThread,
                   ThreadPriority threadPriority = ThreadPriority::normal);
    ~NanoOcp1Server() override;

    //==============================================================================
    /**
     * @brief Binds the socket and starts the accept loop thread.
     * @return True if the socket was bound and the accept thread started.
     */
    bool start() override;

    /**
     * @brief Stops the accept loop and closes any active connection.
     * @return True always.
     */
    bool stop() override;

    //==============================================================================
    /**
     * @brief Sends serialized OCP.1 bytes to the currently connected peer.
     * If no peer is connected, returns false.
     */
    bool sendData(const ByteVector& data) override;

protected:
    //==============================================================================
    /**
     * @brief Factory method called by the accept loop — creates the `NanoOcp1Client`
     *        peer object for the newly accepted TCP connection.
     */
    Ocp1Connection* createConnectionObject() override;

private:
    //==============================================================================
    std::unique_ptr<NanoOcp1Client> m_activeConnection;     ///< The currently connected peer.
    bool           m_callbacksOnMessageThread{ true };      ///< Propagated to the peer client.
    ThreadPriority m_threadPriority;                        ///< Propagated to the peer client.
};

} // namespace NanoOcp1
