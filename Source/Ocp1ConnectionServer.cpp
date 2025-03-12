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

#include "Ocp1ConnectionServer.h"
#include "Ocp1Connection.h"


namespace NanoOcp1
{


Ocp1ConnectionServer::Ocp1ConnectionServer(const juce::Thread::Priority threadPriority) : juce::Thread("NanoOcp1 connection server"), m_threadPriority(threadPriority)
{
}

Ocp1ConnectionServer::~Ocp1ConnectionServer()
{
    stop();
}

//==============================================================================
bool Ocp1ConnectionServer::beginWaitingForSocket(const int portNumber, const juce::String& bindAddress)
{
    stop();

    socket.reset(new juce::StreamingSocket());

    if (socket->createListener(portNumber, bindAddress))
    {
        startThread(m_threadPriority);
        return true;
    }

    socket.reset();
    return false;
}

void Ocp1ConnectionServer::stop()
{
    signalThreadShouldExit();

    if (socket != nullptr)
        socket->close();

    stopThread(4000);
    socket.reset();
}

int Ocp1ConnectionServer::getBoundPort() const noexcept
{
    return (socket == nullptr) ? -1 : socket->getBoundPort();
}

void Ocp1ConnectionServer::run()
{
    while ((!threadShouldExit()) && socket != nullptr)
    {
        std::unique_ptr<juce::StreamingSocket> clientSocket(socket->waitForNextConnection());

        if (clientSocket != nullptr)
            if (auto* newConnection = createConnectionObject())
                newConnection->initialiseWithSocket(std::move(clientSocket));
    }
}

}