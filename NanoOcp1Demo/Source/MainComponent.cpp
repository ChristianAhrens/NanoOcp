/* Copyright (c) 2020-2023, Christian Ahrens
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

#include "MainComponent.h"

#include "../../Source/NanoOcp1.h"
#include "../../Source/Ocp1DataTypes.h"
#include "../../Source/Ocp1ObjectDefinitions.h"


namespace NanoOcp1Demo
{

//==============================================================================
MainComponent::MainComponent()
{
    auto address = juce::String("127.0.0.1");
    auto port = 50014;

    // Create definitions to act as event handlers for the supported objects.
    m_pwrOnObjDef = std::make_unique<NanoOcp1::AmpDxDy::dbOcaObjectDef_Settings_PwrOn>();
    m_potiLevelObjDef = std::make_unique<NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel>(1);

    // Editor to allow user input for ip address and port to use to connect
    m_ipAndPortEditor = std::make_unique<TextEditor>();
    m_ipAndPortEditor->setTextToShowWhenEmpty(address + ";" + juce::String(port), getLookAndFeel().findColour(juce::TextEditor::ColourIds::textColourId).darker().darker());
    m_ipAndPortEditor->addListener(this);
    addAndMakeVisible(m_ipAndPortEditor.get());
    
    // connected status visu
    m_connectedLED = std::make_unique<TextButton>("con");
    m_connectedLED->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::forestgreen);
    m_connectedLED->setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::dimgrey);
    m_connectedLED->setToggleState(false, dontSendNotification);
    m_connectedLED->setEnabled(false);
    addAndMakeVisible(m_connectedLED.get());

    // Button for AddSubscription / RemoveSubscription
    m_subscribeButton = std::make_unique<TextButton>("Subscribe");
    m_subscribeButton->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::blue);
    m_subscribeButton->setClickingTogglesState(true);
    m_subscribeButton->onClick = [=]()
    {
        if (m_subscribeButton->getToggleState())
        {
            // Send AddSubscription requests
            std::uint32_t handle;
            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(
                NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel(1).AddSubscriptionCommand(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_potiLevelObjDef.get());
            DBG("Sent an OCA AddSubscription command with handle " << NanoOcp1::HandleToString(handle));

            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(
                NanoOcp1::AmpDxDy::dbOcaObjectDef_Settings_PwrOn().AddSubscriptionCommand(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_pwrOnObjDef.get());
            DBG("Sent an OCA AddSubscription command with handle " << NanoOcp1::HandleToString(handle));

            // Get initial values
            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(*m_pwrOnObjDef.get(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_pwrOnObjDef.get());
            DBG("Sent an OCA Get command with handle " << NanoOcp1::HandleToString(handle));

            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(*m_potiLevelObjDef.get(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_potiLevelObjDef.get());
            DBG("Sent an OCA Get command with handle " << NanoOcp1::HandleToString(handle));
        }
        else
        {
            // Send RemoveSubscription requests
            std::uint32_t handle;
            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(
                NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel(1).RemoveSubscriptionCommand(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_potiLevelObjDef.get());
            DBG("Sent an OCA RemoveSubscription command with handle " << NanoOcp1::HandleToString(handle));

            m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(
                NanoOcp1::AmpDxDy::dbOcaObjectDef_Settings_PwrOn().RemoveSubscriptionCommand(), handle).GetMemoryBlock());
            m_ocaHandleMap.emplace(handle, m_pwrOnObjDef.get());
            DBG("Sent an OCA RemoveSubscription command with handle " << NanoOcp1::HandleToString(handle));
        }
    };
    addAndMakeVisible(m_subscribeButton.get());

    // Button to act as Power display LED 
    m_powerD40LED = std::make_unique<TextButton>("Power LED");
    m_powerD40LED->setColour(juce::TextButton::ColourIds::buttonOnColourId, juce::Colours::forestgreen);
    m_powerD40LED->setColour(juce::TextButton::ColourIds::buttonColourId, juce::Colours::dimgrey);
    m_powerD40LED->setToggleState(false, dontSendNotification);
    m_powerD40LED->setEnabled(false);
    addAndMakeVisible(m_powerD40LED.get());

    // Button to power OFF the D40
    m_powerOffD40Button = std::make_unique<TextButton>("Pwr Off");
    m_powerOffD40Button->onClick = [=]() 
    {
        std::uint32_t handle;
        auto cmdDef(NanoOcp1::AmpDxDy::dbOcaObjectDef_Settings_PwrOn().SetValueCommand(0)); // 0 == OFF
        m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(cmdDef, handle).GetMemoryBlock());
    };
    addAndMakeVisible(m_powerOffD40Button.get());

    // Button to power ON the D40
    m_powerOnD40Button = std::make_unique<TextButton>("Pwr On");
    m_powerOnD40Button->onClick = [=]() 
    {
        std::uint32_t handle;
        auto cmdDef(NanoOcp1::AmpDxDy::dbOcaObjectDef_Settings_PwrOn().SetValueCommand(1)); // 1 == ON
        m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(cmdDef, handle).GetMemoryBlock());
    };
    addAndMakeVisible(m_powerOnD40Button.get());

    // Gain slider for Channel A
    m_gainSlider = std::make_unique<Slider>(Slider::LinearVertical, Slider::TextBoxBelow);
    m_gainSlider->setRange(-57.5, 6, 0.5);
    m_gainSlider->setTextValueSuffix("dB");
    m_gainSlider->onValueChange = [=]()
    {
        std::uint32_t handle;
        auto cmdDef(NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel(1).SetValueCommand(m_gainSlider->getValue()));
        m_nanoOcp1Client->sendData(NanoOcp1::Ocp1CommandResponseRequired(cmdDef, handle).GetMemoryBlock());
    };
    addAndMakeVisible(m_gainSlider.get());

    setSize(300, 200);

    // create the nano ocp1 client and fire it up
    m_nanoOcp1Client = std::make_unique<NanoOcp1::NanoOcp1Client>(address, port, true /* synch callbacks */);
    m_nanoOcp1Client->onDataReceived = [=](const juce::MemoryBlock& message)
    {
        return OnOcp1MessageReceived(message);
    };
    m_nanoOcp1Client->onConnectionEstablished = [=]()
    {
        if (m_connectedLED)
            m_connectedLED->setToggleState(true, juce::dontSendNotification);
    };
    m_nanoOcp1Client->onConnectionLost = [=]()
    {
        if (m_connectedLED)
            m_connectedLED->setToggleState(false, juce::dontSendNotification);
    };
    m_nanoOcp1Client->start();
}

