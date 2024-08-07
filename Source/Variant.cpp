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

#include "Variant.h"
#include <assert.h>


namespace NanoOcp1
{

Variant::Variant(bool v) { m_value = v; }
Variant::Variant(std::int32_t v) { m_value = v; }
Variant::Variant(std::uint8_t v) { m_value = v; }
Variant::Variant(std::uint16_t v) { m_value = v; }
Variant::Variant(std::uint32_t v) { m_value = v; }
Variant::Variant(std::uint64_t v) { m_value = v; }
Variant::Variant(std::float_t v) { m_value = v; }
Variant::Variant(std::double_t v) { m_value = v; }
Variant::Variant(const std::string& v) { m_value = v; }
Variant::Variant(const char* v) : Variant(std::string(v)) { } // Allow Variant("test") to become of TypeString.

Variant::Variant(std::float_t x, std::float_t y, std::float_t z)
{
    assert(sizeof(std::uint32_t) == sizeof(std::float_t)); // Required for pointer cast to work below
    std::uint32_t xInt = *(std::uint32_t*)&x;
    std::uint32_t yInt = *(std::uint32_t*)&y;
    std::uint32_t zInt = *(std::uint32_t*)&z;

    m_value = std::vector<std::uint8_t>
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
}

Variant::Variant(const std::vector<std::uint8_t>& data, Ocp1DataType type)
{
    bool ok(false);
    switch (type)
    {
        case OCP1DATATYPE_BOOLEAN:
            m_value = NanoOcp1::DataToBool(data, &ok);
            break;
        case OCP1DATATYPE_INT32:
            m_value = NanoOcp1::DataToInt32(data, &ok);
            break;
        case OCP1DATATYPE_UINT8:
            m_value = NanoOcp1::DataToUint8(data, &ok);
            break;
        case OCP1DATATYPE_UINT16:
            m_value = NanoOcp1::DataToUint16(data, &ok);
            break;
        case OCP1DATATYPE_UINT32:
            m_value = NanoOcp1::DataToUint32(data, &ok);
            break;
        case OCP1DATATYPE_UINT64:
            m_value = NanoOcp1::DataToUint64(data, &ok);
            break;
        case OCP1DATATYPE_FLOAT32:
            m_value = NanoOcp1::DataToFloat(data, &ok);
            break;
        case OCP1DATATYPE_FLOAT64:
            m_value = NanoOcp1::DataToDouble(data, &ok);
            break;
        case OCP1DATATYPE_STRING:
            m_value = NanoOcp1::DataToString(data, &ok);
            break;
        case OCP1DATATYPE_BLOB:
            ok = (data.size() >= 2); // OcaBlob size is 2 bytes
            if (ok)
            {
                // TODO: include 2 initial bytes?
                m_value = data;
            }
            break;
        case OCP1DATATYPE_DB_POSITION:
            ok = (data.size() == 12) || // Notification contains 3 floats: x, y, z.
                 (data.size() == 24) || // Notification contains 6 floats: x, y, z, hor, vert, rot.
                 (data.size() == 36);   // Response contains 9 floats: current, min, and max x, y, z.
            if (ok)
            {
                m_value = data;
            }
            break;
        case OCP1DATATYPE_NONE:
        case OCP1DATATYPE_INT8:
        case OCP1DATATYPE_INT16:
        case OCP1DATATYPE_INT64:
        case OCP1DATATYPE_BIT_STRING:
        case OCP1DATATYPE_BLOB_FIXED_LEN:
        case OCP1DATATYPE_CUSTOM:
        default:
            break;
    }

    assert(ok); // Conversion not possible or not yet implemented!
}

bool Variant::operator==(const Variant& other) const
{
    return m_value == other.m_value;
}

bool Variant::operator!=(const Variant& other) const
{
    return m_value != other.m_value;
}

bool Variant::IsValid() const
{
    return (m_value.index() != TypeNone);
}

Ocp1DataType Variant::GetDataType() const
{
    switch (m_value.index())
    {
        case TypeBool:          return OCP1DATATYPE_BOOLEAN;
        case TypeInt32:         return OCP1DATATYPE_INT32;
        case TypeUInt8:         return OCP1DATATYPE_UINT8;
        case TypeUInt16:        return OCP1DATATYPE_UINT16;
        case TypeUInt32:        return OCP1DATATYPE_UINT32;
        case TypeUInt64:        return OCP1DATATYPE_UINT64;
        case TypeFloat:         return OCP1DATATYPE_FLOAT32;
        case TypeDouble:        return OCP1DATATYPE_FLOAT64;
        case TypeString:        return OCP1DATATYPE_STRING;
        case TypeByteVector:    return OCP1DATATYPE_BLOB;
        default:
            break;
    }

    return OCP1DATATYPE_NONE;
}

bool Variant::ToBool(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value);
        case TypeInt32:
            return (std::get<std::int32_t>(m_value) > std::int32_t(0));
        case TypeUInt8:
            return (std::get<std::uint8_t>(m_value) > std::uint8_t(0));
        case TypeUInt16:
            return (std::get<std::uint16_t>(m_value) > std::uint16_t(0));
        case TypeUInt32:
            return (std::get<std::uint32_t>(m_value) > std::uint32_t(0));
        case TypeUInt64:
            return (std::get<std::uint64_t>(m_value) > std::uint64_t(0));
        case TypeFloat:
            return (std::get<std::float_t>(m_value) > std::float_t(0.0f));
        case TypeDouble:
            return (std::get<std::double_t>(m_value) > std::double_t(0.0));
        case TypeString:
            return (std::get<std::string>(m_value) == "true");
        case TypeByteVector:
            return DataToBool(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return false;
}

std::int32_t Variant::ToInt32(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? 1 : 0;
        case TypeInt32:
            return std::get<std::int32_t>(m_value);
        case TypeUInt8:
            return static_cast<std::int32_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return static_cast<std::int32_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::int32_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::int32_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return std::lround(std::get<std::float_t>(m_value));
        case TypeDouble:
            return std::lround(std::get<std::double_t>(m_value));
        case TypeString:
            try 
            { 
                return std::stol(std::get<std::string>(m_value)); 
            } 
            catch (...) 
            { 
                break; 
            }
        case TypeByteVector:
            return DataToInt32(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::int32_t(0);
}

std::uint8_t Variant::ToUInt8(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::uint8_t(1) : std::uint8_t(0);
        case TypeInt32:
            return static_cast<std::uint8_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return std::get<std::uint8_t>(m_value);
        case TypeUInt16:
            return static_cast<std::uint8_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::uint8_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::uint8_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return static_cast<std::uint8_t>(std::lround(std::get<std::float_t>(m_value)));
        case TypeDouble:
            return static_cast<std::uint8_t>(std::lround(std::get<std::double_t>(m_value)));
        case TypeString:
            try
            {
                return static_cast<std::uint8_t>(std::stol(std::get<std::string>(m_value)));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToUint8(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::uint8_t(0);
}

std::uint16_t Variant::ToUInt16(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::uint16_t(1) : std::uint16_t(0);
        case TypeInt32:
            return static_cast<std::uint16_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return static_cast<std::uint16_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return std::get<std::uint16_t>(m_value);
        case TypeUInt32:
            return static_cast<std::uint16_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::uint16_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return static_cast<std::uint16_t>(std::lround(std::get<std::float_t>(m_value)));
        case TypeDouble:
            return static_cast<std::uint16_t>(std::lround(std::get<std::double_t>(m_value)));
        case TypeString:
            try
            {
                return static_cast<std::uint16_t>(std::stoi(std::get<std::string>(m_value)));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToUint16(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::uint16_t(0);
}

std::uint32_t Variant::ToUInt32(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::uint32_t(1) : std::uint32_t(0);
        case TypeInt32:
            return static_cast<std::uint32_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return static_cast<std::uint32_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return static_cast<std::uint32_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::uint32_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::uint32_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return static_cast<std::uint32_t>(std::lround(std::get<std::float_t>(m_value)));
        case TypeDouble:
            return static_cast<std::uint32_t>(std::lround(std::get<std::double_t>(m_value)));
        case TypeString:
            try
            {
                return static_cast<std::uint32_t>(std::stoi(std::get<std::string>(m_value)));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToUint32(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::uint32_t(0);
}

std::uint64_t Variant::ToUInt64(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::uint64_t(1) : std::uint64_t(0);
        case TypeInt32:
            return static_cast<std::uint64_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return static_cast<std::uint64_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return static_cast<std::uint64_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::uint64_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::uint64_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return static_cast<std::uint64_t>(std::llround(std::get<std::float_t>(m_value)));
        case TypeDouble:
            return static_cast<std::uint64_t>(std::llround(std::get<std::double_t>(m_value)));
        case TypeString:
            try
            {
                return static_cast<std::uint64_t>(std::stoll(std::get<std::string>(m_value)));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToUint64(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::uint64_t(0);
}

std::double_t Variant::ToDouble(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::double_t(1.0) : std::double_t(0.0);
        case TypeInt32:
            return static_cast<std::double_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return static_cast<std::double_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return static_cast<std::double_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::double_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::double_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return static_cast<std::double_t>(std::get<std::float_t>(m_value));
        case TypeDouble:
            return std::get<std::double_t>(m_value);
        case TypeString:
            try
            {
                return std::stod(std::get<std::string>(m_value));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToDouble(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::double_t(0.0);
}

std::float_t Variant::ToFloat(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return std::get<bool>(m_value) ? std::float_t(1.0f) : std::float_t(0.0f);
        case TypeInt32:
            return static_cast<std::float_t>(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return static_cast<std::float_t>(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return static_cast<std::float_t>(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return static_cast<std::float_t>(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return static_cast<std::float_t>(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return std::get<std::float_t>(m_value);
        case TypeDouble:
            return static_cast<std::float_t>(std::get<std::double_t>(m_value));
        case TypeString:
            try
            {
                return static_cast<std::float_t>(std::stof(std::get<std::string>(m_value)));
            }
            catch (...)
            {
                break;
            }
        case TypeByteVector:
            return DataToFloat(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::float_t(0.0f);
}

std::string Variant::ToString(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return (std::get<bool>(m_value) ? "true" : "false");
        case TypeInt32:
            return std::to_string(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return std::to_string(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return std::to_string(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return std::to_string(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return std::to_string(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return std::to_string(std::get<std::float_t>(m_value));
        case TypeDouble:
            return std::to_string(std::get<std::double_t>(m_value));
        case TypeString:
            return std::get<std::string>(m_value);
        case TypeByteVector:
            return DataToString(std::get<std::vector<std::uint8_t>>(m_value), pOk);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::string{};
}

std::vector<std::uint8_t> Variant::ToByteVector(bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    switch (m_value.index())
    {
        case TypeBool:
            return DataFromBool(std::get<bool>(m_value));
        case TypeInt32:
            return DataFromInt32(std::get<std::int32_t>(m_value));
        case TypeUInt8:
            return DataFromUint8(std::get<std::uint8_t>(m_value));
        case TypeUInt16:
            return DataFromUint16(std::get<std::uint16_t>(m_value));
        case TypeUInt32:
            return DataFromUint32(std::get<std::uint32_t>(m_value));
        case TypeUInt64:
            return DataFromUint64(std::get<std::uint64_t>(m_value));
        case TypeFloat:
            return DataFromFloat(std::get<std::float_t>(m_value));
        case TypeDouble:
            return DataFromDouble(std::get<std::double_t>(m_value));
        case TypeString:
            return DataFromString(std::get<std::string>(m_value));
        case TypeByteVector:
            return std::get<std::vector<std::uint8_t>>(m_value);
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;

    return std::vector<std::uint8_t>{};
}

std::vector<std::uint8_t> Variant::ToParamData(Ocp1DataType type /* = OCP1DATATYPE_NONE */, bool* pOk) const
{
    if (pOk != nullptr) *pOk = true;

    Ocp1DataType nativeType(type);
    if (type == OCP1DATATYPE_NONE)
        nativeType = GetDataType();

    switch (nativeType)
    {
        case OCP1DATATYPE_BOOLEAN:
            return DataFromBool(ToBool(pOk));
        case OCP1DATATYPE_INT32:
            return DataFromInt32(ToInt32(pOk));
        case OCP1DATATYPE_UINT8:
            return DataFromUint8(ToUInt8(pOk));
        case OCP1DATATYPE_UINT16:
            return DataFromUint16(ToUInt16(pOk));
        case OCP1DATATYPE_UINT32:
            return DataFromUint32(ToUInt32(pOk));
        case OCP1DATATYPE_UINT64:
            return DataFromUint64(ToUInt64(pOk));
        case OCP1DATATYPE_FLOAT32:
            return DataFromFloat(ToFloat(pOk));
        case OCP1DATATYPE_FLOAT64:
            return DataFromDouble(ToDouble(pOk));
        case OCP1DATATYPE_STRING:
            return DataFromString(ToString(pOk));
        case OCP1DATATYPE_BLOB:
            return ToByteVector(pOk);
        case OCP1DATATYPE_DB_POSITION:
            return ToByteVector(pOk);
        case OCP1DATATYPE_NONE:
        case OCP1DATATYPE_INT8:
        case OCP1DATATYPE_INT16:
        case OCP1DATATYPE_INT64:
        case OCP1DATATYPE_BIT_STRING:
        case OCP1DATATYPE_BLOB_FIXED_LEN:
        case OCP1DATATYPE_CUSTOM:
        default:
            break;
    }

    // Conversion not possible or not yet implemented!
    if (pOk != nullptr) *pOk = false;
    
    return std::vector<std::uint8_t>{};
}

std::array<std::float_t, 3> Variant::ToPosition(bool* pOk) const
{
    std::array<std::float_t, 3> ret{ 0.0f };

    if (m_value.index() != TypeByteVector)
    {
        if (pOk != nullptr)
            *pOk = false;

        return ret;
    }

    const auto& data = std::get<std::vector<std::uint8_t>>(m_value);
    bool ok = ((data.size() == 12) || // Value contains 3 floats: x, y, z.
               (data.size() == 36));  // Value contains 9 floats: x, y, z, plus min and max each on top.

    if (ok)
        ret[0] = NanoOcp1::DataToFloat(data, &ok); // x

    if (ok)
        ret[1] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 4, data.data() + 8), &ok); // y

    if (ok)
        ret[2] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 8, data.data() + 12), &ok); // z

    if (pOk != nullptr)
        *pOk = ok;

    return ret;
}

std::array<std::float_t, 6> Variant::ToPositionAndRotation(bool* pOk) const
{
    std::array<std::float_t, 6> ret{ 0.0f };

    if (m_value.index() != TypeByteVector)
    {
        if (pOk != nullptr)
            *pOk = false;

        return ret;
    }

    const auto& data = std::get<std::vector<std::uint8_t>>(m_value);
    bool ok = (data.size() == 24); // Value contains 6 floats: x, y, z, horAngle, vertAngle, rotAngle.
    
    if (ok)
        ret[0] = NanoOcp1::DataToFloat(data, &ok); // x

    if (ok)
        ret[1] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 4, data.data() + 8), &ok); // y

    if (ok)
        ret[2] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 8, data.data() + 12), &ok); // z

    if (ok)
        ret[3] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 12, data.data() + 16), &ok); // hor

    if (ok)
        ret[4] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 16, data.data() + 20), &ok); // ver

    if (ok)
        ret[5] = NanoOcp1::DataToFloat(std::vector<std::uint8_t>(data.data() + 20, data.data() + 24), &ok); // rot

    if (pOk != nullptr)
        *pOk = ok;

    return ret;
}

std::vector<bool> Variant::ToBoolVector(bool* pOk) const
{
    std::vector<bool> boolVector;

    if (m_value.index() != TypeByteVector)
    {
        if (pOk != nullptr)
            *pOk = false;

        return boolVector;
    }

    const auto& data = std::get<std::vector<std::uint8_t>>(m_value);
    bool ok = (data.size() >= 2); // OcaList size takes up the first 2 bytes.

    std::uint16_t listSize(0);
    if (ok)
        listSize = NanoOcp1::DataToUint16(data, &ok);

    ok = ok && (data.size() == listSize + 2);
    if (ok && listSize > 0)
    {
        boolVector.reserve(listSize);
        std::size_t readPos(2); // Start after the OcaList size bytes
        while (readPos < data.size() && ok)
        {
            std::vector<std::uint8_t> tmpData(data.data() + readPos, data.data() + readPos + 1);
            boolVector.push_back(NanoOcp1::DataToBool(tmpData, &ok));
            readPos++;
        }
    }

    ok = ok && (boolVector.size() == listSize);

    if (pOk != nullptr)
        *pOk = ok;

    return boolVector;
}

std::vector<std::string> Variant::ToStringVector(bool* pOk) const
{
    std::vector<std::string> stringVector;

    if (m_value.index() != TypeByteVector)
    {
        if (pOk != nullptr)
            *pOk = false;

        return stringVector;
    }

    const auto& data = std::get<std::vector<std::uint8_t>>(m_value);
    bool ok = (data.size() >= 2); // OcaList size takes up the first 2 bytes.

    std::uint16_t listSize(0);
    if (ok)
        listSize = NanoOcp1::DataToUint16(data, &ok);

    // Byte vector has the right size even assuming empty strings (min 2 bytes per string + 2 bytes for list size).
    ok = ok && (data.size() >= ((listSize * 2) + 2));
    if (ok && listSize > 0)
    {
        stringVector.reserve(static_cast<size_t>(listSize));
        std::size_t readPos(2); // Start after the OcaList size bytes
        while (readPos < data.size() && ok)
        {
            std::vector<std::uint8_t> stringLenData(data.data() + readPos, data.data() + readPos + 2);
            auto stringLen = NanoOcp1::DataToUint16(stringLenData, &ok);
            readPos += 2;

            if (ok)
            {
                stringVector.push_back(std::string(data.data() + readPos, data.data() + readPos + stringLen));
                readPos += stringLen;
            }
        }
    }

    ok = ok && (stringVector.size() == listSize);

    if (pOk != nullptr)
        *pOk = ok;

    return stringVector;
}

}
