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
#include "Ocp1DataTypes.h"


namespace NanoOcp1
{

/**
 * @class Variant
 * @brief Representation of a type-safe union, which can be of exactly one type at any time.
 */
class Variant
{
public:
    /**
     * TODO: document
     */
    Variant() = default;

    /**
     * TODO: document
     */
    Variant(bool v);

    /**
     * TODO: document
     */
    Variant(std::int32_t v);

    /**
     * TODO: document
     */
    Variant(std::uint64_t v);

    /**
     * TODO: document
     */
    Variant(std::float_t v);

    /**
     * TODO: document
     */
    Variant(double v);

    /**
     * TODO: document
     */
    Variant(const std::string& v);

    /**
     * TODO: document
     */
    Variant(std::float_t x, std::float_t y, std::float_t z);

    /**
     * @brief Class constructor.
     *
     * @param[in] data  Byte array representing the parameter data obtained by i.e. an OCP1 Notification or Response. 
     * @param[in] type  Data type of the Ocp1CommandDefinition associated with that OCP1 Notification or Response.
     */
    Variant(const std::vector<std::uint8_t>& data, Ocp1DataType type = OCP1DATATYPE_BLOB);

    /**
     * TODO: document
     */
    virtual ~Variant() = default;

    /**
     * TODO: document
     */
    bool operator==(const Variant& other) const;

    /**
     * TODO: document
     */
    bool operator!=(const Variant& other) const;

    /**
     * @brief Check if this Variant has a valid value and type, i.e. different than the default TypeNone (std::monostate).
     * 
     * @return True if this Variant is valid.
     */
    bool IsValid() const;

    /**
     * TODO: document
     */
    std::vector<std::uint8_t> ToParamData(Ocp1DataType type = OCP1DATATYPE_BLOB) const;

    /**
     * TODO: document
     */
    bool ToBool() const;

    /**
     * TODO: document
     */
    std::int32_t ToInt32() const;

    /**
     * TODO: document
     */
    std::uint64_t ToUInt64() const;

    /**
     * TODO: document
     */
    double ToDouble() const;

    /**
     * TODO: document
     */
    std::float_t ToFloat() const;

    /**
     * TODO: document
     */
    std::string ToString() const;

    /**
     * @brief Convenience helper method to extract x, y, and z float values from a Variant.
     * @details The Variant should internally contain the values as 3 x 4 bytes.
     * 
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained x, y, and z values.
     */
    std::array<std::float_t, 3> ToPosition(bool* pOk = nullptr) const;

    /**
     * @brief Convenience helper method to extract x, y, z, horizontal angle, vertical angle
     * and rotation angle float values from a Variant.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained x, y, z, hor, ver, and rot values.
     */
    std::array<std::float_t, 6> ToPositionAndRotation(bool* pOk = nullptr) const;

    /**
     * @brief Convenience helper method to extract a std::vector<bool> from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaBoolean>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>. 
     */
    std::vector<bool> ToBoolVector(bool* pOk = nullptr) const;

    /**
     * @brief Convenience helper method to extract a juce::StringArray from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaString>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>.
     */
    juce::StringArray ToStringArray(bool* pOk = nullptr) const;


protected:
    /**
     * TODO: document
     */
    std::vector<std::uint8_t> ToByteVector() const;

    /**
     * TODO: document
     */
    enum TypeIndex
    {
        TypeNone = 0,
        TypeBool,
        TypeInt32,
        TypeUInt64,
        TypeDouble,
        TypeString,
        TypeByteVector
    };

    /**
     * TODO: document
     */
    using VariantType = std::variant<std::monostate,                // TypeNone
                                     bool,                          // TypeBool
                                     std::int32_t,                  // TypeInt32
                                     std::uint64_t,                 // TypeUInt64
                                     double,                        // TypeDouble
                                     std::string,                   // TypeString
                                     std::vector<std::uint8_t>>;    // TypeByteVector

private:
    /**
     * TODO: document
     */
    VariantType m_value;
};

}