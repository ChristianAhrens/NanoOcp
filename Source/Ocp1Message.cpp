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

std::int32_t DataToInt32(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::int32_t ret(0);

    // HACK: Use >= and not == to easily deal with responses sometimes including min and max values.
    bool ok = (parameterData.size() >= sizeof(std::int32_t)); // 4 bytes expected.
    if (ok)
    {
        ret = (((parameterData[0] << 24) & 0xff000000) +
               ((parameterData[1] << 16) & 0x00ff0000) +
               ((parameterData[2] << 8)  & 0x0000ff00) + parameterData[3]);
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromInt32(std::int32_t intValue)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(4);

    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    return ret;
}

std::uint8_t DataToUint8(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::uint8_t ret(0);
    bool ok = (parameterData.size() >= sizeof(std::uint8_t));
    if (ok)
    {
        ret = parameterData[0];
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromUint8(std::uint8_t value)
{
    std::vector<std::uint8_t> ret;
    ret.push_back(value);

    return ret;
}


std::uint16_t DataToUint16(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::uint16_t ret(0);
    bool ok = (parameterData.size() >= sizeof(std::uint16_t));
    if (ok)
    {
        for (size_t i = 0; i < sizeof(std::uint16_t); i++)
        {
            ret = ((ret << 8) & 0xff00) + parameterData[i];
        }
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromUint16(std::uint16_t value)
{
    std::vector<std::uint8_t> ret;
    ret.push_back(static_cast<std::uint8_t>(value >> 8));
    ret.push_back(static_cast<std::uint8_t>(value));

    return ret;
}

std::uint32_t DataToUint32(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::uint32_t ret(0);

    bool ok = (parameterData.size() >= sizeof(std::uint32_t)); // 4 bytes expected.
    if (ok)
    {
        ret = (((parameterData[0] << 24) & 0xff000000) +
               ((parameterData[1] << 16) & 0x00ff0000) +
               ((parameterData[2] << 8)  & 0x0000ff00) + parameterData[3]);
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromUint32(std::uint32_t intValue)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(4);

    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    return ret;
}

std::uint64_t DataToUint64(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::uint64_t ret(0);

    bool ok = (parameterData.size() >= sizeof(std::uint64_t)); // 8 bytes expected.
    if (ok)
    {
        std::uint64_t tmp;
        memcpy(&tmp, parameterData.data(), sizeof(juce::uint64));
        ret = juce::ByteOrder::swapIfLittleEndian<juce::uint64>(tmp);
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

juce::String DataToString(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    juce::String ret(0);

    bool ok = parameterData.size() >= 2; // At least 2 bytes for the string length
    if (ok)
    {
        ret.preallocateBytes(parameterData.size() - 1);
        ret = juce::String(std::string(parameterData.begin() + 2, parameterData.end()));
    }
    
    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromString(const juce::String& string)
{
    std::vector<std::uint8_t> ret;

    const char* pStringData = string.toRawUTF8();
    int stringLength = string.length();
    
    ret.reserve(stringLength + 2);
    ret.push_back(static_cast<std::uint8_t>(stringLength >> 8));
    ret.push_back(static_cast<std::uint8_t>(stringLength));
    for (int i = 0; i < stringLength; i++)
    {
        ret.push_back(pStringData[i]);
    }

    return ret;
}

std::float_t DataToFloat(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::float_t ret(0);

    bool ok = (parameterData.size() >= sizeof(std::float_t)); // 4 bytes expected.
    ok = ok && (sizeof(int) == sizeof(std::float_t)); // Required for pointer cast to work
    if (ok)
    {
        int intValue = (((parameterData[0] << 24) & 0xff000000) + 
                        ((parameterData[1] << 16) & 0x00ff0000) + 
                        ((parameterData[2] << 8)  & 0x0000ff00) + parameterData[3]);
        ret = *(std::float_t*)&intValue;
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromFloat(std::float_t floatValue)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(4);

    jassert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast to work
    std::uint32_t intValue = *(std::uint32_t*)&floatValue;

    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    return ret;
}

std::vector<std::uint8_t> DataFromPosition(std::float_t x, std::float_t y, std::float_t z)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(3 * 4);

    jassert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast to work
    
    std::uint32_t intValue = *(std::uint32_t*)&x;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&y;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&z;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    return ret;
}

std::vector<std::uint8_t> DataFromPositionAndRotation(std::float_t x, std::float_t y, std::float_t z, std::float_t hor, std::float_t vert, std::float_t rot)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(6 * 4);

    jassert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast to work

    std::uint32_t intValue = *(std::uint32_t*)&x;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&y;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&z;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&hor;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&vert;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    intValue = *(std::uint32_t*)&rot;
    ret.push_back(static_cast<std::uint8_t>(intValue >> 24));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 16));
    ret.push_back(static_cast<std::uint8_t>(intValue >> 8));
    ret.push_back(static_cast<std::uint8_t>(intValue));

    return ret;
}

std::vector<std::uint8_t> DataFromOnoForSubscription(std::uint32_t ono)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(25);

    ret.push_back(static_cast<std::uint8_t>(ono >> 24)); // Emitter ONo
    ret.push_back(static_cast<std::uint8_t>(ono >> 16));
    ret.push_back(static_cast<std::uint8_t>(ono >> 8));
    ret.push_back(static_cast<std::uint8_t>(ono));
    ret.push_back(static_cast<std::uint8_t>(0x00)); // EventID def level: OcaRoot
    ret.push_back(static_cast<std::uint8_t>(0x01)); 
    ret.push_back(static_cast<std::uint8_t>(0x00)); // EventID idx: PropertyChanged
    ret.push_back(static_cast<std::uint8_t>(0x01)); 

    ret.push_back(static_cast<std::uint8_t>(ono >> 24)); // Subscriber ONo
    ret.push_back(static_cast<std::uint8_t>(ono >> 16));
    ret.push_back(static_cast<std::uint8_t>(ono >> 8));
    ret.push_back(static_cast<std::uint8_t>(ono));
    ret.push_back(static_cast<std::uint8_t>(0x00)); // Method def level: OcaSubscriptionManager
    ret.push_back(static_cast<std::uint8_t>(0x03)); 
    ret.push_back(static_cast<std::uint8_t>(0x00)); // Method idx: AddSubscription
    ret.push_back(static_cast<std::uint8_t>(0x01)); 

    ret.push_back(static_cast<std::uint8_t>(0x00)); // Context size: 0
    ret.push_back(static_cast<std::uint8_t>(0x00));
    ret.push_back(static_cast<std::uint8_t>(0x01)); // Delivery mode: Reliable

    ret.push_back(static_cast<std::uint8_t>(0x00)); // Destination info length: always 4
    ret.push_back(static_cast<std::uint8_t>(0x04));
    ret.push_back(static_cast<std::uint8_t>(0x00)); // Destination info (4 empty bytes)
    ret.push_back(static_cast<std::uint8_t>(0x00));
    ret.push_back(static_cast<std::uint8_t>(0x00));
    ret.push_back(static_cast<std::uint8_t>(0x00));

    return ret;
}

bool VariantToPosition(const juce::var& value, std::float_t& x, std::float_t& y, std::float_t& z)
{
    MemoryBlock* mb = value.getBinaryData();
    bool ok = (mb->getSize() == 12  // Value contains 3 floats: x, y, z.
        || mb->getSize() == 36); // Value contains 9 floats: x, y, z and min and max each on top.
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 0; i < 4; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        x = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 4; i < 8; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        y = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 8; i < 12; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        z = NanoOcp1::DataToFloat(paramData, &ok);
    }

    return ok;
}

bool VariantToPositionAndRotation(const juce::var& value, std::float_t& x, std::float_t& y, std::float_t& z, std::float_t& hor, std::float_t& vert, std::float_t& rot)
{
    MemoryBlock* mb = value.getBinaryData();
    bool ok = (mb->getSize() == 24); // Value contains 6 floats: x, y, z, horAngle, vertAngle, rotAngle.
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 0; i < 4; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        x = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 4; i < 8; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        y = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 8; i < 12; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        z = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 12; i < 16; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        hor = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 16; i < 20; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        vert = NanoOcp1::DataToFloat(paramData, &ok);
    }
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        for (size_t i = 20; i < 24; i++)
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
        rot = NanoOcp1::DataToFloat(paramData, &ok);
    }

    return ok;
}

bool VariantToBoolVector(const juce::var& value, std::vector<bool>& boolVector)
{
    std::uint16_t listSize(0);

    jassert(value.isBinaryData());
    MemoryBlock* mb = value.getBinaryData();

    bool ok = (mb && (mb->getSize() >= 2)); // OcaList size takes up the first 2 bytes.
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        paramData.push_back(static_cast<std::uint8_t>(mb->begin()[0]));
        paramData.push_back(static_cast<std::uint8_t>(mb->begin()[1]));
        listSize = NanoOcp1::DataToUint16(paramData, &ok);
    }

    if (ok)
    {
        boolVector.clear();
        boolVector.reserve(listSize);
        for (int listIdx = 0; listIdx < listSize; ++listIdx)
        {
            boolVector.push_back(static_cast<std::uint8_t>(mb->begin()[listIdx + 2]) == static_cast <std::uint8_t>(1));
        }
        jassert(boolVector.size() == listSize);
    }

    return ok;
}

bool VariantToStringArray(const juce::var& value, juce::StringArray& stringArray)
{
    std::uint16_t listSize(0);
    size_t readPos = 0;

    jassert(value.isBinaryData());
    MemoryBlock* mb = value.getBinaryData();

    bool ok = (mb && (mb->getSize() >= 2)); // OcaList size takes up the first 2 bytes.
    if (ok)
    {
        std::vector<std::uint8_t> paramData;
        paramData.push_back(static_cast<std::uint8_t>(mb->begin()[readPos++]));
        paramData.push_back(static_cast<std::uint8_t>(mb->begin()[readPos++]));
        listSize = NanoOcp1::DataToUint16(paramData, &ok);
    }

    if (ok)
    {
        stringArray.clear();
        stringArray.ensureStorageAllocated(listSize);

        for (int listIdx = 0; listIdx < listSize; ++listIdx)
        {
            std::vector<std::uint8_t> paramData;
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[readPos++]));
            paramData.push_back(static_cast<std::uint8_t>(mb->begin()[readPos++]));
            auto stringLen = NanoOcp1::DataToUint16(paramData, &ok);

            if (ok)
            {
                stringArray.add(juce::String(std::string(mb->begin() + readPos, stringLen)));
                readPos += stringLen;
            }
        }

        jassert(stringArray.size() == listSize);
    }

    return ok;
}