bool MainComponent::OnOcp1MessageReceived(const juce::MemoryBlock& message)
{
    std::unique_ptr<NanoOcp1::Ocp1Message> msgObj = NanoOcp1::Ocp1Message::UnmarshalOcp1Message(message);
    if (msgObj)
    {
        switch (msgObj->GetMessageType())
        {
            case NanoOcp1::Ocp1Message::Notification:
                {
                    NanoOcp1::Ocp1Notification* notifObj = static_cast<NanoOcp1::Ocp1Notification*>(msgObj.get());

                    DBG("Got an OCA notification from ONo 0x" << juce::String::toHexString(notifObj->GetEmitterOno()));

                    // Update the right GUI element according to the definition of the object 
                    // which triggered the notification.
                    if (notifObj->MatchesObject(m_pwrOnObjDef.get()))
                    {
                        std::uint16_t switchSetting = NanoOcp1::DataToUint16(notifObj->GetParameterData());
                        m_powerD40LED->setToggleState(switchSetting > 0, dontSendNotification);
                    }
                    else if (notifObj->MatchesObject(m_potiLevelObjDef.get()))
                    {
                        std::float_t newGain = NanoOcp1::DataToFloat(notifObj->GetParameterData());
                        m_gainSlider->setValue(newGain, dontSendNotification);
                    }
                    else
                    {
                        DBG("Got an OCA notification from UNKNOWN object ONo 0x" << juce::String::toHexString(notifObj->GetEmitterOno()));
                    }

                    return true;
                }
            case NanoOcp1::Ocp1Message::Response:
                {
                    NanoOcp1::Ocp1Response* responseObj = static_cast<NanoOcp1::Ocp1Response*>(msgObj.get());

                    // Get the objDef matching the obtained response handle.
                    const auto iter = m_ocaHandleMap.find(responseObj->GetResponseHandle());
                    if (iter != m_ocaHandleMap.end())
                    {
                        if (responseObj->GetResponseStatus() != 0)
                        {
                            DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
                                " with status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()));
                        }
                        else if (responseObj->GetParamCount() == 0)
                        {
                            DBG("Got an empty \"OK\" OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()));
                        }
                        else
                        {
                            DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
                                " and paramCount " << juce::String(responseObj->GetParamCount()));

                            // Update the right GUI element according to the definition of the object 
                            // which triggered the response.
                            NanoOcp1::Ocp1CommandDefinition* objDef = iter->second;
                            if (objDef == m_pwrOnObjDef.get())
                            {
                                std::uint16_t switchSetting = NanoOcp1::DataToUint16(responseObj->GetParameterData());
                                m_powerD40LED->setToggleState(switchSetting > 0, dontSendNotification);
                            }
                            else if (objDef == m_potiLevelObjDef.get())
                            {
                                std::float_t newGain = NanoOcp1::DataToFloat(responseObj->GetParameterData());
                                m_gainSlider->setValue(newGain, dontSendNotification);
                            }
                            else
                            {
                                DBG("Got an OCA response for handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
                                    " which could not be processed (unknown object)!");
                            }
                        }

                        // Finally remove handle from the list, as it has been processed.
                        m_ocaHandleMap.erase(iter);
                    }
                    else
                    {
                        DBG("Got an OCA response for UNKNOWN handle " << NanoOcp1::HandleToString(responseObj->GetResponseHandle()) <<
                            "; status " << NanoOcp1::StatusToString(responseObj->GetResponseStatus()) <<
                            "; paramCount " << juce::String(responseObj->GetParamCount()));
                    }

                    return true;
                }
            case NanoOcp1::Ocp1Message::KeepAlive:
                {
                    // Reset online timer

                    return true;
                }
            default:
                break;
        }
    }

    return false;
}

