/* Copyright (c) 2025, Bernardo Escalona
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

#include <memory>

#include "Variant.h"
#include "Ocp1DataTypes.h" //< USE Ocp1DataType


namespace NanoOcp1
{

/**
 * @struct Ocp1CommandDefinition
 * @brief Parameter bundle that fully describes one OCA controllable property.
 *
 * Every OCA property on a device is identified by three coordinates in the AES70
 * class hierarchy:
 * - **Target ONo** — which object (encoded device address: type, channel, box).
 * - **Property def-level** — which class in the inheritance chain defines the property
 *   (e.g. `DefLevel_OcaGain = 4` for gain properties defined at the OcaGain class).
 * - **Property index** — which property within that class (e.g. `1` = `Prop_Gain`).
 *
 * `Ocp1CommandDefinition` stores these plus the data type and any fixed parameter bytes,
 * so that the four factory methods (`AddSubscriptionCommand()`, `RemoveSubscriptionCommand()`,
 * `GetValueCommand()`, `SetValueCommand()`) can produce ready-to-send
 * `Ocp1CommandResponseRequired` objects without the caller having to manually build
 * the binary parameter data.
 *
 * ## Concrete subclasses
 * Each controllable parameter on each device type has a dedicated struct in
 * `Ocp1ObjectDefinitions.h` and `Ocp1DS100ObjectDefinitions.h`, e.g.:
 * ```cpp
 * // Instantiate for sound object channel 5 on a DS100
 * NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position def(5);
 *
 * // Build and send a GetValue command:
 * uint32_t handle;
 * auto cmd = Ocp1CommandResponseRequired(def.GetValueCommand(), handle);
 * client->sendData(cmd.GetSerializedData());
 *
 * // Build and send a SetValue command:
 * Variant newPos(0.5f, 0.5f, 0.0f);
 * auto setCmd = Ocp1CommandResponseRequired(def.SetValueCommand(newPos), handle);
 * client->sendData(setCmd.GetSerializedData());
 * ```
 *
 * ## DeviceController (Umsci) usage
 * `DeviceController::CreateKnownONosMap()` pre-constructs one `Ocp1CommandDefinition`
 * per (RemObjIdent, channel/record address) pair, keyed by ONo.  The reverse map
 * (ONo → RemObjIdent) allows incoming Notifications and Responses to be matched
 * back to logical parameter names without a linear search.
 */
struct Ocp1CommandDefinition
{
    /**
     * Standard struct constructor.
     */
    Ocp1CommandDefinition()
        :   m_targetOno(static_cast<std::uint32_t>(0)),
            m_propertyType(static_cast<std::uint16_t>(0)),
            m_propertyDefLevel(static_cast<std::uint16_t>(0)),
            m_propertyIndex(static_cast<std::uint16_t>(0)),
            m_paramCount(static_cast<std::uint8_t>(0))
    {
    }

    /**
     * Parameterized struct constructor.
     */
    Ocp1CommandDefinition(std::uint32_t targetOno,
                          std::uint16_t propertyType,
                          std::uint16_t propertyDefLevel,
                          std::uint16_t propertyIndex,
                          std::uint8_t paramCount = static_cast<std::uint8_t>(0),
                          const ByteVector& parameterData = std::vector<std::uint8_t>())
        :   m_targetOno(targetOno),
            m_propertyType(propertyType),
            m_propertyDefLevel(propertyDefLevel),
            m_propertyIndex(propertyIndex),
            m_paramCount(paramCount),
            m_parameterData(parameterData)
    {
    }

    /**
     * Struct destructor.
     */
    virtual ~Ocp1CommandDefinition() = default;

    /**
     * Generates a Ocp1CommandDefinition for a typical AddSubscription command.
     * Can be overriden for custom object AddSubscription commands.
     * 
     * @return An AddSubscription command definition.
     */
    virtual Ocp1CommandDefinition AddSubscriptionCommand() const;

    /**
     * Generates a Ocp1CommandDefinition for a typical Removeubscription command.
     * Can be overriden for custom object RemoveSubscription commands.
     *
     * @return A RemoveSubscription command definition.
     */
    virtual Ocp1CommandDefinition RemoveSubscriptionCommand() const;