juce::String StatusToString(std::uint8_t status)
{
    juce::String result;

    switch (status)
    {
        case 0: // OCASTATUS_OK:
            result = juce::String("OK");
            break;
        case 1: // OCASTATUS_PROTOCOL_VERSION_ERROR:
            result = juce::String("ProtocolVersionError");
            break;
        case 2: // OCASTATUS_DEVICE_ERROR:
            result = juce::String("DeviceError");
            break;
        case 3: // OCASTATUS_LOCKED:
            result = juce::String("Locked");
            break;
        case 4: // OCASTATUS_BAD_FORMAT:
            result = juce::String("BadFormat");
            break;
        case 5: // OCASTATUS_BAD_ONO:
            result = juce::String("BadONo");
            break;
        case 6: // OCASTATUS_PARAMETER_ERROR:
            result = juce::String("ParameterError");
            break;
        case 7: // OCASTATUS_PARAMETER_OUT_OF_RANGE:
            result = juce::String("ParameterOutOfRange");
            break;
        case 8: // OCASTATUS_NOT_IMPLEMENTED:
            result = juce::String("NotImplemented");
            break;
        case 9: // OCASTATUS_INVALID_REQUEST:
            result = juce::String("InvalidRequest");
            break;
        case 10: // OCASTATUS_PROCESSING_FAILED:
            result = juce::String("ProcessingFailed");
            break;
        case 11: // OCASTATUS_BAD_METHOD:
            result = juce::String("BadMethod");
            break;
        case 12: // OCASTATUS_PARTIALLY_SUCCEEDED:
            result = juce::String("PartiallySucceeded");
            break;
        case 13: // OCASTATUS_TIMEOUT:
            result = juce::String("Timeout");
            break;
        case 14: // OCASTATUS_BUFFER_OVERFLOW:
            result = juce::String("BufferOverflow");
            break;
        case 15: // OCASTATUS_PERMISSION_DENIED:
            result = juce::String("PermissionDenied");
            break;
        default:
            result = juce::String(status);
            break;
    }

    return result;
}

