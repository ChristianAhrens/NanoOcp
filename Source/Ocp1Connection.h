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

#include <JuceHeader.h>


namespace NanoOcp1
{


class Ocp1ConnectionServer;
class MemoryBlock;


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
    Ocp1Connection(bool callbacksOnMessageThread = true);
    virtual ~Ocp1Connection();

    bool connectToSocket(const juce::String& hostName, int portNumber, int timeOutMillisecs);
    void disconnect(int timeoutMs = -1, Notify notify = Notify::yes);
    bool isConnected() const;
    juce::StreamingSocket* getSocket() const noexcept { return socket.get(); }
    juce::String getConnectedHostName() const;
    bool sendMessage(const juce::MemoryBlock& message);

    //==============================================================================
    virtual void connectionMade() = 0;
    virtual void connectionLost() = 0;
    virtual void messageReceived(const juce::MemoryBlock& message) = 0;

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
    void deliverDataInt(const juce::MemoryBlock&);
    bool readNextMessage();
    int readData(void*, int);

    struct ConnectionThread;
    std::unique_ptr<ConnectionThread> thread;
    std::atomic<bool> threadIsRunning{ false };

    class SafeAction;
    std::shared_ptr<SafeAction> safeAction;

    void runThread();
    int writeData(void*, int);

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ocp1Connection)
};

}