    /**
     * Generates a Ocp1CommandDefinition for a typical GetValue command (methodIndex 1).
     * Can be overriden for custom object GetValue commands.
     * 
     * @return A GetValue command definition.
     */
    virtual Ocp1CommandDefinition GetValueCommand() const;

    /**
     * Generates a Ocp1CommandDefinition for a typical SetValue command (methodIndex 2).
     * Can be overriden for custom object SetValue commands.
     * 
     * @return A SetValue command definition.
     */
    virtual Ocp1CommandDefinition SetValueCommand(const Variant& newValue) const;

    /**
     * Clone this object. To prevent slicing, this method must be overriden whenever new members or methods
     * are added to a subclass. 
     * 
     * @return A pointer to a copy of this object. It is the caller's responsibility to worry about the object's ownership.
     */
    virtual Ocp1CommandDefinition* Clone() const;

    /**
     * Convenience getter method for the Ocp1CommandDefinition's type.
     *
     * @return the Ocp1CommandDefinition's type as a Ocp1DataType.
     */
    Ocp1DataType GetDataType() const
    {
        return static_cast<Ocp1DataType>(m_propertyType);
    }


    std::uint32_t m_targetOno;                  // Target ONo of the command.
    std::uint16_t m_propertyType;               // Property type of the command, as a Ocp1DataType.
    std::uint16_t m_propertyDefLevel;           // Level of the property definition within the AES70 class hierarchy.
    std::uint16_t m_propertyIndex;              // Index of the property within its AES70 class definition.
    std::uint8_t m_paramCount;                  // Number of parameters contained in m_parameterData.
    ByteVector m_parameterData;                 // Parameter data for the command.
};


/**
 * Representation of the header of a OCA message.
 */
class Ocp1Header
{
public:
    /**
     * Class constructor.
     */
    Ocp1Header(std::uint8_t msgType, std::size_t parameterDataLength)
        :   m_syncVal(0x3b),
            m_protoVers(static_cast<std::uint16_t>(1)),
            m_msgSize(CalculateMessageSize(msgType, parameterDataLength)),
            m_msgType(msgType),
            m_msgCnt(static_cast<std::uint16_t>(1))
    {
    }

    /**
     * Class constructor which creates a Ocp1Header based on a ByteVector.
     */
    explicit Ocp1Header(const ByteVector& memory);

    /**
     * Class destructor.
     */
    virtual ~Ocp1Header() = default;

    /**
     * Gets the type of the OCA message. (i.e. Notification, KeepAlive, etc).
     *
     * @return  Type of OCA message.
     */
    std::uint8_t GetMessageType() const
    {
        return m_msgType;
    }

    /**
     * Gets the size of the OCA message.
     *
     * @return  Size of OCA message, in byes.
     */
    std::uint32_t GetMessageSize() const
    {
        return m_msgSize;
    }

    /**
     * Checks if the header is valid.
     *
     * @return  True if the header's sync byte is correct, protoVers is 1, messageSize is
     *          large enough, messageType is valid, and messageCount at least 1.
     */
    bool IsValid() const;

    /**
     * Returns a vector of bytes representing the binary contents of the header.
     * 
     * @return  A vector of 10 bytes containing the OCA header.
     */
    ByteVector GetSerializedData() const;

    /**
     * Helper method to calculate the OCA message size based on the message's type and 
     * the number of parameter data bytes contained in the message.
     *
     * @param[in] msgType               Type of OCA message (i.e. Notification, KeepAlive, etc).
     * @param[in] parameterDataLength   Number of parameter data bytes contained in the message.
     * @return  Size of the complete OCA message in bytes, excluding the initial sync bit.
     */
    static std::uint32_t CalculateMessageSize(std::uint8_t msgType, size_t parameterDataLength);

    /**
     * Size of an OCA message header, in bytes, including the starting sync byte.
     */
    static constexpr std::uint32_t Ocp1HeaderSize = 10;

protected:
    std::uint8_t                m_syncVal;      // Always 0x3b
    std::uint16_t               m_protoVers;    // Always 1
    std::uint32_t               m_msgSize;      // Size of the complete OCA message in bytes, excluding the initial sync bit.
    std::uint8_t                m_msgType;      // Type of OCA message (i.e. Notification, KeepAlive, etc).
    std::uint16_t               m_msgCnt;       // Always 1
};