juce::String DataTypeToString(int dataType)
{
    juce::String result;

    switch (dataType)
    {
    case OCP1DATATYPE_BOOLEAN: 
        result = juce::String("Boolean");
        break;
    case OCP1DATATYPE_INT8:
        result = juce::String("Int8");
        break;
    case OCP1DATATYPE_INT16:
        result = juce::String("Int16");
        break;
    case OCP1DATATYPE_INT32:
        result = juce::String("Int32");
        break;
    case OCP1DATATYPE_INT64:
        result = juce::String("Int64");
        break;
    case OCP1DATATYPE_UINT8:
        result = juce::String("UInt8");
        break;
    case OCP1DATATYPE_UINT16:
        result = juce::String("UInt16");
        break;
    case OCP1DATATYPE_UINT32:
        result = juce::String("UInt32");
        break;
    case OCP1DATATYPE_UINT64:
        result = juce::String("UInt64");
        break;
    case OCP1DATATYPE_FLOAT32:
        result = juce::String("Float32");
        break;
    case OCP1DATATYPE_FLOAT64:
        result = juce::String("Float64");
        break;
    case OCP1DATATYPE_STRING:
        result = juce::String("String");
        break;
    case OCP1DATATYPE_BIT_STRING:
        result = juce::String("BitString");
        break;
    case OCP1DATATYPE_BLOB:
        result = juce::String("Blob");
        break;
    case OCP1DATATYPE_BLOB_FIXED_LEN:
        result = juce::String("BlobFixedLength");
        break;
    case OCP1DATATYPE_DB_POSITION:
        result = juce::String("Position (d&b)");
        break;
    case OCP1DATATYPE_CUSTOM:
        result = juce::String("Custom");
        break;
    default:
        break;
    }

    return result;
}

