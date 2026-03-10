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

#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    #include <juce_core/juce_core.h>
#else
    #include <JuceHeader.h>
#endif

#include "Ocp1DataTypes.h"


namespace NanoOcp1
{


class Ocp1ConnectionServer;


/**
 * @class Ocp1Connection
 * @brief Low-level TCP socket manager for a single OCP.1 connection.
 *
 * Derived from and inspired by `juce::InterprocessConnection`, but stripped of
 * the JUCE IPC handshake header and all named-pipe support so it works as a
 * plain TCP byte-stream suitable for OCP.1.
 *
 * ## Role in the library
 * `Ocp1Connection` is **abstract** — it manages the socket, the dedicated read
 * thread, and message framing, but delegates the three events to pure-virtual
 * overrides:
 * - `connectionMade()` — TCP handshake succeeded.
 * - `connectionLost()` — TCP dropped or disconnected.
 * - `messageReceived()` — a complete OCP.1 frame arrived.
 *
 * `NanoOcp1Client` provides the concrete implementation: it bridges these calls
 * into the `NanoOcp1Base` callback functions (`onConnectionEstablished` etc.) and
 * optionally marshals them to the JUCE message thread.
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
 * which acquires `socketLock` (a `ReadWriteLock`).  Callbacks are either delivered
 * on the read thread (`callbacksOnMessageThread=false`) or posted asynchronously
 * to the JUCE message thread.
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
     * @param callbacksOnMessageThread  If true, `connectionMade()`, `connectionLost()`,
     *                                  and `messageReceived()` are posted to the JUCE
     *                                  message thread.  If false, they run directly on
     *                                  the socket read thread (lower latency).
     * @param threadPriority            OS priority of the socket read thread.
     */
    Ocp1Connection(bool callbacksOnMessageThread = true, const juce::Thread::Priority threadPriority = juce::Thread::Priority::normal);
    virtual ~Ocp1Connection();

    /**
     * @brief Attempts a TCP connection to the given host and port.
     * Spawns the read thread on success.
     * @param hostName          IP address or hostname of the remote device.
     * @param portNumber        TCP port number (DS100 default: 50014).
     * @param timeOutMillisecs  Maximum time to wait for the TCP handshake.
     * @return True if the connection was established.
     */
    bool connectToSocket(const juce::String& hostName, int portNumber, int timeOutMillisecs);

    /**
     * @brief Closes the TCP socket and stops the read thread.
     * @param timeoutMs  Maximum ms to wait for the read thread to exit.
     * @param notify     Whether to invoke `connectionLost()` after closing.
     *                   Pass `Notify::no` for a clean intentional shutdown.
     */
    void disconnect(int timeoutMs = 0, Notify notify = Notify::yes);

    /** @brief Returns true if the TCP socket is currently open. */
    bool isConnected() const;

    /** @brief Returns the underlying JUCE socket (for diagnostics). */
    juce::StreamingSocket* getSocket() const noexcept { return socket.get(); }

    /** @brief Returns the hostname of the currently connected remote peer, or an empty string. */
    juce::String getConnectedHostName() const;

    /**
     * @brief Sends a complete OCP.1 frame over the TCP socket.
     * Acquires the write lock, then writes all bytes in one blocking call.
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
    juce::ReadWriteLock socketLock;
    std::unique_ptr<juce::StreamingSocket> socket;
    bool callbackConnectionState = false;
    const bool useMessageThread;

    friend class Ocp1ConnectionServer;
    void initialise();
    void initialiseWithSocket(std::unique_ptr<juce::StreamingSocket>);
    void deleteSocket();
    void connectionMadeInt();
    void connectionLostInt();
    void deliverDataInt(const ByteVector&);
    bool readNextMessage();
    int readData(void*, int);

    struct ConnectionThread;
    std::unique_ptr<ConnectionThread> thread;
    std::atomic<bool> threadIsRunning{ false };

    class SafeAction;
    std::shared_ptr<SafeAction> safeAction;

    void runThread();
    int writeData(void*, int);
    
    juce::Thread::Priority m_threadPriority;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ocp1Connection)
};

}