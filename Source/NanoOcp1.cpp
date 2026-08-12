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
NanoOcp1Base::NanoOcp1Base(const std::string& address, const int port)
{
    setAddress(address);
    setPort(port);
}

NanoOcp1Base::~NanoOcp1Base()
{
}

void NanoOcp1Base::setAddress(const std::string& address)
{
    m_address = address;
}

const std::string& NanoOcp1Base::getAddress() const
{
    return m_address;
}

void NanoOcp1Base::setPort(const int port)
{
    m_port = port;
}

int NanoOcp1Base::getPort() const
{
    return m_port;
}

bool NanoOcp1Base::processReceivedData(const ByteVector& data)
{
    if (onDataReceived)
        return onDataReceived(data);

    return false;
}

//==============================================================================
NanoOcp1Client::NanoOcp1Client(bool callbacksOnMessageThread,
                               ThreadPriority threadPriority)
    : NanoOcp1Client(std::string{}, 0, callbacksOnMessageThread, threadPriority)
{
}

NanoOcp1Client::NanoOcp1Client(const std::string& address, int port,
                               bool callbacksOnMessageThread,
                               ThreadPriority threadPriority)
    : NanoOcp1Base(address, port),
      Ocp1Connection(callbacksOnMessageThread, threadPriority)
{
}

NanoOcp1Client::~NanoOcp1Client()
{
    m_running = false;
    stopTimer();

    // See comment in Ocp1Connection destructor.
    disconnect(4000, Notify::no);
}

bool NanoOcp1Client::start()
{
    m_running = true;

    if (connectToSocket(getAddress(), getPort(), 50))
        return true; // connection immediately established
    else
        startTimer(500); // start trying to establish connection

    return false;
}

bool NanoOcp1Client::stop()
{
    m_running = false;

    stopTimer();

    disconnect(1000);

    if (onConnectionLost && !isConnected())
        onConnectionLost();

    return !isConnected();
}

bool NanoOcp1Client::isRunning()
{
    return m_running;
}

bool NanoOcp1Client::sendData(const ByteVector& data)
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

    if (m_running)
        startTimer(500); // start trying to reestablish connection
}

void NanoOcp1Client::messageReceived(const ByteVector& message)
{
    processReceivedData(message);
}

void NanoOcp1Client::timerCallback()
{
    // Do NOT call stopTimer() here on success. connectToSocket() already routes
    // through initialise() -> connectionMadeInt() -> connectionMade(), and
    // connectionMade() itself calls stopTimer(). When callbacksOnMessageThread
    // is true (the default), that call happens asynchronously on the dispatcher
    // thread — concurrently with this very callback still running on the timer
    // thread. Two concurrent stopTimer() calls on the same NanoTimer deadlock:
    // the foreign-thread call blocks in std::thread::join() waiting for this
    // timer thread's callback to return, while this thread's own reentrant
    // stopTimer() call (guarded to skip join() on self) still has to wait for
    // the same m_lifecycleMutex the joining thread is holding — which it can
    // never release until this thread returns. So leave stopping the timer
    // solely to connectionMade(), and just avoid redialling an already-live
    // connection while that callback is in flight.
    if (!isConnected())
        connectToSocket(getAddress(), getPort(), 50);
}

//==============================================================================
NanoOcp1Server::NanoOcp1Server(bool callbacksOnMessageThread,
                               ThreadPriority threadPriority)
    : NanoOcp1Server(std::string{}, 0, callbacksOnMessageThread, threadPriority)
{
}

NanoOcp1Server::NanoOcp1Server(const std::string& address, int port,
                               bool callbacksOnMessageThread,
                               ThreadPriority threadPriority)
    : NanoOcp1Base(address, port),
      Ocp1ConnectionServer(threadPriority),
      m_callbacksOnMessageThread(callbacksOnMessageThread),
      m_threadPriority(threadPriority)
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
        m_activeConnection->disconnect(1000);
        return !m_activeConnection->isConnected();
    }
    else
        return true;
}

bool NanoOcp1Server::sendData(const ByteVector& data)
{
    if (!m_activeConnection)
        return false;

    return m_activeConnection->sendData(data);
}

Ocp1Connection* NanoOcp1Server::createConnectionObject()
{
    m_activeConnection = std::make_unique<NanoOcp1Client>(
        m_callbacksOnMessageThread, m_threadPriority);
    m_activeConnection->onDataReceived = this->onDataReceived;

    return m_activeConnection.get();
}


} // namespace NanoOcp1