juce::String HandleToString(std::uint32_t handle)
{
    juce::String result;

    switch (handle)
    {
        case 0: // OCA_INVALID_SESSIONID
            result = juce::String("InvalidSessionID");
            break;
        case 1: // OCA_LOCAL_SESSIONID
            result = juce::String("LocalSessionID");
            break;
        default: 
            result = juce::String(handle);
            break;
    }

    return result;
}

std::uint32_t ReadUint32(const char* buffer)
{
    return (((static_cast<std::uint8_t>(buffer[0]) << 24) & 0xff000000) +
            ((static_cast<std::uint8_t>(buffer[1]) << 16) & 0x00ff0000) +
            ((static_cast<std::uint8_t>(buffer[2]) << 8)  & 0x0000ff00) +
              static_cast<std::uint8_t>(buffer[3]));
}

std::uint16_t ReadUint16(const char* buffer)
{
    return (((static_cast<std::uint8_t>(buffer[0]) << 8)  & 0xff00) +
              static_cast<std::uint8_t>(buffer[1]));
}

std::uint32_t GetONo(std::uint32_t type, std::uint32_t record, std::uint32_t channel, std::uint32_t boxAndObjectNumber)
{
    return (std::uint32_t((type) & 0xF) << 28)
        | (std::uint32_t((record) & 0xFF) << 20)
        | (std::uint32_t((channel) & 0x1F) << 15)
        | (std::uint32_t(boxAndObjectNumber) & 0x7FFF);
}