/**
 * @class Ocp1Message
 * @brief Abstract base class for all OCP.1 protocol messages.
 *
 * Every OCP.1 frame starts with a 10-byte `Ocp1Header` (sync byte 0x3b, protocol
 * version 1, message size, message type, message count) followed by type-specific
 * payload bytes.  `Ocp1Message` stores both and provides `GetSerializedData()` to
 * produce the complete binary frame for transmission.
 *
 * ## Message flow in an OCA session
 * ```
 * Client                                   Device
 *   │──Ocp1CommandResponseRequired(AddSub)──►│  subscribe to a property
 *   │◄──────────────Ocp1Response(OK)─────────│
 *   │──Ocp1CommandResponseRequired(GetValue)─►│  read current value
 *   │◄──────────────Ocp1Response(value)───────│
 *   │◄──────────────Ocp1Notification──────────│  value changed (unsolicited)
 *   │──Ocp1CommandResponseRequired(SetValue)─►│  write new value
 *   │◄──────────────Ocp1Response(OK)─────────│
 *   │──Ocp1KeepAlive──────────────────────────►│  heartbeat
 *   │◄──────────────Ocp1KeepAlive─────────────│
 * ```
 *
 * ## Receiving messages
 * `UnmarshalOcp1Message()` is the factory entry point.  Pass the raw bytes received
 * from the socket and it returns a typed `unique_ptr<Ocp1Message>` (or nullptr on
 * parse error).  Dispatch on `GetMessageType()`:
 * ```cpp
 * auto msg = Ocp1Message::UnmarshalOcp1Message(rawBytes);
 * if (!msg) return;
 * switch (msg->GetMessageType())
 * {
 *     case Ocp1Message::Notification:
 *     {
 *         auto* n = static_cast<Ocp1Notification*>(msg.get());
 *         // match n->GetEmitterOno() against subscription table
 *         break;
 *     }
 *     case Ocp1Message::Response:
 *     {
 *         auto* r = static_cast<Ocp1Response*>(msg.get());
 *         // match r->GetResponseHandle() against pending command handles
 *         break;
 *     }
 *     case Ocp1Message::KeepAlive: break; // no action needed
 *     default: break;
 * }
 * ```
 */
class Ocp1Message
{
public:
    /**
     * @brief OCP.1 message type codes as defined in AES70.
     *
     * | Value | Name | Direction | Description |
     * |---|---|---|---|
     * | 0 | Command | Client→Device | Fire-and-forget; no response expected. |
     * | 1 | CommandResponseRequired | Client→Device | Command that expects an `Ocp1Response` with a matching handle. |
     * | 2 | Notification | Device→Client | Unsolicited property-change event (requires prior `AddSubscription`). |
     * | 3 | Response | Device→Client | Reply to a `CommandResponseRequired`; carries status and return value. |
     * | 4 | KeepAlive | Both | Heartbeat for connection supervision; carries heartbeat interval. |
     */
    enum MessageType
    {
        Command = 0,                    ///< Fire-and-forget command; no response expected.
        CommandResponseRequired = 1,    ///< Command that expects a Response with a matching handle.
        Notification = 2,               ///< Unsolicited property change from device to client.
        Response = 3,                   ///< Device reply to a CommandResponseRequired.
        KeepAlive = 4                   ///< Heartbeat for connection supervision.
    };

    /**
     * Class constructor.
     */
    Ocp1Message(std::uint8_t msgType, const ByteVector& parameterData)
        : m_header(Ocp1Header(msgType, parameterData.size())),
        m_parameterData(parameterData)

    {
    }

    /**
     * Class destructor.
     */
    virtual ~Ocp1Message() = default;

    /**
     * Gets the type of the OCA message. (i.e. Notification, KeepAlive, etc).
     *
     * @return  Type of OCA message.
     */
    std::uint8_t GetMessageType() const
    {
        return m_header.GetMessageType();
    }

    /**
     * Returns a vector of bytes representing the parameter data contained in the message.
     *
     * @return  A vector containing the OCA message including header.
     */
    ByteVector GetParameterData() const
    {
        return m_parameterData;
    }

    /**
     * Returns a vector of bytes representing the binary contents of the complete message.
     * Must be reimplemented for each message type.
     *
     * @return  A vector containing the OCA message including header.
     */
    virtual ByteVector GetSerializedData() = 0;


