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


namespace NanoOcp1
{
    class NanoOcp1;
    class NanoOcp1Client;
    struct Ocp1CommandDefinition;
}

namespace NanoOcp1Demo
{

//==============================================================================
/*
    This component lives inside our window, and this is where you should put all
    your controls and content.
*/
class MainComponent   : public Component, public juce::TextEditor::Listener
{
public:
    //==============================================================================
    MainComponent();
    ~MainComponent();

    //==============================================================================
    void resized() override;

    //==============================================================================
    void textEditorReturnKeyPressed(TextEditor& editor) override;

    //==============================================================================
    bool OnOcp1MessageReceived(const juce::MemoryBlock& message);
    
private:
    //==============================================================================
    std::unique_ptr<NanoOcp1::NanoOcp1Client>   m_nanoOcp1Client;

    std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>   m_pwrOnObjDef;
    std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>   m_potiLevelObjDef;
    std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>   m_soundobjectEnableObjDef;
    std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>   m_speakerGroupObjDef;
    std::unique_ptr<NanoOcp1::Ocp1CommandDefinition>   m_guidObjDef;
    std::map<std::uint32_t, NanoOcp1::Ocp1CommandDefinition*> m_ocaHandleMap;

    //==============================================================================
    std::unique_ptr<TextEditor>     m_ipAndPortEditor;
    std::unique_ptr<TextButton>     m_connectedLED;
    std::unique_ptr<TextButton>     m_subscribeButton;
    std::unique_ptr<TextButton>     m_powerD40LED;
    std::unique_ptr<TextButton>     m_powerOnD40Button;
    std::unique_ptr<TextButton>     m_powerOffD40Button;
    std::unique_ptr<Slider>         m_gainSlider;

    JUCE_DECLARE_NON_COPYABLE_WITH_LEAK_DETECTOR (MainComponent)
};

}
