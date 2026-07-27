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

#include "Ocp1Connection.h"
#include "Ocp1Message.h"

#include <algorithm>
#include <cassert>
#include <mutex>
#include <shared_mutex>


namespace NanoOcp1
{


// ── ConnectionThread ──────────────────────────────────────────────────────────

struct Ocp1Connection::ConnectionThread : public NanoThread
{
    explicit ConnectionThread(Ocp1Connection& c)
        : NanoThread("Ocp1Connection::ConnectionThread"), owner(c) {}

    ConnectionThread(const ConnectionThread&)            = delete;
    ConnectionThread& operator=(const ConnectionThread&) = delete;

    void run() override { owner.runThread(); }

    Ocp1Connection& owner;
};


// ── SafeAction guard ──────────────────────────────────────────────────────────
// Guards against invoking pure-virtual callbacks after the derived object has
// been destroyed.

class SafeActionImpl
{
public:
    explicit SafeActionImpl(Ocp1Connection& p) : ref(p) {}

    template <typename Fn>
    void ifSafe(Fn&& fn)
    {
        std::lock_guard<std::mutex> lock(mutex);
        if (safe)
            fn(ref);
    }

    void setSafe(bool s)
    {
        std::lock_guard<std::mutex> lock(mutex);
        safe = s;
    }

