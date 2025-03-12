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

#ifdef JUCE_GLOBAL_MODULE_SETTINGS_INCLUDED
    #include <juce_core/juce_core.h>
    #include <juce_events/juce_events.h>
#else
    #include <JuceHeader.h>
#endif


namespace NanoOcp1
{


struct Ocp1Connection::ConnectionThread : public juce::Thread
{
    ConnectionThread(Ocp1Connection& c) : juce::Thread("JUCE IPC"), owner(c) {}
    void run() override { owner.runThread(); }

    Ocp1Connection& owner;
    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionThread)
};

class SafeActionImpl
{
public:
    explicit SafeActionImpl(Ocp1Connection& p)
        : ref(p) {}

    template <typename Fn>
    void ifSafe(Fn&& fn)
    {
        const juce::ScopedLock lock(mutex);

        if (safe)
            fn(ref);
    }

    void setSafe(bool s)
    {
        const juce::ScopedLock lock(mutex);
        safe = s;
    }

    bool isSafe()
    {
        const juce::ScopedLock lock(mutex);
        return safe;
    }

private:
    juce::CriticalSection mutex;
    Ocp1Connection& ref;
    bool safe = false;
};

class Ocp1Connection::SafeAction : public SafeActionImpl
{
    using SafeActionImpl::SafeActionImpl;
};

//==============================================================================
Ocp1Connection::Ocp1Connection(bool callbacksOnMessageThread , const juce::Thread::Priority threadPriority)
    : useMessageThread(callbacksOnMessageThread),
    safeAction(std::make_shared<SafeAction>(*this)), m_threadPriority(threadPriority)
{
    thread.reset(new ConnectionThread(*this));
}

Ocp1Connection::~Ocp1Connection()
{
    // You *must* call `disconnect` in the destructor of your derived class to ensure
    // that any pending messages are not delivered. If the messages were delivered after
    // destroying the derived class, we'd end up calling the pure virtual implementations
    // of `messageReceived`, `connectionMade` and `connectionLost` which is definitely
    // not a good idea!
    jassert(!safeAction->isSafe());

    callbackConnectionState = false;
    disconnect(4000, Notify::no);
    thread.reset();
}

//==============================================================================
bool Ocp1Connection::connectToSocket(const juce::String& hostName,
    int portNumber, int timeOutMillisecs)
{
    disconnect(1000);

    auto s = std::make_unique<juce::StreamingSocket>();

    if (s->connect(hostName, portNumber, timeOutMillisecs))
    {
        const juce::ScopedWriteLock sl(socketLock);
        initialiseWithSocket(std::move(s));
        return true;
    }

    return false;
}

void Ocp1Connection::disconnect(int timeoutMs, Notify notify)
{
    //should be called before socket->close to ensure that running processes on the thread
    //are notified that the thread is about to exit.
    thread->stopThread(timeoutMs);
    
    {
        const juce::ScopedReadLock sl(socketLock);
        if (socket != nullptr)  socket->close();
    }
    
    deleteSocket();

    if (notify == Notify::yes)
        connectionLostInt();

    callbackConnectionState = false;
    safeAction->setSafe(false);
}

void Ocp1Connection::deleteSocket()
{
    const juce::ScopedWriteLock sl(socketLock);
    socket.reset();
}

bool Ocp1Connection::isConnected() const
{
    const juce::ScopedReadLock sl(socketLock);

    return (socket != nullptr && socket->isConnected())
        && threadIsRunning;
}

juce::String Ocp1Connection::getConnectedHostName() const
{
    {
        const juce::ScopedReadLock sl(socketLock);

        if (socket == nullptr)
            return {};

        if (socket != nullptr && !socket->isLocal())
            return socket->getHostName();
    }

    return juce::IPAddress::local().toString();
}

//==============================================================================
bool Ocp1Connection::sendMessage(const juce::MemoryBlock& message)
{
    juce::MemoryBlock messageData(message.getSize());
    messageData.copyFrom(message.getData(), 0, message.getSize());

    return writeData(messageData.getData(), (int)messageData.getSize()) == (int)messageData.getSize();
}

