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


namespace NanoOcp1
{

class Ocp1Connection;


/**
 * @class Ocp1ConnectionServer
 * @brief TCP accept-loop server base class for OCP.1 connections.
 *
 * Runs a background `juce::Thread` that blocks on `accept()` and calls the
 * pure-virtual `createConnectionObject()` each time a new TCP client connects.
 * The concrete subclass (`NanoOcp1Server`) implements `createConnectionObject()`
 * to return a `NanoOcp1Client` peer wired to the server's callbacks.
 *
 * Derived from and inspired by `juce::InterprocessConnectionServer`, stripped of
 * named-pipe and JUCE IPC handshake support.
 *
 * @see NanoOcp1::Ocp1Connection
 * @see NanoOcp1::NanoOcp1Server
 */
class Ocp1ConnectionServer : private juce::Thread
{
public:
    //==============================================================================
    /**
     * @brief Constructs the server.
     * @param threadPriority  OS priority of the accept-loop thread.
     */
    Ocp1ConnectionServer(const juce::Thread::Priority threadPriority = juce::Thread::Priority::normal);
    ~Ocp1ConnectionServer() override;

    /**
     * @brief Binds a TCP socket to the given port and starts the accept-loop thread.
     * @param portNumber    Local TCP port to listen on.
     * @param bindAddress   Local IP address to bind to (empty = all interfaces).
     * @return True if the socket was bound and the thread started.
     */
    bool beginWaitingForSocket(int portNumber, const juce::String& bindAddress = juce::String());

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
     * (typically `NanoOcp1Client`) configured for the accepted socket.  The server
     * takes ownership.
     *
     * @return A heap-allocated `Ocp1Connection` object for the new client, or nullptr
     *         to reject the connection.
     */
    virtual Ocp1Connection* createConnectionObject() = 0;

private:
    //==============================================================================
    std::unique_ptr<juce::StreamingSocket> socket;

    void run() override;
    
    juce::Thread::Priority m_threadPriority;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ocp1ConnectionServer)
};

}