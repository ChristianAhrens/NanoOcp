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
 
#include "Ocp1DataTypes.h"


namespace NanoOcp1
{

static constexpr std::uint8_t uint8_8(8);
static constexpr std::uint8_t uint8_16(16);
static constexpr std::uint8_t uint8_24(24);
static constexpr std::uint8_t uint8_32(32);
static constexpr std::uint8_t uint8_40(40);
static constexpr std::uint8_t uint8_48(48);
static constexpr std::uint8_t uint8_56(56);

bool DataToBool(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    bool ret(false);
    bool ok = parameterData.size() == 1;

    if (ok)
    {
        ret = (bool)(parameterData[0] == static_cast<std::uint8_t>(1));
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromBool(bool boolValue)
{
    return std::vector<std::uint8_t>{ boolValue ? static_cast<std::uint8_t>(1) : static_cast<std::uint8_t>(0) };
}

std::int32_t DataToInt32(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::int32_t ret(0);

    // HACK: Use >= and not == to easily deal with responses sometimes including min and max values.
    bool ok = (parameterData.size() >= sizeof(std::int32_t)); // 4 bytes expected.
    if (ok)
    {
        ret = (((parameterData[0] << uint8_24) & 0xff000000) +
               ((parameterData[1] << uint8_16) & 0x00ff0000) +
               ((parameterData[2] << uint8_8)  & 0x0000ff00) + parameterData[3]);
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
            ret = ((ret << static_cast<std::uint16_t>(8)) & 0xff00) + parameterData[i];
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
        ret = (((parameterData[0] << uint8_24) & 0xff000000) +
               ((parameterData[1] << uint8_16) & 0x00ff0000) +
               ((parameterData[2] << uint8_8)  & 0x0000ff00) + parameterData[3]);
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

std::vector<std::uint8_t> DataFromUint64(std::uint64_t intValue)
{
    return std::vector<std::uint8_t>
        ({
            static_cast<std::uint8_t>(intValue >> 56),
            static_cast<std::uint8_t>(intValue >> 48),
            static_cast<std::uint8_t>(intValue >> 40),
            static_cast<std::uint8_t>(intValue >> 32),
            static_cast<std::uint8_t>(intValue >> 24),
            static_cast<std::uint8_t>(intValue >> 16),
            static_cast<std::uint8_t>(intValue >> 8),
            static_cast<std::uint8_t>(intValue),
        });
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
    
    ret.reserve(static_cast<std::size_t>(stringLength + 2));
    ret.push_back(static_cast<std::uint8_t>(stringLength >> 8));
    ret.push_back(static_cast<std::uint8_t>(stringLength));
    for (int i = 0; i < stringLength; i++)
    {
        ret.push_back(static_cast<std::uint8_t>(pStringData[i]));
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
        int intValue = (((parameterData[0] << uint8_24) & 0xff000000) +
                        ((parameterData[1] << uint8_16) & 0x00ff0000) +
                        ((parameterData[2] << uint8_8)  & 0x0000ff00) + parameterData[3]);
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

std::double_t DataToDouble(const std::vector<std::uint8_t>& parameterData, bool* pOk)
{
    std::double_t ret(0);

    bool ok = (parameterData.size() >= sizeof(std::double_t)); // 8 bytes expected.
    ok = ok && (sizeof(std::uint64_t) == sizeof(std::double_t)); // Required for pointer cast to work
    if (ok)
    {
        std::uint64_t intValue = (((static_cast<std::uint64_t>(parameterData[0]) << uint8_56) & 0xff00000000000000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[1]) << uint8_48) & 0x00ff000000000000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[2]) << uint8_40) & 0x0000ff0000000000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[3]) << uint8_32) & 0x000000ff00000000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[4]) << uint8_24) & 0x00000000ff000000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[5]) << uint8_16) & 0x0000000000ff0000ULL) +
                                  ((static_cast<std::uint64_t>(parameterData[6]) << uint8_8)  & 0x000000000000ff00ULL) + 
                                  parameterData[7]);
        ret = *(std::double_t*)&intValue;
    }

    if (pOk != nullptr)
    {
        *pOk = ok;
    }

    return ret;
}

std::vector<std::uint8_t> DataFromDouble(std::double_t doubleValue)
{
    jassert(sizeof(std::uint64_t) == sizeof(std::double_t)); // Required for pointer cast to work
    std::uint64_t intValue = *(std::uint64_t*)&doubleValue;

    return std::vector<std::uint8_t>
        ({ 
            static_cast<std::uint8_t>(intValue >> 56),
            static_cast<std::uint8_t>(intValue >> 48),
            static_cast<std::uint8_t>(intValue >> 40),
            static_cast<std::uint8_t>(intValue >> 32),
            static_cast<std::uint8_t>(intValue >> 24),
            static_cast<std::uint8_t>(intValue >> 16),
            static_cast<std::uint8_t>(intValue >> 8),
            static_cast<std::uint8_t>(intValue),
        });
}

std::vector<std::uint8_t> DataFromPosition(std::float_t x, std::float_t y, std::float_t z)
{
    jassert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast below
    std::uint32_t xInt = *(std::uint32_t*)&x;
    std::uint32_t yInt = *(std::uint32_t*)&y;
    std::uint32_t zInt = *(std::uint32_t*)&z;

    std::vector<std::uint8_t> ret
    ({
        static_cast<std::uint8_t>(xInt >> 24),
        static_cast<std::uint8_t>(xInt >> 16),
        static_cast<std::uint8_t>(xInt >> 8),
        static_cast<std::uint8_t>(xInt),
        static_cast<std::uint8_t>(yInt >> 24),
        static_cast<std::uint8_t>(yInt >> 16),
        static_cast<std::uint8_t>(yInt >> 8),
        static_cast<std::uint8_t>(yInt),
        static_cast<std::uint8_t>(zInt >> 24),
        static_cast<std::uint8_t>(zInt >> 16),
        static_cast<std::uint8_t>(zInt >> 8),
        static_cast<std::uint8_t>(zInt),
    });

    return ret;
}

std::vector<std::uint8_t> DataFromPositionAndRotation(std::float_t x, std::float_t y, std::float_t z, std::float_t hor, std::float_t vert, std::float_t rot)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(6 * 4);

    jassert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast to work

    std::uint32_t intValue = *(std::uint32_t*)&hor;
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

    intValue = *(std::uint32_t*)&x;
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

std::vector<std::uint8_t> DataFromOnoForSubscription(std::uint32_t ono, bool add)
{
    std::vector<std::uint8_t> ret;
    ret.reserve(add ? 25 : 16);

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
    
    if (!add)
        return ret;

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
    return (((static_cast<std::uint8_t>(buffer[0]) << uint8_24) & 0xff000000) +
            ((static_cast<std::uint8_t>(buffer[1]) << uint8_16) & 0x00ff0000) +
            ((static_cast<std::uint8_t>(buffer[2]) << uint8_8)  & 0x0000ff00) +
              static_cast<std::uint8_t>(buffer[3]));
}

std::uint16_t ReadUint16(const char* buffer)
{
    return (((static_cast<std::uint8_t>(buffer[0]) << uint8_8)  & 0xff00) +
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

}
