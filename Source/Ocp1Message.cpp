/* Copyright (c) 2023, Bernardo Escalona
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
 
#include "Ocp1Message.h"


namespace NanoOcp1
{

//==============================================================================
// Class Ocp1CommandDefinition
//==============================================================================

Ocp1CommandDefinition Ocp1CommandDefinition::AddSubscriptionCommand() const
{
    return Ocp1CommandDefinition(0x00000004,                     // ONO of OcaSubscriptionManager
                                 m_propertyType,
                                 3,                              // OcaSubscriptionManager level
                                 1,                              // AddSubscription method
                                 5,                              // 5 Params 
                                 DataFromOnoForSubscription(m_targetOno, true));
}

Ocp1CommandDefinition Ocp1CommandDefinition::RemoveSubscriptionCommand() const
{
    return Ocp1CommandDefinition(0x00000004,                     // ONO of OcaSubscriptionManager
                                 m_propertyType,
                                 3,                              // OcaSubscriptionManager level
                                 2,                              // RemoveSubscription method
                                 2,                              // 2 Params 
                                 DataFromOnoForSubscription(m_targetOno, false));
}

Ocp1CommandDefinition Ocp1CommandDefinition::GetValueCommand() const
{
    return Ocp1CommandDefinition(m_targetOno,
                                 m_propertyType,
                                 m_propertyDefLevel,
                                 1,                              // Get method is usually MethodIdx 1
                                 0,                              // 0 Param
                                 std::vector<std::uint8_t>());   // Empty parameters
}

Ocp1CommandDefinition Ocp1CommandDefinition::SetValueCommand(const Variant& newValue) const
{
    std::vector<std::uint8_t> newParamData = newValue.ToParamData(GetDataType());

    return Ocp1CommandDefinition(m_targetOno,
                                 m_propertyType,
                                 m_propertyDefLevel,
                                 2,                     // Set method is usually MethodIdx 2
                                 1,                     // Set method usually takes one parameter
                                 newParamData);
}

Ocp1CommandDefinition* Ocp1CommandDefinition::Clone() const
{
    return new Ocp1CommandDefinition(*this);
}


//==============================================================================
// Class Ocp1Header
//==============================================================================

Ocp1Header::Ocp1Header(const juce::MemoryBlock& memoryBlock)
    :   m_syncVal(static_cast<std::uint8_t>(0)),
        m_protoVers(static_cast<std::uint16_t>(0)),
        m_msgSize(static_cast<std::uint32_t>(0)),
        m_msgType(static_cast<std::uint8_t>(0)),
        m_msgCnt(static_cast<std::uint16_t>(0))
{
    jassert(memoryBlock.getSize() >= 10); // Not enough data to fit even a Ocp1Header.
    if (memoryBlock.getSize() >= 10)
    {
        m_syncVal = static_cast<std::uint8_t>(memoryBlock[0]);
        jassert(m_syncVal == 0x3b); // Message does not start with the sync byte.

        m_protoVers = ReadUint16(memoryBlock.begin() + 1);
        jassert(m_protoVers == 1); // Protocol version is expected to be 1.
        
        m_msgSize = ReadUint32(memoryBlock.begin() + 3);
        jassert(m_msgSize >= Ocp1HeaderSize); // Message has unexpected size.

        m_msgType = static_cast<std::uint8_t>(memoryBlock[7]);
        jassert(m_msgType <= Ocp1Message::KeepAlive); // Message type outside expected range.

        m_msgCnt = ReadUint16(memoryBlock.begin() + 8);
        jassert(m_msgCnt > 0); // At least one message expected. 
    }
}

bool Ocp1Header::IsValid() const
{
    return ((m_syncVal == 0x3b) && (m_protoVers == 1) && (m_msgSize >= Ocp1HeaderSize) &&
            (m_msgType <= Ocp1Message::KeepAlive) && (m_msgCnt > 0));
}

std::vector<std::uint8_t> Ocp1Header::GetSerializedData() const
{
    std::vector<std::uint8_t> serializedData;

    serializedData.push_back(m_syncVal);
    serializedData.push_back(static_cast<std::uint8_t>(m_protoVers >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_protoVers));
    serializedData.push_back(static_cast<std::uint8_t>(m_msgSize >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(m_msgSize >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_msgSize >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_msgSize));
    serializedData.push_back(m_msgType);
    serializedData.push_back(static_cast<std::uint8_t>(m_msgCnt >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_msgCnt));

    return serializedData;
}

std::uint32_t Ocp1Header::CalculateMessageSize(std::uint8_t msgType, size_t parameterDataLength)
{
    std::uint32_t ret(0);

    switch (msgType)
    {
        case Ocp1Message::Command:
        case Ocp1Message::CommandResponseRequired:
            ret = static_cast<std::uint32_t>(26 + parameterDataLength);
            break;
        case Ocp1Message::Notification:
            ret = static_cast<std::uint32_t>(37 + parameterDataLength);
            break;
        case Ocp1Message::Response:
            ret = static_cast<std::uint32_t>(19 + parameterDataLength);
            break;
        case Ocp1Message::KeepAlive:
            ret = static_cast<std::uint32_t>(9 + parameterDataLength);
            break;
        default:
            break;
    }

    return ret;
}


//==============================================================================
// Class Ocp1Message
//==============================================================================

// OCA_INVALID_SESSIONID  == 0, OCA_LOCAL_SESSIONID == 1
std::uint32_t Ocp1Message::m_nextHandle = 2;

std::unique_ptr<Ocp1Message> Ocp1Message::UnmarshalOcp1Message(const juce::MemoryBlock& receivedData)
{
    Ocp1Header header(receivedData);
    if (!header.IsValid())
        return nullptr;

    switch (header.GetMessageType())
    {
        case Notification:
            {
                std::uint32_t notificationSize(ReadUint32(receivedData.begin() + 10));
                std::uint32_t newValueSize = notificationSize - 28;
                if (newValueSize < 1)
                    return nullptr;

                // Not a valid object number.
                std::uint32_t targetOno(ReadUint32(receivedData.begin() + 14));
                if (targetOno == 0)
                    return nullptr;

                // Method DefinitionLevel expected to be 3 (OcaSubscriptionManager)
                std::uint16_t methodDefLevel(ReadUint16(receivedData.begin() + 18));
                if (methodDefLevel < 1)
                    return nullptr;

                // Method index expected to be 1 (AddSubscription)
                std::uint16_t methodIdx(ReadUint16(receivedData.begin() + 20));
                if (methodIdx < 1)
                    return nullptr;

                // At least one parameter expected.
                std::uint8_t paramCount = static_cast<std::uint8_t>(receivedData[22]);
                if (paramCount < 1)
                    return nullptr;

                std::uint16_t contextSize(ReadUint16(receivedData.begin() + 23));

                // Not a valid object number.
                std::uint32_t emitterOno(ReadUint32(receivedData.begin() + 25 + contextSize));
                if (emitterOno == 0)
                    return nullptr;

                // Event definiton level expected to be 1 (OcaRoot).
                std::uint16_t eventDefLevel(ReadUint16(receivedData.begin() + 29 + contextSize));
                if (eventDefLevel != 1)
                    return nullptr;

                // Event index expected to be 1 (OCA_EVENT_PROPERTY_CHANGED).
                std::uint16_t eventIdx(ReadUint16(receivedData.begin() + 31 + contextSize));
                if (eventIdx != 1)
                    return nullptr;

                // Property definition level expected to be > 0.
                std::uint16_t propDefLevel(ReadUint16(receivedData.begin() + 33 + contextSize));
                if (propDefLevel == 0)
                    return nullptr;

                // Property index expected to be > 0.
                std::uint16_t propIdx(ReadUint16(receivedData.begin() + 35 + contextSize));
                if (propIdx == 0)
                    return nullptr;

                std::vector<std::uint8_t> parameterData;
                parameterData.reserve(newValueSize);
                for (std::uint32_t i = 0; i < newValueSize; i++) // TODO: check if this can be optimized via memcpy
                {
                    parameterData.push_back(static_cast<std::uint8_t>(receivedData[37 + contextSize + i]));
                }

                return std::make_unique<Ocp1Notification>(emitterOno, propDefLevel, propIdx, paramCount, parameterData);
            }

        case Response:
            {
                std::uint32_t responseSize(ReadUint32(receivedData.begin() + 10));
                std::uint32_t parameterDataLength = responseSize - 10;
                if (responseSize < 10)
                    return nullptr;

                // Not a valid handle.
                std::uint32_t handle(ReadUint32(receivedData.begin() + 14));
                if (handle == 0)
                    return nullptr;

                std::uint8_t status = static_cast<std::uint8_t>(receivedData[18]);
                std::uint8_t paramCount = static_cast<std::uint8_t>(receivedData[19]);

                std::vector<std::uint8_t> parameterData;
                if (parameterDataLength > 0)
                {
                    parameterData.reserve(parameterDataLength);
                    for (std::uint32_t i = 0; i < parameterDataLength; i++)
                    {
                        parameterData.push_back(static_cast<std::uint8_t>(receivedData[20 + i]));
                    }
                }

                return std::make_unique<Ocp1Response>(handle, status, paramCount, parameterData);
            }

        case KeepAlive:
            {
                std::uint16_t heartbeat(ReadUint16(receivedData.begin() + 10));

                return std::make_unique<Ocp1KeepAlive>(heartbeat);
            }

        case Command:
        case CommandResponseRequired:
            {
                // TODO
                return nullptr;
            }
        default:
            return nullptr;
    }
}



//==============================================================================
// Class Ocp1CommandResponseRequired
//==============================================================================

std::vector<std::uint8_t> Ocp1CommandResponseRequired::GetSerializedData()
{
    std::vector<std::uint8_t> serializedData = m_header.GetSerializedData();

    std::uint32_t commandSize(m_header.GetMessageSize() - 9); // Message size minus the header
    serializedData.push_back(static_cast<std::uint8_t>(commandSize >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(commandSize >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(commandSize >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(commandSize));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle));
    serializedData.push_back(static_cast<std::uint8_t>(m_targetOno >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(m_targetOno >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_targetOno >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_targetOno));
    serializedData.push_back(static_cast<std::uint8_t>(m_methodDefLevel >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_methodDefLevel));
    serializedData.push_back(static_cast<std::uint8_t>(m_methodIndex >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_methodIndex));
    serializedData.push_back(static_cast<std::uint8_t>(m_paramCount));
    for (size_t i = 0; i < m_parameterData.size(); i++)
    {
        serializedData.push_back(m_parameterData[i]);
    }

    return serializedData;
}



//==============================================================================
// Class Ocp1Response
//==============================================================================

std::vector<std::uint8_t> Ocp1Response::GetSerializedData()
{
    std::vector<std::uint8_t> serializedData = m_header.GetSerializedData();

    std::uint32_t responseSize(m_header.GetMessageSize() - 9); // Message size minus the header
    serializedData.push_back(static_cast<std::uint8_t>(responseSize >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(responseSize >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(responseSize >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(responseSize));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_handle));
    serializedData.push_back(static_cast<std::uint8_t>(m_status));
    serializedData.push_back(static_cast<std::uint8_t>(m_paramCount));
    for (size_t i = 0; i < m_parameterData.size(); i++)
    {
        serializedData.push_back(m_parameterData[i]);
    }

    return serializedData;
}



//==============================================================================
// Class Ocp1Notification
//==============================================================================

std::vector<std::uint8_t> Ocp1Notification::GetSerializedData()
{
    std::vector<std::uint8_t> serializedData = m_header.GetSerializedData();

    std::uint32_t notificationSize(m_header.GetMessageSize() - 9); // Message size minus the header
    serializedData.push_back(static_cast<std::uint8_t>(notificationSize >> 24));
    serializedData.push_back(static_cast<std::uint8_t>(notificationSize >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(notificationSize >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(notificationSize));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 24)); // TargetOno
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno));
    std::uint16_t methodDefLevel = 3; // OcaSubscriptionManager
    serializedData.push_back(static_cast<std::uint8_t>(methodDefLevel >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(methodDefLevel));
    std::uint16_t methodIdx = 1; // AddSubscription
    serializedData.push_back(static_cast<std::uint8_t>(methodIdx >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(methodIdx));
    std::uint16_t paramCount = 2;
    serializedData.push_back(static_cast<std::uint8_t>(paramCount));
    std::uint16_t contextLength = 0;
    serializedData.push_back(static_cast<std::uint8_t>(contextLength >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(contextLength));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 24)); // EmitterOno
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 16));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterOno));
    std::uint16_t eventDefLevel = 1; // OcaRoot level
    serializedData.push_back(static_cast<std::uint8_t>(eventDefLevel >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(eventDefLevel));
    std::uint16_t eventIdx = 1; // PropertyChanged event
    serializedData.push_back(static_cast<std::uint8_t>(eventIdx >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(eventIdx));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterPropertyDefLevel >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterPropertyDefLevel));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterPropertyIndex >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_emitterPropertyIndex));
    for (size_t i = 0; i < m_parameterData.size(); i++)
    {
        serializedData.push_back(m_parameterData[i]);
    }
    serializedData.push_back(static_cast<std::uint8_t>(1)); // Ending byte

    return serializedData;
}



//==============================================================================
// Class Ocp1KeepAlive
//==============================================================================

Ocp1KeepAlive::Ocp1KeepAlive(std::uint16_t heartBeatSeconds)
    : Ocp1Message(static_cast<std::uint8_t>(KeepAlive), 
                  DataFromUint16(heartBeatSeconds))
{
}

Ocp1KeepAlive::Ocp1KeepAlive(std::uint32_t heartBeatMilliseconds)
    : Ocp1Message(static_cast<std::uint8_t>(KeepAlive), 
                  DataFromUint32(heartBeatMilliseconds))
{
}

std::uint16_t Ocp1KeepAlive::GetHeartBeatSeconds() const
{
    if (m_parameterData.size() == sizeof(std::uint16_t))
    {
        return DataToUint16(m_parameterData);
    }

    return 0;
}

std::uint32_t Ocp1KeepAlive::GetHeartBeatMilliseconds() const
{
    if (m_parameterData.size() == sizeof(std::uint32_t))
    {
        return DataToUint32(m_parameterData);
    }

    return 0;
}

std::vector<std::uint8_t> Ocp1KeepAlive::GetSerializedData()
{
    std::vector<std::uint8_t> serializedData = m_header.GetSerializedData();
    serializedData.insert(serializedData.end(), m_parameterData.begin(), m_parameterData.end());
    
    return serializedData;
}

}