std::uint32_t GetONoTy2(std::uint32_t type, std::uint32_t record, std::uint32_t channel, std::uint32_t boxNumber, std::uint32_t objectNumber)
{
    return (std::uint32_t((type) & 0xF) << 28)
        | (std::uint32_t((record) & 0xFF) << 20)
        | (std::uint32_t((channel) & 0xFF) << 12)
        | (std::uint32_t((boxNumber) & 0x1F) << 7)
        | (std::uint32_t((objectNumber) & 0x7F));
}


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
                                 DataFromOnoForSubscription(m_targetOno));
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

Ocp1CommandDefinition Ocp1CommandDefinition::SetValueCommand(const juce::var& newValue) const
{
    std::uint8_t paramCount(0);
    std::vector<std::uint8_t> newParamData;

    switch (m_propertyType) // See enum Ocp1DataType
    {
        case OCP1DATATYPE_INT32:
            paramCount = 1;
            newParamData = DataFromInt32(static_cast<std::int32_t>(int(newValue)));
            break;
        case OCP1DATATYPE_UINT8:
            paramCount = 1;
            newParamData = DataFromUint8(static_cast<std::uint8_t>(int(newValue)));
            break;
        case OCP1DATATYPE_UINT16:
            paramCount = 1;
            newParamData = DataFromUint16(static_cast<std::uint16_t>(int(newValue)));
            break;
        case OCP1DATATYPE_UINT32:
            paramCount = 1;
            newParamData = DataFromUint32(static_cast<std::uint32_t>(int(newValue)));
            break;
        case OCP1DATATYPE_FLOAT32:
            paramCount = 1;
            newParamData = DataFromFloat(float(newValue));
            break;
        case OCP1DATATYPE_STRING:
            paramCount = 1;
            newParamData = DataFromString(newValue.toString());
            break;
        case OCP1DATATYPE_DB_POSITION:
            {
                paramCount = 1;
                MemoryBlock* mb = newValue.getBinaryData();
                if (mb->getSize() >= 12)
                {
                    newParamData.reserve(12);
                    for (size_t i = 0; i < 12; i++)
                        newParamData.push_back(static_cast<std::uint8_t>(mb->begin()[i]));
                }
            }
            break;
        case OCP1DATATYPE_BOOLEAN:
            {
                paramCount = 1;
                newParamData = { newValue.equals(bool(true)) ? static_cast<std::uint8_t>(1) : static_cast<std::uint8_t>(0)};
            }
            break;
        case OCP1DATATYPE_NONE:
        case OCP1DATATYPE_INT8:
        case OCP1DATATYPE_INT16:
        case OCP1DATATYPE_INT64:
        case OCP1DATATYPE_UINT64:
        case OCP1DATATYPE_FLOAT64:
        case OCP1DATATYPE_BIT_STRING:
        case OCP1DATATYPE_BLOB:
        case OCP1DATATYPE_BLOB_FIXED_LEN:
        case OCP1DATATYPE_CUSTOM:
        default:
            jassert(false); // Type conversion not implemented yet.
            break;
    }

    return Ocp1CommandDefinition(m_targetOno,
                                 m_propertyType,
                                 m_propertyDefLevel,
                                 2,                     // Set method is usually MethodIdx 2
                                 paramCount,
                                 newParamData);
}

