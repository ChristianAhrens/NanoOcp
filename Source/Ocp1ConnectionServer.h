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


//==============================================================================
/**
    Class taken from JUCE and modified to act as regular TCP connection without
    the magic header contents and all pipe related methods, etc.

    @see NanoOcp1::Ocp1Connection
*/
class Ocp1ConnectionServer : private juce::Thread
{
public:
    //==============================================================================
    Ocp1ConnectionServer(const juce::Thread::Priority threadPriority=juce::Thread::Priority::normal);
    ~Ocp1ConnectionServer() override;

    bool beginWaitingForSocket(int portNumber, const juce::String& bindAddress = juce::String());
    void stop();
    int getBoundPort() const noexcept;

protected:
    //==============================================================================
    virtual Ocp1Connection* createConnectionObject() = 0;

private:
    //==============================================================================
    std::unique_ptr<juce::StreamingSocket> socket;

    void run() override;
    
    juce::Thread::Priority m_threadPriority;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR(Ocp1ConnectionServer)
};

}