    /**
     * Factory method which creates a new Ocp1Message object based on a vector<std::uint8_t>.
     *
     * @param[in] receivedData    Vector containing the received OCA message.
     * @return  A unique pointer to the unmarshaled Ocp1Message object.
     */
    static std::unique_ptr<Ocp1Message> UnmarshalOcp1Message(const ByteVector& receivedData);


protected:
    Ocp1Header                  m_header;           // OCA message header.
    ByteVector   m_parameterData;                   // Parameter data contained by the message.
    static std::uint32_t        m_nextHandle;       // Static variable to generate unique command handles.
};


/**
 * Representation of an OCA CommandResponseRequired message.
 */
class Ocp1CommandResponseRequired : public Ocp1Message
{
public:
    /**
     * Class constructor without creating the handle.
     * To set the handle of this command, use SetHandle() after instantiation.
     */
    Ocp1CommandResponseRequired(std::uint32_t targetOno,
                                std::uint16_t methodDefLevel,
                                std::uint16_t methodIndex,
                                std::uint8_t paramCount,
                                const ByteVector& parameterData)
        : Ocp1Message(static_cast<std::uint8_t>(CommandResponseRequired), parameterData),
            m_handle(0),
            m_targetOno(targetOno),
            m_methodDefLevel(methodDefLevel),
            m_methodIndex(methodIndex),
            m_paramCount(paramCount)
    {
    }

    /**
     * Class constructor.
     */
    Ocp1CommandResponseRequired(std::uint32_t targetOno,
                                std::uint16_t methodDefLevel,
                                std::uint16_t methodIndex,
                                std::uint8_t paramCount,
                                const ByteVector& parameterData,
                                std::uint32_t& handle)
        : Ocp1CommandResponseRequired(targetOno, methodDefLevel, methodIndex,
                                      paramCount, parameterData)
    {
        // Return a new unique handle every time this class is instantiated.
        m_handle = m_nextHandle;
        handle = m_handle;
        m_nextHandle++;
    }

    /**
     * Class constructor that takes parameters via a Ocp1CommandDefinition struct.
     */
    Ocp1CommandResponseRequired(const Ocp1CommandDefinition& def,
                                std::uint32_t& handle)
        : Ocp1CommandResponseRequired(def.m_targetOno, def.m_propertyDefLevel, def.m_propertyIndex,
                                      def.m_paramCount, def.m_parameterData, handle)
    {
    }

    /**
     * Class destructor.
     */
    ~Ocp1CommandResponseRequired() override = default;

    /**
     * Override the automatically assigned command handle with a manually defined one.
     * 
     * @param[in] handle    New command handle to use.
     */
    void SetHandle(std::uint32_t handle)
    {
        m_handle = handle;
    }

    std::uint32_t GetHandle() const
    {
        return m_handle;
    }

    std::uint32_t GetTargetOno() const
    {
        return m_targetOno;
    }

    std::uint16_t GetMethodDefLevel() const
    {
        return m_methodDefLevel;
    }

    std::uint16_t GetMethodIndex() const
    {
        return m_methodIndex;
    }
    
    // Reimplemented from Ocp1Message

    ByteVector GetSerializedData() override;

protected:
    std::uint32_t               m_handle;           // Handle of the command.
    std::uint32_t               m_targetOno;        // Target ONo of the command.
    std::uint16_t               m_methodDefLevel;   // Level of the method definition within the AES70 class hierarchy.
    std::uint16_t               m_methodIndex;      // Index of the method within its AES70 class definition.
    std::uint8_t                m_paramCount;       // Number of parameters contained in the command.
};


/**
 * Representation of an Oca Response message.
 */
class Ocp1Response : public Ocp1Message
{
public:
    /**
     * Class constructor.
     */
    Ocp1Response(std::uint32_t handle,
                 std::uint8_t status,
                 std::uint8_t paramCount,
                 const ByteVector& parameterData)
        : Ocp1Message(static_cast<std::uint8_t>(Response), parameterData),
            m_handle(handle),
            m_status(status),
            m_paramCount(paramCount)
    {
    }

    /**
     * Class destructor.
     */
    ~Ocp1Response() override = default;

    /**
     * Gets the handle of the OCA response.
     *
     * @return  Handle of OCA response.
     */
    std::uint32_t GetResponseHandle() const
    {
        return m_handle;
    }

