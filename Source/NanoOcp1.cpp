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

#include "NanoOcp1.h"


namespace NanoOcp1
{


//==============================================================================
NanoOcp1Base::NanoOcp1Base(const juce::String& address, const int port)
{
    setAddress(address);
    setPort(port);
}

NanoOcp1Base::~NanoOcp1Base()
{
}

void NanoOcp1Base::setAddress(const juce::String& address)
{
    m_address = address;
}

const juce::String& NanoOcp1Base::getAddress()
{
    return m_address;
}

void NanoOcp1Base::setPort(const int port)
{
    m_port = port;
}

const int NanoOcp1Base::getPort()
{
    return m_port;
}

bool NanoOcp1Base::processReceivedData(const juce::MemoryBlock& data)
{
    if (onDataReceived)
        return onDataReceived(data);

    return false;
}

//==============================================================================
NanoOcp1Client::NanoOcp1Client(const bool callbacksOnMessageThread) :
    NanoOcp1Client(juce::String(), 0, callbacksOnMessageThread)
{
}

NanoOcp1Client::NanoOcp1Client(const juce::String& address, const int port, const bool callbacksOnMessageThread) :
    NanoOcp1Base(address, port), Ocp1Connection(callbacksOnMessageThread)
{
}

NanoOcp1Client::~NanoOcp1Client()
{
    stop();
}

bool NanoOcp1Client::start()
{
    m_started = true;

    if (connectToSocket(getAddress(), getPort(), 50))
        return true; // connection immediatly established
    else
        startTimer(500); // start trying to establish connection

    return false;
}

bool NanoOcp1Client::stop()
{
    m_started = false;

    stopTimer();

    disconnect();

    if (onConnectionLost && !isConnected())
        onConnectionLost();

    return !isConnected();
}

bool NanoOcp1Client::sendData(const juce::MemoryBlock& data)
{
    if (!isConnected())
        return false;

    return Ocp1Connection::sendMessage(data);
}

void NanoOcp1Client::connectionMade()
{
    stopTimer();

    if (onConnectionEstablished)
        onConnectionEstablished();
}

void NanoOcp1Client::connectionLost()
{
    if (onConnectionLost)
        onConnectionLost();

    if (m_started)
        startTimer(500); // start trying to reestablish connection
}

void NanoOcp1Client::messageReceived(const juce::MemoryBlock& message)
{
    processReceivedData(message);
}

void NanoOcp1Client::timerCallback()
{
    if (connectToSocket(getAddress(), getPort(), 50))
        stopTimer(); // connection established, no need to retry
}

//==============================================================================
NanoOcp1Server::NanoOcp1Server(const bool callbacksOnMessageThread) :
    NanoOcp1Server(juce::String(), 0, callbacksOnMessageThread)
{
}

NanoOcp1Server::NanoOcp1Server(const juce::String& address, const int port, const bool callbacksOnMessageThread) :
    NanoOcp1Base(address, port), Ocp1ConnectionServer(), m_callbacksOnMessageThread(callbacksOnMessageThread)
{
}

NanoOcp1Server::~NanoOcp1Server()
{
    stop();
}

bool NanoOcp1Server::start()
{
    return beginWaitingForSocket(getPort(), getAddress());
}

bool NanoOcp1Server::stop()
{
    if (m_activeConnection)
    {
        m_activeConnection->disconnect();
        return !m_activeConnection->isConnected();
    }
    else
        return true;
}

bool NanoOcp1Server::sendData(const juce::MemoryBlock& data)
{
    if (!m_activeConnection)
        return false;

    return m_activeConnection->sendData(data);
}

Ocp1Connection* NanoOcp1Server::createConnectionObject()
{
    m_activeConnection = std::make_unique<NanoOcp1Client>(m_callbacksOnMessageThread);
    m_activeConnection->onDataReceived = this->onDataReceived;

    return m_activeConnection.get();
}


}