MainComponent::~MainComponent()
{
    m_nanoOcp1Client->onDataReceived = std::function<bool(const MemoryBlock&)>();
    m_nanoOcp1Client->onConnectionEstablished = std::function<void()>();
    m_nanoOcp1Client->onConnectionLost = std::function<void()>();
    m_nanoOcp1Client->stop();
}

void MainComponent::textEditorReturnKeyPressed(TextEditor& editor)
{
    if (&editor == m_ipAndPortEditor.get())
    {
        auto ip = editor.getText().upToFirstOccurrenceOf(";", false, true);
        auto port = editor.getText().fromLastOccurrenceOf(";", false, true).getIntValue();
        
        m_nanoOcp1Client->stop();
        m_nanoOcp1Client->setAddress(ip);
        m_nanoOcp1Client->setPort(port);
        m_nanoOcp1Client->start();
    }
}

void MainComponent::resized()
{
    auto bounds = getLocalBounds();

    auto connectionParamsHeight = 35;
    auto connectionLedWidth = 60;

    auto textEditorBounds = bounds.removeFromTop(connectionParamsHeight);
    auto connectedLedBounds = textEditorBounds.removeFromRight(connectionLedWidth);
    m_connectedLED->setBounds(connectedLedBounds.reduced(5));
    m_ipAndPortEditor->setBounds(textEditorBounds.reduced(5));

    auto sliderBounds = bounds.removeFromRight(connectionLedWidth);
    m_gainSlider->setBounds(sliderBounds.reduced(5));

    auto button1Bounds = bounds;
    auto button2Bounds = button1Bounds.removeFromRight(button1Bounds.getWidth() / 2);
    auto button3Bounds = button2Bounds.removeFromBottom(button2Bounds.getHeight() / 2);
    auto button4Bounds = button1Bounds.removeFromBottom(button1Bounds.getHeight() / 2);

    button1Bounds.reduce(5, 5);
    button2Bounds.reduce(5, 5);
    button3Bounds.reduce(5, 5);
    button4Bounds.reduce(5, 5);

    m_subscribeButton->setBounds(button1Bounds);
    m_powerD40LED->setBounds(button2Bounds);
    m_powerOffD40Button->setBounds(button3Bounds);
    m_powerOnD40Button->setBounds(button4Bounds);
}

}
