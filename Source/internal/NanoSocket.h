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

#include <string>

#if defined(_WIN32) || defined(_WIN64)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <winsock2.h>
  #include <ws2tcpip.h>
  using NanoSocketHandle = SOCKET;
  static constexpr NanoSocketHandle invalidSocketHandle = INVALID_SOCKET;
#else
  using NanoSocketHandle = int;
  static constexpr NanoSocketHandle invalidSocketHandle = -1;
#endif

namespace NanoOcp1
{

/**
 * Minimal cross-platform TCP streaming socket that covers the juce::StreamingSocket
 * surface used by NanoOcp1: connect, read, write, close, createListener,
 * waitForNextConnection, waitUntilReady.
 *
 * A single NanoSocket instance is either a client socket (created via connect()
 * or accepted via waitForNextConnection()) or a server/listener socket (created
 * via createListener()).  It is not thread-safe beyond what the OS guarantees:
 * calling close() from one thread while another thread is blocked in read() is
 * the intended and safe usage.
 */
class NanoSocket
{
public:
    NanoSocket();
    ~NanoSocket();

    NanoSocket(const NanoSocket&)            = delete;
    NanoSocket& operator=(const NanoSocket&) = delete;

    // ── Client ────────────────────────────────────────────────────────────────

    /**
     * Attempt a TCP connection to hostName:portNumber within timeoutMs milliseconds.
     * Tries every address returned by getaddrinfo for the given host.
     * Returns true on success.
     */
    bool connect(const std::string& hostName, int portNumber, int timeoutMs);

    /**
     * Read exactly num bytes into data when blockUntilFull=true (used by the
     * OCP.1 framing loop).  Returns bytes actually read, 0 on graceful close,
     * -1 on error.
     */
    int read(void* data, int num, bool blockUntilFull);

    /**
     * Write dataSize bytes from data.  Returns bytes written or -1 on error.
     */
    int write(const void* data, int dataSize);

    // ── Common ────────────────────────────────────────────────────────────────

    /** Close the underlying OS socket. Safe to call from any thread. */
    void close();

    /** Returns true if the socket is open and connected. */
    bool isConnected() const;

    /** Returns the hostname / IP address of the remote peer. */
    std::string getHostName() const;

    /**
     * Block until the socket is ready for reading (readyForReading=true) or
     * writing.  timeoutMs=-1 means block indefinitely.
     * Returns: 1=ready, 0=timed out, -1=error.
     */
    int waitUntilReady(bool readyForReading, int timeoutMs) const;

    // ── Server ────────────────────────────────────────────────────────────────

    /**
     * Bind to portNumber on bindAddress (empty = all interfaces) and start
     * listening.  Returns true on success.
     */
    bool createListener(int portNumber, const std::string& bindAddress = {});

    /**
     * Block (with a short internal timeout) until a client connects.
     * Returns a heap-allocated NanoSocket for the new connection, or nullptr
     * on timeout / error.  The caller owns the returned pointer.
     */
    NanoSocket* waitForNextConnection() const;

    /** Returns the local port the listener is bound to, or -1. */
    int getBoundPort() const;

private:
    NanoSocketHandle m_fd{invalidSocketHandle};
    std::string      m_hostName;
    bool             m_connected{false};
    bool             m_listener{false};
    int              m_boundPort{-1};

    // Initialise platform networking (idempotent, Winsock on Windows).
    static void platformInit();

    // Set m_fd to non-blocking / blocking mode.
    bool setNonBlocking(bool nonBlocking);
};

} // namespace NanoOcp1