    bool isSafe()
    {
        std::lock_guard<std::mutex> lock(mutex);
        return safe;
    }

private:
    std::mutex      mutex;
    Ocp1Connection& ref;
    bool            safe = false;
};

class Ocp1Connection::SafeAction : public SafeActionImpl
{
    using SafeActionImpl::SafeActionImpl;
};


// ── Construction / destruction ────────────────────────────────────────────────

Ocp1Connection::Ocp1Connection(bool callbacksOnMessageThread,
                               ThreadPriority threadPriority)
    : useMessageThread(callbacksOnMessageThread),
      safeAction(std::make_shared<SafeAction>(*this)),
      m_threadPriority(threadPriority)
{
    thread.reset(new ConnectionThread(*this));

    if (useMessageThread)
        dispatcher = std::make_unique<NanoAsyncDispatcher>();
}

Ocp1Connection::~Ocp1Connection()
{
    // You *must* call `disconnect` in the destructor of your derived class to ensure
    // that any pending messages are not delivered. If the messages were delivered after
    // destroying the derived class, we'd end up calling the pure virtual implementations
    // of `messageReceived`, `connectionMade` and `connectionLost` which is definitely
    // not a good idea!
    assert(!safeAction->isSafe());

    callbackConnectionState = false;
    disconnect(4000, Notify::no);
    thread.reset();
}


// ── Connect / disconnect ──────────────────────────────────────────────────────

bool Ocp1Connection::connectToSocket(const std::string& hostName,
                                     int portNumber,
                                     int timeOutMillisecs)
{
    disconnect(1000);

    auto s = std::make_unique<NanoSocket>();
    if (s->connect(hostName, portNumber, timeOutMillisecs))
    {
        initialiseWithSocket(std::move(s));
        return true;
    }

    return false;
}

void Ocp1Connection::disconnect(int timeoutMs, Notify notify)
{
    // Signal exit and close the socket BEFORE joining the thread.
    // The thread may be blocked in a blocking recv() inside readData(); closing the
    // socket causes recv() to return -1 so the thread can observe threadShouldExit()
    // and exit.  NanoThread::stopThread() joins unconditionally, so if the socket
    // were closed *after* the join the two would deadlock: join waits for recv() to
    // unblock, recv() waits for the socket to close.
    thread->signalThreadShouldExit();

    {
        std::shared_lock<std::shared_mutex> sl(socketLock);
        if (socket != nullptr) socket->close();
    }

    thread->stopThread(timeoutMs);

    deleteSocket();

    if (notify == Notify::yes)
        connectionLostInt();

    callbackConnectionState = false;
    safeAction->setSafe(false);
}

void Ocp1Connection::deleteSocket()
{
    std::unique_lock<std::shared_mutex> sl(socketLock);
    socket.reset();
}

bool Ocp1Connection::isConnected() const
{
    std::shared_lock<std::shared_mutex> sl(socketLock);
    return (socket != nullptr && socket->isConnected()) && threadIsRunning;
}

std::string Ocp1Connection::getConnectedHostName() const
{
    std::shared_lock<std::shared_mutex> sl(socketLock);
    if (socket != nullptr)
        return socket->getHostName();
    return {};
}


// ── Send ──────────────────────────────────────────────────────────────────────

bool Ocp1Connection::sendMessage(const ByteVector& message)
{
    return writeData(const_cast<std::uint8_t*>(message.data()),
                     static_cast<int>(message.size()))
           == static_cast<int>(message.size());
}

int Ocp1Connection::writeData(void* data, int dataSize)
{
    std::shared_lock<std::shared_mutex> sl(socketLock);
    if (socket != nullptr)
        return socket->write(data, dataSize);
    return 0;
}


// ── Initialise ────────────────────────────────────────────────────────────────

void Ocp1Connection::initialise()
{
    safeAction->setSafe(true);
    threadIsRunning = true;
    connectionMadeInt();
    thread->startThread(m_threadPriority);
}

void Ocp1Connection::initialiseWithSocket(std::unique_ptr<NanoSocket> newSocket)
{
    // Assign the socket under the exclusive lock, then release before calling
    // initialise().  initialise() fires connectionMadeInt() which triggers the
    // onConnectionEstablished callback; that callback may call sendData() which
    // calls isConnected() → shared_lock.  std::shared_mutex is NOT reentrant, so
    // holding the unique_lock here while the callback runs would deadlock.
    {
        std::unique_lock<std::shared_mutex> sl(socketLock);
        assert(socket == nullptr);
        socket = std::move(newSocket);
    }
    initialise();
}


// ── Callback dispatch ─────────────────────────────────────────────────────────
// When useMessageThread is false, callbacks fire synchronously on the socket
// thread. When true, they are posted to `dispatcher` and run on its dedicated
// worker thread instead — see the constructor documentation in the header.

void Ocp1Connection::dispatchOrCall(std::function<void(Ocp1Connection&)> fn)
{
    if (useMessageThread && dispatcher)
    {
        // Capture safeAction by value so the guard (and the connection object it
        // refers to) stays valid for the lifetime of the queued task, even if
        // this Ocp1Connection is torn down before the task runs — ifSafe() will
        // simply no-op once setSafe(false) has been called.
        auto action = safeAction;
        dispatcher->post([action, fn]() { action->ifSafe(fn); });
    }
    else
    {
        safeAction->ifSafe(fn);
    }
}

void Ocp1Connection::connectionMadeInt()
{
    if (!callbackConnectionState)
    {
        callbackConnectionState = true;
        dispatchOrCall([](Ocp1Connection& owner) { owner.connectionMade(); });
    }
}

void Ocp1Connection::connectionLostInt()
{
    if (callbackConnectionState)
    {
        callbackConnectionState = false;
        dispatchOrCall([](Ocp1Connection& owner) { owner.connectionLost(); });
    }
}

void Ocp1Connection::deliverDataInt(const ByteVector& data)
{
    assert(callbackConnectionState);
    // Copy: when dispatched asynchronously this runs after the caller's local
    // buffer (see readNextMessage()) has gone out of scope.
    dispatchOrCall([data](Ocp1Connection& owner) { owner.messageReceived(data); });
}


// ── Read loop ─────────────────────────────────────────────────────────────────

int Ocp1Connection::readData(void* data, int num)
{
    std::shared_lock<std::shared_mutex> sl(socketLock);
    if (socket != nullptr)
        return socket->read(data, num, true);
    assert(false);
    return -1;
}

bool Ocp1Connection::readNextMessage()
{
    // Read enough data to fit an OCA header.
    ByteVector messageData(Ocp1Header::Ocp1HeaderSize);
    auto bytes = readData(messageData.data(), Ocp1Header::Ocp1HeaderSize);

    if (bytes == Ocp1Header::Ocp1HeaderSize)
    {
        Ocp1Header tmpHeader(messageData);

        // Resize to fit the complete OCA message (msgSize does not include the sync byte).
        messageData.resize(static_cast<size_t>(tmpHeader.GetMessageSize()) + 1);

        auto readPosition = static_cast<int>(Ocp1Header::Ocp1HeaderSize);
        auto bytesLeft    = static_cast<int>(tmpHeader.GetMessageSize() + 1
                                              - Ocp1Header::Ocp1HeaderSize);
        while (bytesLeft > 0)
        {
            if (thread->threadShouldExit())
                return false;

            auto numThisTime = std::min(bytesLeft, 65536);
            auto bytesIn     = readData(messageData.data() + readPosition, numThisTime);

            // A closed or broken connection mid-message (0 = peer closed gracefully,
            // < 0 = socket error) must be treated the same as a lost connection —
            // delivering a truncated frame to messageReceived() would be wrong.
            if (bytesIn <= 0)
            {
                if (socket != nullptr)
                    deleteSocket();

                connectionLostInt();
                return false;
            }

            readPosition += bytesIn;
            bytesLeft    -= bytesIn;
        }

        deliverDataInt(messageData);
        return true;
    }

    // bytes == 0: peer performed a graceful close (TCP FIN), typically observed
    // while idle between messages. bytes < 0: socket error. Both mean the
    // connection is gone and must be reported via connectionLostInt() so that
    // NanoOcp1Client retries and any dependent state machine (Ocp1Controller and
    // subclasses) is notified instead of silently going idle forever.
    if (bytes <= 0)
    {
        if (socket != nullptr)
            deleteSocket();

        connectionLostInt();
    }

    return false;
}

void Ocp1Connection::runThread()
{
    while (!thread->threadShouldExit())
    {
        if (socket != nullptr)
        {
            auto ready = socket->waitUntilReady(true, 100);

            if (ready < 0)
            {
                deleteSocket();
                connectionLostInt();
                break;
            }

            if (ready == 0)
            {
                thread->wait(1);
                continue;
            }
        }
        else
        {
            break;
        }

        if (thread->threadShouldExit() || !readNextMessage())
            break;
    }

    threadIsRunning = false;
}

} // namespace NanoOcp1
