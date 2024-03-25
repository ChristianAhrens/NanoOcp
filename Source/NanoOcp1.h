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

#pragma once

#include <JuceHeader.h>

#include "Ocp1Connection.h"
#include "Ocp1ConnectionServer.h"


namespace NanoOcp1
{

class NanoOcp1Base
{
public:
    //==============================================================================
    NanoOcp1Base(const juce::String& address, const int port);
    virtual ~NanoOcp1Base();

    void setAddress(const juce::String& address);
    const juce::String& getAddress();

    void setPort(const int port);
    const int getPort();

    //==============================================================================
    virtual bool start() = 0;
    virtual bool stop() = 0;

    //==============================================================================
    virtual bool sendData(const juce::MemoryBlock& data) = 0;

    //==============================================================================
    std::function<bool(const juce::MemoryBlock&)> onDataReceived;
    std::function<void()> onConnectionEstablished;
    std::function<void()> onConnectionLost;

protected:
    //==============================================================================
    bool processReceivedData(const juce::MemoryBlock& data);

private:
    //==============================================================================
    juce::String    m_address;
    int             m_port{ 0 };

};

class NanoOcp1Client : public NanoOcp1Base, public Ocp1Connection, public juce::Timer
{
public:
    //==============================================================================
    NanoOcp1Client(const bool callbacksOnMessageThread);
    NanoOcp1Client(const juce::String& address, const int port, const bool callbacksOnMessageThread);
    ~NanoOcp1Client() override;

    //==============================================================================
    bool start() override;
    bool stop() override;

    //==============================================================================
    bool sendData(const juce::MemoryBlock& data) override;

    //==============================================================================
    void connectionMade() override;
    void connectionLost() override;
    void messageReceived(const juce::MemoryBlock& message) override;

protected:
    //==============================================================================
    void timerCallback() override;

private:
    //==============================================================================
    bool m_started{ false };
};

class NanoOcp1Server : public NanoOcp1Base, public Ocp1ConnectionServer
{
public:
    //==============================================================================
    NanoOcp1Server(const bool callbacksOnMessageThread);
    NanoOcp1Server(const juce::String& address, const int port, const bool callbacksOnMessageThread);
    ~NanoOcp1Server() override;

    //==============================================================================
    bool start() override;
    bool stop() override;

    //==============================================================================
    bool sendData(const juce::MemoryBlock& data) override;

protected:
    //==============================================================================
    Ocp1Connection* createConnectionObject() override;

private:
    //==============================================================================
    std::unique_ptr<NanoOcp1Client> m_activeConnection;
    bool m_callbacksOnMessageThread{ true };
};

}
