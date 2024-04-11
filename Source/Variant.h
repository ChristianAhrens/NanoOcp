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

#include <JuceHeader.h>     //< USE juce::StringArray
#include <variant>          //< USE std::variant
#include "Ocp1DataTypes.h"  //< USE NanoOcp1::Ocp1DataType


namespace NanoOcp1
{

/**
 * @class Variant
 * Essentially a wrapper class for a std::variant with added functionality:
 *  - Mutability: as opposed to a pure std::variant, the Variant can be constructed with a given type
 *    and be later interpreted as a different type using the available To<T> conversion methods.
 *    Note that these type conversions are mostly only supported between primitive types.
 *  - Marshaling: A Variant can be directly created (unmarshaled) from a byte vector, 
 *    and it can also be marshaled into its byte-vector representation using the ToParamData method.
 */
class Variant
{
public:
    Variant(bool v);
    Variant(std::int32_t v);
    Variant(std::uint8_t v);
    Variant(std::uint16_t v);
    Variant(std::uint32_t v);
    Variant(std::uint64_t v);
    Variant(std::float_t v);
    Variant(std::double_t v);
    Variant(const std::string& v);
    Variant(const char* v);
    Variant(std::float_t x, std::float_t y, std::float_t z);

    /**
     * Default constructor. Type-less and value-less per default, and will return FALSE on IsValid as such.
     */
    Variant() = default;

    /**
     * Unmarshaling constructor.
     * Deserializes the data from the passed byte vector into the object using the passed type.
     *
     * @param[in] data  Byte vector representing the parameter data obtained by i.e. an OCP1 Notification or Response. 
     * @param[in] type  Data type of the Ocp1CommandDefinition associated with that OCP1 message.
     */
    Variant(const std::vector<std::uint8_t>& data, Ocp1DataType type = OCP1DATATYPE_BLOB);

    virtual ~Variant() = default;
    bool operator==(const Variant& other) const;
    bool operator!=(const Variant& other) const;

    /**
     * Check if this Variant has a valid value and type, i.e. different than the default TypeNone (std::monostate).
     * @note A Variant has no value or type per default.
     * 
     * @return True if this Variant is valid.
     */
    bool IsValid() const;

    /**
     * Gives the native type of this Variant, i.e. the type it was created as.
     *
     * @return This Variant's native type, as a Ocp1DataType.
     */
    Ocp1DataType GetType() const;

    /**
     * Marshal the Variant's value into a byte-vector representation, based on the desired type.
     * 
     * @param[in] type  Data type to unmarshal the Varaiant as. 
     *                  If this is left as the default (NONE), the Variant's native type will be used.
     */
    std::vector<std::uint8_t> ToParamData(Ocp1DataType type = OCP1DATATYPE_NONE) const;

    /**
     * Type conversion methods. 
     * Note that some of these conversions can leas to signedness change or data loss.
     */

    bool ToBool() const;
    std::int32_t ToInt32() const;
    std::uint8_t ToUInt8() const;
    std::uint16_t ToUInt16() const;
    std::uint32_t ToUInt32() const;
    std::uint64_t ToUInt64() const;
    std::float_t ToFloat() const;
    std::double_t ToDouble() const;
    std::string ToString() const;

    /**
     * Convenience helper method to extract x, y, and z float values from a Variant.
     * The Variant should internally contain the values as 3 x 4 bytes.
     * 
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained x, y, and z values.
     */
    std::array<std::float_t, 3> ToPosition(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract x, y, z, horizontal angle, vertical angle
     * and rotation angle float values from a Variant.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained x, y, z, hor, ver, and rot values.
     */
    std::array<std::float_t, 6> ToPositionAndRotation(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract a std::vector<bool> from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaBoolean>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>. 
     */
    std::vector<bool> ToBoolVector(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract a juce::StringArray from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaString>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>.
     */
    juce::StringArray ToStringArray(bool* pOk = nullptr) const;


protected:
    /**
     * Marshals the Variant into a byte vector using a format based on the Variant's native type.
     * 
     * @return  The Variant's byte vector representation.
     */
    std::vector<std::uint8_t> ToByteVector() const;

    /**
     * Used internally to identify the possible types that the internal std::variant can assume.
     * Mirrors enum Ocp1DataType, but without gaps (essential for std::variant) or unused types.
     */
    enum TypeIndex
    {
        TypeNone = 0,
        TypeBool,
        TypeInt32,
        TypeUInt8,
        TypeUInt16,
        TypeUInt32,
        TypeUInt64,
        TypeFloat,
        TypeDouble,
        TypeString,
        TypeByteVector
    };

    /**
     * Definition of the internal std::variant class template.
     * The internal m_value holds a value of exactly one of the alternative types at any given time.
     */
    using VariantType = std::variant<std::monostate,                // TypeNone
                                     bool,                          // TypeBool
                                     std::int32_t,                  // TypeInt32
                                     std::uint8_t,                  // TypeUInt8
                                     std::uint16_t,                 // TypeUInt16
                                     std::uint32_t,                 // TypeUInt32
                                     std::uint64_t,                 // TypeUInt64
                                     std::float_t,                  // TypeFloat
                                     std::double_t,                 // TypeDouble
                                     std::string,                   // TypeString
                                     std::vector<std::uint8_t>>;    // TypeByteVector

private:
    /**
     * Internal value as a std::variant.
     */
    VariantType m_value;
};

}