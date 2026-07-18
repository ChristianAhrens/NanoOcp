/* Copyright (c) 2023, Christian Ahrens
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
#include <memory>
#include <shared_mutex>
#include <string>

#include "Ocp1DataTypes.h"
#include "internal/NanoSocket.h"
#include "internal/NanoThread.h"


namespace NanoOcp1
{


class Ocp1ConnectionServer;


/**
 * @class Ocp1Connection
 * @brief Low-level TCP socket manager for a single OCP.1 connection.
 *
 * Manages the socket, the dedicated read thread, and message framing.
 * Delegates three events to pure-virtual overrides:
 * - `connectionMade()` — TCP handshake succeeded.
 * - `connectionLost()` — TCP dropped or disconnected.
 * - `messageReceived()` — a complete OCP.1 frame arrived.
 *
 * `NanoOcp1Client` provides the concrete implementation.
 *
 * ## Message framing
 * `readNextMessage()` reads from the socket, checks the OCP.1 sync byte (0x3b),
 * reads the 10-byte header to determine total message size, then reads the
 * remaining bytes.  The complete frame is passed as a `ByteVector` to
 * `messageReceived()`.  `Ocp1Message::UnmarshalOcp1Message()` then parses it into
 * a typed message object.
 *
 * ## Thread safety
 * The read thread owns the socket exclusively.  Writes go through `sendMessage()`
 * which acquires `socketLock` (a `shared_mutex`).  Callbacks are always delivered
 * on the read thread.
 */
class Ocp1Connection
{
public:
    /**
     * @brief Controls whether `connectionLost()` is called when `disconnect()` is invoked.
     * Use `Notify::no` when shutting down deliberately so the application does not
     * treat an intentional disconnect as an error.
     */
    enum class Notify { no, yes };

public:
    /**
     * @brief Constructs the connection object.
     * @param callbacksOnMessageThread  Kept for API compatibility; has no effect —
     *                                  callbacks always run on the socket read thread.
     * @param threadPriority            OS priority of the socket read thread.
     */
    Ocp1Connection(bool callbacksOnMessageThread = true,
                   ThreadPriority threadPriority = ThreadPriority::normal);
    virtual ~Ocp1Connection();

    Ocp1Connection(const Ocp1Connection&)            = delete;
    Ocp1Connection& operator=(const Ocp1Connection&) = delete;

    /**
     * @brief Attempts a TCP connection to the given host and port.
     * Spawns the read thread on success.
     * @param hostName          IP address or hostname of the remote device.
     * @param portNumber        TCP port number.
     * @param timeOutMillisecs  Maximum time to wait for the TCP handshake.
     * @return True if the connection was established.
     */
    bool connectToSocket(const std::string& hostName, int portNumber, int timeOutMillisecs);

    /**
     * @brief Closes the TCP socket and stops the read thread.
     * @param timeoutMs  Maximum ms to wait for the read thread to exit.
     * @param notify     Whether to invoke `connectionLost()` after closing.
     *                   Pass `Notify::no` for a clean intentional shutdown.
     */
    void disconnect(int timeoutMs = 0, Notify notify = Notify::yes);

    /** @brief Returns true if the TCP socket is currently open. */
    bool isConnected() const;

    /** @brief Returns the underlying socket (for diagnostics). */
    NanoSocket* getSocket() const noexcept { return socket.get(); }

    /** @brief Returns the hostname of the currently connected remote peer, or an empty string. */
    std::string getConnectedHostName() const;

    /**
     * @brief Sends a complete OCP.1 frame over the TCP socket.
     * @param message  Complete serialized OCP.1 message bytes.
     * @return True if all bytes were written successfully.
     */
    bool sendMessage(const ByteVector& message);

    //==============================================================================
    /** @brief Called when the TCP connection is successfully established. Override to react. */
    virtual void connectionMade() = 0;
    /** @brief Called when the TCP connection is dropped or closed. Override to react. */
    virtual void connectionLost() = 0;
    /**
     * @brief Called with each complete OCP.1 frame received from the remote device.
     * Pass `message` to `Ocp1Message::UnmarshalOcp1Message()` to get a typed message object.
     */
    virtual void messageReceived(const ByteVector& message) = 0;

private:
    //==============================================================================
    mutable std::shared_mutex        socketLock;
    std::unique_ptr<NanoSocket>      socket;
    bool                             callbackConnectionState = false;
    const bool                       useMessageThread;

    friend class Ocp1ConnectionServer;
    void initialise();
    void initialiseWithSocket(std::unique_ptr<NanoSocket>);
    void deleteSocket();
    void connectionMadeInt();
    void connectionLostInt();
    void deliverDataInt(const ByteVector&);
    bool readNextMessage();
    int  readData(void*, int);

    struct ConnectionThread;
    std::unique_ptr<ConnectionThread> thread;
    std::atomic<bool>                 threadIsRunning{ false };

    class SafeAction;
    std::shared_ptr<SafeAction>       safeAction;

    void runThread();
    int  writeData(void*, int);

    ThreadPriority m_threadPriority;
};

} // namespace NanoOcp1
