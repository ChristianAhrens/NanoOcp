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

#include <memory>
#include <string>

#include "internal/NanoSocket.h"
#include "internal/NanoThread.h"


namespace NanoOcp1
{

class Ocp1Connection;


/**
 * @class Ocp1ConnectionServer
 * @brief TCP accept-loop server base class for OCP.1 connections.
 *
 * Runs a background thread that blocks on `accept()` and calls the
 * pure-virtual `createConnectionObject()` each time a new TCP client connects.
 * The concrete subclass (`NanoOcp1Server`) implements `createConnectionObject()`
 * to return a `NanoOcp1Client` peer wired to the server's callbacks.
 *
 * @see NanoOcp1::Ocp1Connection
 * @see NanoOcp1::NanoOcp1Server
 */
class Ocp1ConnectionServer : private NanoThread
{
public:
    //==============================================================================
    /**
     * @brief Constructs the server.
     * @param threadPriority  OS priority of the accept-loop thread.
     */
    explicit Ocp1ConnectionServer(ThreadPriority threadPriority = ThreadPriority::normal);
    ~Ocp1ConnectionServer() override;

    Ocp1ConnectionServer(const Ocp1ConnectionServer&)            = delete;
    Ocp1ConnectionServer& operator=(const Ocp1ConnectionServer&) = delete;

    /**
     * @brief Binds a TCP socket to the given port and starts the accept-loop thread.
     * @param portNumber    Local TCP port to listen on.
     * @param bindAddress   Local IP address to bind to (empty = all interfaces).
     * @return True if the socket was bound and the thread started.
     */
    bool beginWaitingForSocket(int portNumber, const std::string& bindAddress = {});

    /** @brief Stops the accept-loop thread and closes the listening socket. */
    void stop();

    /** @brief Returns the port number the server is bound to, or -1 if not listening. */
    int getBoundPort() const noexcept;

protected:
    //==============================================================================
    /**
     * @brief Called by the accept loop each time a new TCP client connects.
     *
     * Implementations should create and return a new `Ocp1Connection`-derived object
     * (typically `NanoOcp1Client`) configured for the accepted socket.
     *
     * @return A heap-allocated `Ocp1Connection` object for the new client, or nullptr
     *         to reject the connection.
     */
    virtual Ocp1Connection* createConnectionObject() = 0;

private:
    //==============================================================================
    std::unique_ptr<NanoSocket> socket;

    void run() override;

    ThreadPriority m_threadPriority;
};

} // namespace NanoOcp1
