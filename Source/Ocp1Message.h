/*
  ==============================================================================

    Ocp1Message.h
    Created: 31 Mar 2023 4:50:02pm
    Author:  bernardoe

  ==============================================================================
*/

#pragma once

#include <JuceHeader.h>


namespace NanoOcp1
{

/**
 * Convinience helper method to convert a byte vector into a Uint8
 * 
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint8.
 */
std::uint8_t DataToUint8(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convinience helper method to convert a Uint8 into a byte vector
 * 
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint8(std::uint8_t value);

/**
 * Convinience helper method to convert a byte vector into a Uint16
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint16.
 */
std::uint16_t DataToUint16(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convinience helper method to convert a Uint16 into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint16(std::uint16_t value);

/**
 * Convinience helper method to convert a byte vector into a Uint32
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a Uint32.
 */
std::uint32_t DataToUint32(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convinience helper method to convert a Uint32 into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromUint32(std::uint32_t value);

/**
 * Convinience helper method to convert a byte vector into a juce::String
 *
 * @param[in] parameterData     Vector of bytes containing the string to be converted.
 *                              Note that the first two bytes contain the string's length.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The string contained in the parameterData as a juce::String.
 */
juce::String DataToString(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convinience helper method to convert a juce::String into a byte vector
 *
 * @param[in] string     juce::String to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromString(const juce::String& string);

/**
 * Convinience helper method to convert a byte vector into a 32-bit float.
 *
 * @param[in] parameterData     Vector of bytes containing the value to be converted.
 * @param[in] ok                Optional parameter to verify if the conversion was successful.
 * @return  The value contained in the parameterData as a float.
 */
std::float_t DataToFloat(const std::vector<std::uint8_t>& parameterData, bool* ok = nullptr);

/**
 * Convinience helper method to convert a 32-bit float into a byte vector
 *
 * @param[in] value     Value to be converted.
 * @return  The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromFloat(std::float_t value);



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
            m_msgType(msgType),
            m_msgCnt(static_cast<std::uint16_t>(1)),
            m_msgSize(CalculateMessageSize(msgType, parameterDataLength))
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
        :   m_parameterData(parameterData),
            m_header(Ocp1Header(msgType, parameterData.size()))
    {
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
    static Ocp1Message* UnmarshalOcp1Message(const juce::MemoryBlock& receivedData);

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
                                const std::vector<std::uint8_t>& parameterData,
                                std::uint32_t& handle)
        : Ocp1Message(static_cast<std::uint8_t>(CommandResponseRequired), parameterData),
            m_targetOno(targetOno),
            m_methodDefLevel(methodDefLevel),
            m_methodIndex(methodIndex)
    {
        // Return a new unique handle every time this class is instantiated.
        m_handle = m_nextHandle;
        handle = m_handle;
        m_nextHandle++;
    }

    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData() override;

protected:
    std::uint32_t               m_handle;           // TODO
    std::uint32_t               m_targetOno;
    std::uint16_t               m_methodDefLevel;
    std::uint16_t               m_methodIndex;
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
                     const std::vector<std::uint8_t>& parameterData)
        : Ocp1Message(static_cast<std::uint8_t>(Notification), parameterData),
            m_emitterOno(emitterOno),
            m_emitterPropertyDefLevel(emitterPropertyDefLevel),
            m_emitterPropertyIndex(emitterPropertyIndex)
    {
    }

    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData();

protected:
    std::uint32_t               m_emitterOno;               // TODO
    std::uint16_t               m_emitterPropertyDefLevel;
    std::uint16_t               m_emitterPropertyIndex;
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

    // Reimplemented from Ocp1Message

    std::vector<std::uint8_t> GetSerializedData();

protected:
    std::uint16_t               m_heartBeat;    // Heartbeat time in seconds, typically 5
};

}