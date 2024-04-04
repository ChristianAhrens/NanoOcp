/* Copyright (c) 2024, Bernardo Escalona
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
 * Enumeration that describes all available base data types. 
 * Same values as OcaBaseDataType in OcaBaseDataTypes.h
 * of the Bosch reference implementation.
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
 * @brief  Convenience helper method to convert a byte vector into a bool
 * @param  parameterData Vector of bytes containing the value to be converted.
 * @param  pOk           Optional parameter to verify if the conversion was successful.
 * @return               The value contained in the parameterData as bool.
 */
bool DataToBool(const std::vector<std::uint8_t>& parameterData, bool* pOk);

/**
 * @brief  Convenience helper method to convert a bool into a byte vector
 * @param  boolValue Value to be converted.
 * @return           The value as a byte vector.
 */
std::vector<std::uint8_t> DataFromBool(bool boolValue);

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
 * necessary for an AddSubscription or RemoveSubscription command for a given object.
 *
 * @param[in] ono     ONo of the object that the subscription shall be added or removed for.
 * @param[in] add     True to generate a AddSubscription command. False to generate a RemoveSubscription command.
 * @return  The parameters as a byte vector.
 */
std::vector<std::uint8_t> DataFromOnoForSubscription(std::uint32_t ono, bool add = true);

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

}
