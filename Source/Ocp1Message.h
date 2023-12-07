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

#pragma once

#include <JuceHeader.h>


namespace NanoOcp1
{

/**
 * Convenience helper method to convert a byte vector into a Int32
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Int32.
 */
std::int32_t DataToInt32(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a Int32 into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromInt32(std::int32_t value);

/**
 * Convenience helper method to convert a byte vector into a Uint8
 * 
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint8.
 */
std::uint8_t DataToUint8(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a Uint8 into a byte vector
 * 
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint8(std::uint8_t value);

/**
 * Convenience helper method to convert a byte vector into a Uint16
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint16.
 */
std::uint16_t DataToUint16(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a Uint16 into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint16(std::uint16_t value);

/**
 * Convenience helper method to convert a byte vector into a Uint32
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint32.
 */
std::uint32_t DataToUint32(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a Uint32 into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint32(std::uint32_t value);

/**
 * Convenience helper method to convert a byte vector into a Uint64
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint64.
 */
std::uint64_t DataToUint64(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a byte vector into a juce::String
 *
 * @param[in] parameterData     Vector of bytes containing the string to be converted.
 *                              Note that the first two bytes contain the string's length.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The string contained in the parameterData as a juce::String.
 */
juce::String DataToString(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a juce::String into a byte vector
 *
 * @param[in] string     juce::String to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromString(const juce::String& string);

/**
 * Convenience helper method to convert a byte vector into a 32-bit float.
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a float.
 */
std::float_t DataToFloat(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convenience helper method to convert a 32-bit float into a byte vector
 *
 * @param[in] floatValue     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromFloat(std::float_t floatValue);

/**
 * Convenience helper method to convert a 3D position (three 32-bit floats) into a byte vector
 *
 * @param[in] x     First value to be converted.
 * @param[in] y     Second value to be converted.
 * @param[in] z     Third value to be converted.
 * @return  The values as a byte vector.
 */
std::vector<std::uint8_t> DataFromPosition(std::float_t x, std::float_t y, std::float_t z);

/**
 * Convenience helper method to convert a 3D position and rotation (six 32-bit floats) into a byte vector
 *
 * @param[in] x     First value to be converted.
 * @param[in] y     Second value to be converted.
 * @param[in] z     Third value to be converted.
 * @param[in] hor   Fourth value to be converted.
 * @param[in] vert  Fifth value to be converted.
 * @param[in] rot   Sixth value to be converted.
 * @return  The values as a byte vector.
 */
std::vector<std::uint8_t> DataFromPositionAndRotation(std::float_t x, std::float_t y, std::float_t z, std::float_t hor, std::float_t vert, std::float_t rot);

/**
 * Convenience helper method to generate a byte vector containing the parameters
 * necessary for an AddSubscription command for a given object.
 *
 * @param[in] ono     ONo of the object that the subscription shall be added for.
 * @return  The parameters as a byte vector.
 */
std::vector<std::uint8_t> DataFromOnoForSubscription(std::uint32_t ono);

/**
 * Convenience helper method to extract x, y, and z float values from a juce::var.
 *
 * @param[in] value The juce::var containing the values as 3 x 4 bytes.
 * @param[out] x    The contained x value.
 * @param[out] y    The contained y value.
 * @param[out] z    The contained z value.
 * @return  True if the conversion was successful. 
 */
bool VariantToPosition(const juce::var& value, std::float_t& x, std::float_t& y, std::float_t& z);

/**
 * Convenience helper method to extract x, y, z, horizontal angle, vertical angle and rotation angle float values from a juce::var.
 *
 * @param[in] value The juce::var containing the values as 6 x 4 bytes.
 * @param[out] x    The contained x value.
 * @param[out] y    The contained y value.
 * @param[out] z    The contained z value.
 * @param[out] hor  The contained horizontal angle value.
 * @param[out] vert The contained vertical angle value.
 * @param[out] rot  The contained rotation angle value.
 * @return  True if the conversion was successful.
 */
bool VariantToPositionAndRotation(const juce::var& value, std::float_t& x, std::float_t& y, std::float_t& z, std::float_t& hor, std::float_t& vert, std::float_t& rot);

/**
 * Convenience helper method to exract a std::vector<bool> from a from a juce::var.
 * The juce::var needs to be of type MemoryBlock, and the contents need to be marshalled as an OcaList<OcaBoolean>.
 *
 * @param[in] value         The juce::var containing a list of boolean values.
 * @param[out] boolVector   The resulting std::vector<bool>. 
 * @return  True if the conversion was successful.
 */
bool VariantToBoolVector(const juce::var& value, std::vector<bool>& boolVector);

/**
 * Convenience helper method to exract a juce::StringArray from a from a juce::var.
 * The juce::var needs to be of type MemoryBlock, and the contents need to be marshalled as an OcaList<OcaString>.
 *
 * @param[in] value         The juce::var containing a list of strings.
 * @param[out] stringArray  The resulting juce::StringArray.
 * @return  True if the conversion was successful.
 */
bool VariantToStringArray(const juce::var& value, juce::StringArray& stringArray);

/**
 * Convenience method to convert an integer representing an OcaStatus to its string representation.
 *
 * @param[in] status     Integer representing an OcaStatus.
 * @return  The string representation of the OcaStatus.
 */
juce::String StatusToString(std::uint8_t status);

/**
 * Convenience method to convert an integer representing an Ocp1DataType to its string representation.
 *
 * @param[in] dataType  Integer representing an Ocp1DataType.
 * @return  The string representation of the Ocp1DataType.
 */
juce::String DataTypeToString(int dataType);

/**
 * Convenience method to convert an integer representing an OCA Response handle to its string representation.
 * It will return juce::String(handle) most of the time, except in cases OCA_INVALID_SESSIONID and OCA_LOCAL_SESSIONID.
 *
 * @param[in] handle     OCA Response handle.
 * @return  The string representation of the handle.
 */
juce::String HandleToString(std::uint32_t handle);

/**
 * Convenience method to read 4 bytes from a buffer.
 *
 * @param[in] buffer     Pointer to the start of the data to be read.
 * @return  Resulting 4 bytes as an uint32_t.
 */
std::uint32_t ReadUint32(const char* buffer);

/**
 * Convenience method to read 2 bytes from a buffer.
 *
 * @param[in] buffer     Pointer to the start of the data to be read.
 * @return  Resulting 2 bytes as an uint16_t.
 */
std::uint16_t ReadUint16(const char* buffer);

/**
 * Convenience method to generate a unique target object number.
 * This is the method to use when addressing regular amp objects.
 *
 * @param[in] type                  The object type.
 * @param[in] record                The object record number value.
 * @param[in] channel               The object channel number value.
 * @param[in] boxAndObjectNumber    The aggregate box and target number.
 * @return  The object ONo.
 */
std::uint32_t GetONo(std::uint32_t type, std::uint32_t record, std::uint32_t channel, std::uint32_t boxAndObjectNumber);

/**
 * Convenience method to generate a unique target object number.
 * This is the method to use when addressing newer Matrix type object (to be confirmed).
 *
 * @param[in] type                  The object type.
 * @param[in] record                The object record number value.
 * @param[in] channel               The object channel number value.
 * @param[in] boxNumber             The objects box number.
 * @param[in] objectNumber          The objects target number.
 * @return  The object ONo.
 */
std::uint32_t GetONoTy2(std::uint32_t type, std::uint32_t record, std::uint32_t channel, std::uint32_t boxNumber, std::uint32_t objectNumber);


/** 
 * Enumeration that describes all available base data types. 
 * Same values as OcaBaseDataType in OcaBaseDataTypes.h
 */
enum Ocp1DataType
{
    OCP1DATATYPE_NONE               = 0,
    OCP1DATATYPE_BOOLEAN            = 1,
    OCP1DATATYPE_INT8               = 2,
    OCP1DATATYPE_INT16              = 3,
    OCP1DATATYPE_INT32              = 4,
    OCP1DATATYPE_INT64              = 5,
    OCP1DATATYPE_UINT8              = 6,
    OCP1DATATYPE_UINT16             = 7,
    OCP1DATATYPE_UINT32             = 8,
    OCP1DATATYPE_UINT64             = 9,
    OCP1DATATYPE_FLOAT32            = 10,
    OCP1DATATYPE_FLOAT64            = 11,
    OCP1DATATYPE_STRING             = 12,
    OCP1DATATYPE_BIT_STRING         = 13,
    OCP1DATATYPE_BLOB               = 14,
    OCP1DATATYPE_BLOB_FIXED_LEN     = 15,
    OCP1DATATYPE_DB_POSITION        = 32,   // Type used by CdbOcaPositionAgentDeprecated
    OCP1DATATYPE_CUSTOM             = 128   // User-defined types
};


/**
 * Helper struct to encapsulate parameters for OCA Commands, Responses and Notifications.
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
                          const std::vector<std::uint8_t>& parameterData = std::vector<std::uint8_t>())
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
    virtual ~Ocp1CommandDefinition()
    {
    }

    /**
     * Generates a Ocp1CommandDefinition for a typical AddSubscription command.
     * Can be overriden for custom object AddSubscription commands.
     * 
     * @return An AddSubscription command definition.
     */
    virtual Ocp1CommandDefinition AddSubscriptionCommand() const;

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
    virtual Ocp1CommandDefinition SetValueCommand(const juce::var& newValue) const;

    /**
     * Convert the parameter data obtained by i.e. an OCA Notification message to the correct
     * data type depending on this Ocp1CommandDefinition's type.
     * The resulting value will be returned as a variant juce::var.
     * For example, a command definition for an OcaGain object will convert the provided byte array to a float value.
     * 
     * @param[in] paramCount    Number of parameters inside parameterData. Usually 1.
     * @param[in] parameterData Byte array provided by an OCA Notification or Response message.
     * @return    The value contained in parameterData, converted to the type corresponding to 
     *            this Ocp1CommandDefinition, and packed into a juce::var.
     */
    virtual juce::var ToVariant(std::uint8_t paramCount, const std::vector<std::uint8_t>& parameterData);

    /**
     * Clone this object. To prevent slicing, this method must be overriden whenever new members or methods
     * are added to a subclass. 
     * 
     * @return A pointer to a copy of this object. It is the caller's responsibility to worry about the object's ownership.
     */
    virtual Ocp1CommandDefinition* Clone() const;


    std::uint32_t m_targetOno;
    std::uint16_t m_propertyType;
    std::uint16_t m_propertyDefLevel;
    std::uint16_t m_propertyIndex;
    std::uint8_t m_paramCount;
    std::vector<std::uint8_t> m_parameterData;
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
    Ocp1Header(std::uint8_t msgType, size_t parameterDataLength)
        :   m_syncVal(0x3b),
            m_protoVers(static_cast<std::uint16_t>(1)),
            m_msgSize(CalculateMessageSize(msgType, parameterDataLength)),
            m_msgType(msgType),
            m_msgCnt(static_cast<std::uint16_t>(1))
    {
    }

    /**
     * Class constructor which creates a Ocp1Header based on a juce::MemoryBlock.
     */
    Ocp1Header(const juce::MemoryBlock& memoryBlock);

    /**
     * Class destructor.
     */
    virtual ~Ocp1Header()
    {
    }

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
    std::vector<std::uint8_t> GetSerializedData() const;

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
 * Abstract representation of a general OCA Message.
 */
class Ocp1Message
{
public:
    /**
     * Enumeration of message types.
     */
    enum MessageType
    {
        Command = 0,                    // Command - No response required. 
        CommandResponseRequired = 1,    // Command - Response required.
        Notification = 2,               // Notification.
        Response = 3,                   // Response (to a command).
        KeepAlive = 4                   // KeepAlive message used for device supervision. 
    };

    /**
     * Class constructor.
     */
    Ocp1Message(std::uint8_t msgType, const std::vector<std::uint8_t>& parameterData)
        : m_header(Ocp1Header(msgType, parameterData.size())),
        m_parameterData(parameterData)

    {
    }

    /**
     * Class destructor.
     */
    virtual ~Ocp1Message()
    {
    }

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
    std::vector<std::uint8_t> GetParameterData() const
    {
        return m_parameterData;
    }

    /**
     * Returns a vector of bytes representing the binary contents of the complete message.
     * Must be reimplemented for each message type.
     *
     * @return  A vector containing the OCA message including header.
     */
    virtual std::vector<std::uint8_t> GetSerializedData() = 0;

    /**
     * Convenience method which returns a juce::MemoryBloc representing 
     * the binary contents of the complete message.
     *
     * @return  A juce::MemoryBlock containing the OCA message including header.
     */
    juce::MemoryBlock GetMemoryBlock()
    {
        auto serializedData = GetSerializedData();
        return juce::MemoryBlock((const char*)serializedData.data(), serializedData.size());
    }

    /**
     * Factory method which creates a new Ocp1Message object based on a MemoryBlock.
     * 
     * TODO
     */
    static std::unique_ptr<Ocp1Message> UnmarshalOcp1Message(const juce::MemoryBlock& receivedData);

protected:
    Ocp1Header                  m_header;           // TODO
    std::vector<std::uint8_t>   m_parameterData;
    static std::uint32_t        m_nextHandle;
};


/**
 * Representation of an OCA CommandResponseRequired message.
 */
class Ocp1CommandResponseRequired : public Ocp1Message
{
public:
    /**
     * Class constructor.
     */
    Ocp1CommandResponseRequired(std::uint32_t targetOno,
                                std::uint16_t methodDefLevel,
                                std::uint16_t methodIndex,
                                std::uint8_t paramCount,
                                const std::vector<std::uint8_t>& parameterData,
                                std::uint32_t& handle)
        : Ocp1Message(static_cast<std::uint8_t>(CommandResponseRequired), parameterData),
            m_targetOno(targetOno),
            m_methodDefLevel(methodDefLevel),
            m_methodIndex(methodIndex),
            m_paramCount(paramCount)
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
    ~Ocp1CommandResponseRequired() override
    {
    }

    /**
     * Override the automatically assigned command handle with a manually defined one.
     * 
     * @param[in] handle    New command handle to use.
     */
    void SetHandle(std::uint32_t handle)
    {
        m_handle = handle;
    }

    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData() override;

protected:
    std::uint32_t               m_handle;           // TODO
    std::uint32_t               m_targetOno;
    std::uint16_t               m_methodDefLevel;
    std::uint16_t               m_methodIndex;
    std::uint8_t                m_paramCount;
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
                 const std::vector<std::uint8_t>& parameterData)
        : Ocp1Message(static_cast<std::uint8_t>(Response), parameterData),
            m_handle(handle),
            m_status(status),
            m_paramCount(paramCount)
    {
    }

    /**
     * Class destructor.
     */
    ~Ocp1Response() override
    {
    }

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

    std::vector<std::uint8_t> GetSerializedData() override;

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
                     const std::vector<std::uint8_t>& parameterData)
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
    ~Ocp1Notification() override
    {
    }

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
     * @param[in] TODO
     * @return  True if this notification was triggered by the given object.
     */
    bool MatchesObject(const Ocp1CommandDefinition* def) const
    {
        return ((def->m_targetOno == m_emitterOno) && 
                (def->m_propertyDefLevel == m_emitterPropertyDefLevel) &&
                (def->m_propertyIndex == m_emitterPropertyIndex));
    }

    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData() override;

protected:
    std::uint32_t               m_emitterOno;               // TODO
    std::uint16_t               m_emitterPropertyDefLevel;
    std::uint16_t               m_emitterPropertyIndex;

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
     * Class constructor.
     */
    Ocp1KeepAlive(std::uint16_t heartBeat)
        : Ocp1Message(static_cast<std::uint8_t>(KeepAlive), std::vector<std::uint8_t>()),
            m_heartBeat(heartBeat)
    {
    }

    /**
     * Class destructor.
     */
    ~Ocp1KeepAlive() override
    {
    }

    /**
     * Get this KeepAlive message's heartbeat time.
     * @return This KeepAlive message's heartbeat time in seconds.
     */
    std::uint16_t GetHeartBeat() const
    {
        return m_heartBeat;
    }


    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData() override;

protected:
    std::uint16_t               m_heartBeat;    // Heartbeat time in seconds, typically 5
};

}