juce::var Ocp1CommandDefinition::ToVariant(std::uint8_t paramCount, const std::vector<std::uint8_t>& parameterData)
{
    juce::var ret;

    // NOTE: Notifications usually contain 2 parameters: the context and the new value.
    bool ok = (paramCount == 1) || (paramCount == 2) || (paramCount == 3) || (paramCount == 6);

    if (ok)
    {
        ok = false;
        switch (m_propertyType) // See enum Ocp1DataType
        {
            case OCP1DATATYPE_INT32:
                ret = (int)NanoOcp1::DataToInt32(parameterData, &ok);
                break;
            case OCP1DATATYPE_UINT8:
                ret = NanoOcp1::DataToUint8(parameterData, &ok);
                break;
            case OCP1DATATYPE_UINT16:
                ret = NanoOcp1::DataToUint16(parameterData, &ok);
                break;
            case OCP1DATATYPE_UINT32:
                ret = (int)NanoOcp1::DataToUint32(parameterData, &ok);
                break;
            case OCP1DATATYPE_UINT64:
                ret = (juce::int64)NanoOcp1::DataToUint64(parameterData, &ok);
                break;
            case OCP1DATATYPE_FLOAT32:
                ret = NanoOcp1::DataToFloat(parameterData, &ok);
                break;
            case OCP1DATATYPE_STRING:
                ret = DataToString(parameterData, &ok);
                break;
            case OCP1DATATYPE_DB_POSITION:
                ok = (parameterData.size() == 12) || // Notification contains 3 floats: x, y, z.
                     (parameterData.size() == 24) || // Notification contains 6 floats: x, y, z, hor, vert, rot.
                     (parameterData.size() == 36);   // Response contains 9 floats: current, min, and max x, y, z.
                if (ok)
                {
                    ret = juce::MemoryBlock((const char*)parameterData.data(), parameterData.size());
                }
                break;
            case OCP1DATATYPE_BLOB:
                ok = (parameterData.size() >= 2); // OcaBlob size is 2 bytes
                if (ok)
                {
                    ret = juce::MemoryBlock((const char*)parameterData.data(), parameterData.size());
                }
                break;
            case OCP1DATATYPE_BOOLEAN:
                ok = parameterData.size() == 1;
                ret = (bool)(parameterData[0] == static_cast<std::uint8_t>(1));
                break;
            case OCP1DATATYPE_NONE:
            case OCP1DATATYPE_INT8:
            case OCP1DATATYPE_INT16:
            case OCP1DATATYPE_INT64:
            case OCP1DATATYPE_FLOAT64:
            case OCP1DATATYPE_BIT_STRING:
            case OCP1DATATYPE_BLOB_FIXED_LEN:
            case OCP1DATATYPE_CUSTOM:
            default:
                break;
        }
    }

    jassert(ok); // Type conversion failed or not implemented.
    return ret;
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
        m_syncVal = memoryBlock[0];
        jassert(m_syncVal == 0x3b); // Message does not start with the sync byte.

        m_protoVers = ReadUint16(memoryBlock.begin() + 1);
        jassert(m_protoVers == 1); // Protocol version is expected to be 1.
        
        m_msgSize = ReadUint32(memoryBlock.begin() + 3);
        jassert(m_msgSize >= Ocp1HeaderSize); // Message has unexpected size.

        m_msgType = memoryBlock[7];
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
};

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
            ret = static_cast<std::uint32_t>(11);
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
                std::uint8_t paramCount = receivedData[22];
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

                std::uint8_t status = receivedData[18];
                std::uint8_t paramCount = receivedData[19];

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
};



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
};



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
};



//==============================================================================
// Class Ocp1KeepAlive
//==============================================================================

std::vector<std::uint8_t> Ocp1KeepAlive::GetSerializedData()
{
    std::vector<std::uint8_t> serializedData = m_header.GetSerializedData();

    serializedData.push_back(static_cast<std::uint8_t>(m_heartBeat >> 8));
    serializedData.push_back(static_cast<std::uint8_t>(m_heartBeat));

    return serializedData;
};

}