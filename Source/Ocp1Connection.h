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


//==============================================================================
/**
    Class taken from JUCE and modified to act as regular TCP connection without
    the magic header contents and all pipe related methods, etc.
*/
class Ocp1Connection
{
public:
    /** Whether the disconnect call should trigger callbacks. */
    enum class Notify { no, yes };

public:
    Ocp1Connection(bool callbacksOnMessageThread = true, const juce::Thread::Priority threadPriority = juce::Thread::Priority::normal);
    virtual ~Ocp1Connection();

    bool connectToSocket(const juce::String& hostName, int portNumber, int timeOutMillisecs);
    void disconnect(int timeoutMs = 0, Notify notify = Notify::yes);
    bool isConnected() const;
    juce::StreamingSocket* getSocket() const noexcept { return socket.get(); }
    juce::String getConnectedHostName() const;
    bool sendMessage(const ByteVector& message);

    //==============================================================================
    virtual void connectionMade() = 0;
    virtual void connectionLost() = 0;
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