    /**
     * Gets the status of the OCA response. Use StatusToString for its string representation.
     *
     * @return  Status of the OCA response.
     */
    std::uint8_t GetResponseStatus() const
    {
        return m_status;
    }

    /**
     * Gets the number of parameters contained in this response. Status doesn't count as a parameter.
     *
     * @return  Number of parameters contained in this response. 
     */
    std::uint8_t GetParamCount() const
    {
        return m_paramCount;
    }

    // Reimplemented from Ocp1Message

    ByteVector GetSerializedData() override;

protected:
    /**
     * Handle of the response. Should match the handle of a previously sent command.
     */
    std::uint32_t               m_handle;

    /**
     * Indicates whether the previously sent command was successful.
     */
    std::uint8_t                m_status;

    /**
     * Number of parameters contained in this response. Status doesn't count as a parameter.
     */
    std::uint8_t                m_paramCount;
};


/**
 * Representation of an Oca Notification message.
 */
class Ocp1Notification : public Ocp1Message
{
public:
    /**
     * Class constructor.
     */
    Ocp1Notification(std::uint32_t emitterOno,
                     std::uint16_t emitterPropertyDefLevel,
                     std::uint16_t emitterPropertyIndex,
                     std::uint8_t paramCount,
                     const ByteVector& parameterData)
        : Ocp1Message(static_cast<std::uint8_t>(Notification), parameterData),
            m_emitterOno(emitterOno),
            m_emitterPropertyDefLevel(emitterPropertyDefLevel),
            m_emitterPropertyIndex(emitterPropertyIndex),
            m_paramCount(paramCount)
    {
    }

    /**
     * Get the ONo of the object whose property changed, triggering this notification.
     * 
     * @return  The emitter object's ONo.
     */
    std::uint32_t GetEmitterOno() const
    {
        return m_emitterOno;
    }

    /**
     * Class destructor.
     */
    ~Ocp1Notification() override = default;

    /**
     * Gets the number of parameters contained in this Notification.
     *
     * @return  Number of parameters contained in this Notification.
     */
    std::uint8_t GetParamCount() const
    {
        return m_paramCount;
    }

    /**
     * Helper method which matches this notification to a given object definition.
     * 
     * @param[in] def   Object definition to match against.
     * @return  True if this notification was triggered by the given object.
     */
    bool MatchesObject(const Ocp1CommandDefinition* def) const
    {
        return ((def->m_targetOno == m_emitterOno) && 
                (def->m_propertyDefLevel == m_emitterPropertyDefLevel) &&
                (def->m_propertyIndex == m_emitterPropertyIndex));
    }

    // Reimplemented from Ocp1Message

    ByteVector GetSerializedData() override;

protected:
    std::uint32_t               m_emitterOno;               // ONo of the object whose property changed, triggering this notification.
    std::uint16_t               m_emitterPropertyDefLevel;  // Level of the property definition within the AES70 class hierarchy.
    std::uint16_t               m_emitterPropertyIndex;     // Index of the property within its AES70 class definition.

    /**
     * Number of parameters contained in this Notification.
     */
    std::uint8_t                m_paramCount;
};


/**
 * Representation of an Oca KeepAlive message. 
 */
class Ocp1KeepAlive : public Ocp1Message
{
public:
    /**
     * Class constructor for initialization with a 16bit seconds value.
     */
    Ocp1KeepAlive(std::uint16_t heartBeatSeconds);
    
    /**
     * Class constructor for initialization with a 32bit milliseconds value.
     */
    Ocp1KeepAlive(std::uint32_t heartBeatMilliseconds);

    /**
     * Class destructor.
     */
    ~Ocp1KeepAlive() override = default;

    /**
     * Get this KeepAlive message's heartbeat time.
     * @return This KeepAlive message's heartbeat time in seconds or 0 if 32bit milliseconds are used.
     */
    std::uint16_t GetHeartBeatSeconds() const;
    
    /**
     * Get this KeepAlive message's heartbeat time.
     * @return This KeepAlive message's heartbeat time in milliseconds or 0 if 16bit seconds are used.
     */
    std::uint32_t GetHeartBeatMilliseconds() const;


    // Reimplemented from Ocp1Message

    ByteVector GetSerializedData() override;
};

}