int Ocp1Connection::writeData(void* data, int dataSize)
{
    const juce::ScopedReadLock sl(socketLock);

    if (socket != nullptr)
        return socket->write(data, dataSize);

    return 0;
}

//==============================================================================
void Ocp1Connection::initialise()
{
    safeAction->setSafe(true);
    threadIsRunning = true;
    connectionMadeInt();
    thread->startThread(m_threadPriority);
}

void Ocp1Connection::initialiseWithSocket(std::unique_ptr<juce::StreamingSocket> newSocket)
{
    jassert(socket == nullptr);
    socket = std::move(newSocket);
    initialise();
}

//==============================================================================
struct ConnectionStateMessage : public juce::MessageManager::MessageBase
{
    ConnectionStateMessage(std::shared_ptr<SafeActionImpl> ipc, bool connected) noexcept
        : safeAction(ipc), connectionMade(connected)
    {}

    void messageCallback() override
    {
        safeAction->ifSafe([this](Ocp1Connection& owner)
            {
                if (connectionMade)
                    owner.connectionMade();
                else
                    owner.connectionLost();
            });
    }

    std::shared_ptr<SafeActionImpl> safeAction;
    bool connectionMade;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(ConnectionStateMessage)
};

void Ocp1Connection::connectionMadeInt()
{
    if (!callbackConnectionState)
    {
        callbackConnectionState = true;

        if (useMessageThread)
            (new ConnectionStateMessage(safeAction, true))->post();
        else
            connectionMade();
    }
}

void Ocp1Connection::connectionLostInt()
{
    if (callbackConnectionState)
    {
        callbackConnectionState = false;

        if (useMessageThread)
            (new ConnectionStateMessage(safeAction, false))->post();
        else
            connectionLost();
    }
}

struct DataDeliveryMessage : public juce::Message
{
    DataDeliveryMessage(std::shared_ptr<SafeActionImpl> ipc, const juce::MemoryBlock& d)
        : safeAction(ipc), data(d)
    {}

    void messageCallback() override
    {
        safeAction->ifSafe([this](Ocp1Connection& owner)
            {
                owner.messageReceived(data);
            });
    }

    std::shared_ptr<SafeActionImpl> safeAction;
    juce::MemoryBlock data;
};

void Ocp1Connection::deliverDataInt(const juce::MemoryBlock& data)
{
    jassert(callbackConnectionState);

    if (useMessageThread)
        (new DataDeliveryMessage(safeAction, data))->post();
    else
        messageReceived(data);
}

//==============================================================================
int Ocp1Connection::readData(void* data, int num)
{
    const juce::ScopedReadLock sl(socketLock);

    if (socket != nullptr)
        return socket->read(data, num, true);

    jassertfalse;
    return -1;
}

bool Ocp1Connection::readNextMessage()
{
    // Read enough data to fit an OCA header.
    juce::MemoryBlock messageData((size_t)Ocp1Header::Ocp1HeaderSize);
    auto bytes = readData(messageData.getData(), Ocp1Header::Ocp1HeaderSize);

    if (bytes == Ocp1Header::Ocp1HeaderSize)
    {
        // Unmarshal the OCA header using a Ocp1Header helper object.
        Ocp1Header tmpHeader(messageData);

        // Resize the MemoryBlock to fit the complete OCA message.
        // NOTE: msgSize does not include the sync byte.
        messageData.setSize(static_cast<size_t>(tmpHeader.GetMessageSize()) + 1);

        auto readPosition = static_cast<int>(Ocp1Header::Ocp1HeaderSize);
        auto bytesLeft = static_cast<int>(tmpHeader.GetMessageSize() + 1 - Ocp1Header::Ocp1HeaderSize);
        while (bytesLeft > 0)
        {
            if (thread->threadShouldExit())
                return false;

            auto numThisTime = juce::jmin(bytesLeft, 65536);
            auto bytesIn = readData(juce::addBytesToPointer(messageData.getData(), readPosition), numThisTime);

            if (bytesIn <= 0)
                break;

            readPosition += bytesIn;
            bytesLeft -= bytesIn;
        }

        deliverDataInt(messageData);

        return true;
    }

    if (bytes < 0)
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

}