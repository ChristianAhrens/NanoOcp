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

#include <cstdint>          //< USE std::uint8_t, std::uint16_t, std::uint32_t, std::uint64_t in GCC-13
#include <variant>          //< USE std::variant
#include <array>            //< USE std::array
#include "Ocp1DataTypes.h"  //< USE NanoOcp1::Ocp1DataType


namespace NanoOcp1
{

/**
 * @class Variant
 * @brief Type-erased OCA parameter value with built-in marshal/unmarshal support.
 *
 * `Variant` wraps a `std::variant` holding one of the native C++ types that map to
 * AES70 base data types.  It adds two capabilities beyond a plain `std::variant`:
 *
 * - **Mutability / cross-type conversion** — A `Variant` can be constructed with one
 *   type and later read as a different primitive type via the `To<T>()` methods.
 *   Cross-type conversions between primitive types (e.g. `ToFloat()` on a `uint16_t`
 *   Variant) are supported but may involve narrowing or sign changes.  Conversions
 *   between fundamentally incompatible types (e.g. `ToString()` on a float Variant)
 *   will set `*pOk = false` and return a default value.
 *
 * - **OCP.1 marshaling / unmarshaling** — A `Variant` can be constructed directly from
 *   a raw `ByteVector` received in an `Ocp1Response` or `Ocp1Notification`, and it can
 *   be serialized back to bytes via `ToParamData()` for use in a `SetValueCommand`.
 *
 * ## Typical usage in NanoOcp
 *
 * ### Constructing a value to send (SetValue)
 * ```cpp
 * // Set sound-object position to (x=0.5, y=0.5, z=0.0):
 * NanoOcp1::Variant pos(0.5f, 0.5f, 0.0f);
 * auto def = NanoOcp1::DS100::dbOcaObjectDef_Positioning_Source_Position(5);
 * uint32_t handle;
 * auto cmd = NanoOcp1::Ocp1CommandResponseRequired(def.SetValueCommand(pos), handle);
 * client->sendData(cmd.GetSerializedData());
 * ```
 *
 * ### Decoding a received value (Notification / Response)
 * ```cpp
 * // Inside onDataReceived, after UnmarshalOcp1Message():
 * auto* notif = static_cast<NanoOcp1::Ocp1Notification*>(msg.get());
 * NanoOcp1::Variant v(notif->GetParameterData(), NanoOcp1::OCP1DATATYPE_BLOB);
 *
 * // 3D position (source):
 * auto [x, y, z] = v.ToPosition();
 *
 * // 6-DOF loudspeaker position:
 * auto [hor, vert, rot, x, y, z] = v.ToAimingAndPosition();
 *
 * // Simple scalar:
 * bool ok;
 * float gain = v.ToFloat(&ok);
 * if (!ok) { handleError(); }
 * ```
 *
 * ### DeviceController (Umsci) pattern
 * `DeviceController::UpdateObjectValue()` uses the unmarshaling constructor to decode
 * incoming `Ocp1Notification` and `Ocp1Response` parameter data, using the
 * `Ocp1DataType` stored in the matching `Ocp1CommandDefinition` to guide deserialization.
 * The resulting `Variant` is stored in a `RemoteObject` and delivered to
 * `onRemoteObjectReceived` on the application's callback thread.
 *
 * ## Supported types
 * | Constructor | Internal TypeIndex | `GetDataType()` returns |
 * |---|---|---|
 * | `Variant(bool)` | TypeBool | OCP1DATATYPE_BOOLEAN |
 * | `Variant(int32_t)` | TypeInt32 | OCP1DATATYPE_INT32 |
 * | `Variant(uint8_t)` | TypeUInt8 | OCP1DATATYPE_UINT8 |
 * | `Variant(uint16_t)` | TypeUInt16 | OCP1DATATYPE_UINT16 |
 * | `Variant(uint32_t)` | TypeUInt32 | OCP1DATATYPE_UINT32 |
 * | `Variant(uint64_t)` | TypeUInt64 | OCP1DATATYPE_UINT64 |
 * | `Variant(float_t)` | TypeFloat | OCP1DATATYPE_FLOAT32 |
 * | `Variant(double_t)` | TypeDouble | OCP1DATATYPE_FLOAT64 |
 * | `Variant(string)` | TypeString | OCP1DATATYPE_STRING |
 * | `Variant(float x, float y, float z)` | TypeByteVector | OCP1DATATYPE_BLOB |
 * | `Variant(ByteVector, type)` | TypeByteVector | OCP1DATATYPE_BLOB |
 * | `Variant()` (default) | TypeNone | OCP1DATATYPE_NONE; invalid until set |
 */
class Variant
{
public:
    /** @brief Constructs a Variant holding a boolean value. */
    Variant(bool v);
    /** @brief Constructs a Variant holding a signed 32-bit integer. */
    Variant(std::int32_t v);
    /** @brief Constructs a Variant holding an unsigned 8-bit integer. */
    Variant(std::uint8_t v);
    /** @brief Constructs a Variant holding an unsigned 16-bit integer. */
    Variant(std::uint16_t v);
    /** @brief Constructs a Variant holding an unsigned 32-bit integer. */
    Variant(std::uint32_t v);
    /** @brief Constructs a Variant holding an unsigned 64-bit integer. */
    Variant(std::uint64_t v);
    /** @brief Constructs a Variant holding a 32-bit float. */
    Variant(std::float_t v);
    /** @brief Constructs a Variant holding a 64-bit double. */
    Variant(std::double_t v);
    /** @brief Constructs a Variant holding a string value. */
    Variant(const std::string& v);
    /** @brief Constructs a Variant holding a string value (C-string overload). */
    Variant(const char* v);
    /**
     * @brief Constructs a Variant holding a 3D position as a 12-byte blob (3 × big-endian float32).
     * This is the preferred way to build a value for `SetValueCommand` on a
     * `dbOcaObjectDef_Positioning_Source_Position`.
     * @param x  Normalised X position [0.0, 1.0].
     * @param y  Normalised Y position [0.0, 1.0].
     * @param z  Normalised Z position [0.0, 1.0].
     */
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

    /** @brief Returns true if both Variants hold the same type and value. */
    bool operator==(const Variant& other) const;
    /** @brief Returns true if the Variants differ in type or value. */
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
    Ocp1DataType GetDataType() const;

    /**
     * Marshal the Variant's value into a byte-vector representation, based on the desired type.
     *
     * @param[in] type  Data type to unmarshal the Varaiant as.
     *                  If this is left as the default (NONE), the Variant's native type will be used.
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     */
    std::vector<std::uint8_t> ToParamData(Ocp1DataType type = OCP1DATATYPE_NONE, bool* pOk = nullptr) const;

    /**
     * @name Primitive type conversions
     * @brief Extract the Variant's value as a specific primitive C++ type.
     *
     * All methods accept an optional `pOk` output parameter.  If provided, it is set
     * to `true` on success and `false` if the conversion is not possible (e.g. the
     * internal type is incompatible with the requested type).  On failure a default
     * value (0, false, or empty string) is returned.
     *
     * Cross-type numeric conversions between primitive types are generally supported
     * but may involve narrowing or signedness changes (e.g. `ToUInt8()` on a float
     * Variant truncates the fractional part and clamps to [0, 255]).
     * @{
     */

    /** @brief Returns the value as bool. Numeric types: non-zero = true. */
    bool ToBool(bool* pOk = nullptr) const;
    /** @brief Returns the value as int32_t. Numeric widening/narrowing applied as needed. */
    std::int32_t ToInt32(bool* pOk = nullptr) const;
    /** @brief Returns the value as uint8_t. Values outside [0, 255] are clamped/truncated. */
    std::uint8_t ToUInt8(bool* pOk = nullptr) const;
    /** @brief Returns the value as uint16_t. */
    std::uint16_t ToUInt16(bool* pOk = nullptr) const;
    /** @brief Returns the value as uint32_t. */
    std::uint32_t ToUInt32(bool* pOk = nullptr) const;
    /** @brief Returns the value as uint64_t. */
    std::uint64_t ToUInt64(bool* pOk = nullptr) const;
    /** @brief Returns the value as float_t (32-bit). Conversion from double loses precision. */
    std::float_t ToFloat(bool* pOk = nullptr) const;
    /** @brief Returns the value as double_t (64-bit). */
    std::double_t ToDouble(bool* pOk = nullptr) const;
    /**
     * @brief Returns the value as a std::string.
     * Only succeeds if the internal type is TypeString; numeric types are not auto-converted.
     * For a human-readable representation of numeric types, use `ToPositionString()` etc.
     */
    std::string ToString(bool* pOk = nullptr) const;

    /** @} */

    /**
     * Convenience helper method to extract x, y, and z float values from a Variant.
     * The Variant should internally contain the values as 3 x 4 bytes.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained x, y, and z values.
     */
    std::array<std::float_t, 3> ToPosition(bool* pOk = nullptr) const;

    /**
     * Calls ToPosition and returns a human-readable string with the result.
     *
     * @param[in] pOk   Optional parameter to verify if the ToPosition call was successful.
     * @return  A string in the format "x, y, z".
     */
    std::string ToPositionString(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract x, y, z, horizontal angle (yaw),
     * vertical angle (pitch) and rotation angle (roll) float values from a Variant.
     * @note The aiming angles are unmarshaled first and the position second, to keep in line
     *       with the CdbOcaAimingAndPosition::Unmarshal method.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The contained values in the order: hor, ver, rot, x, y, z.
     */
    std::array<std::float_t, 6> ToAimingAndPosition(bool* pOk = nullptr) const;

    [[deprecated("Use ToAimingAndPosition instead, this method will be removed in the future. "
      "NOTE: The output of both methods is identical, but the new method has a more consistent name.")]]
    std::array<std::float_t, 6> ToPositionAndRotation(bool* pOk = nullptr) const;

    /**
     * Calls ToAimingAndPosition and returns a human-readable string with the result.
     *
     * @param[in] pOk   Optional parameter to verify if the ToAimingAndPosition call
     *                  was successful.
     * @return  A string in the format: "hor, ver, rot, x, y, z".
     */
    std::string ToAimingAndPositionString(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract a std::vector<bool> from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaBoolean>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>.
     */
    std::vector<bool> ToBoolVector(bool* pOk = nullptr) const;

    /**
     * Convenience helper method to extract a std::vector<std::string> from a from a Variant.
     * The Variant's contents need to be marshalled as an OcaList<OcaString>.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The resulting OcaList<OcaBoolean> as a std::vector<bool>.
     */
    std::vector<std::string> ToStringVector(bool* pOk = nullptr) const;


protected:
    /**
     * Marshals the Variant into a byte vector using a format based on the Variant's native type.
     *
     * @param[in] pOk   Optional parameter to verify if the conversion was successful.
     * @return  The Variant's byte vector representation.
     */
    std::vector<std::uint8_t> ToByteVector(bool* pOk = nullptr) const;

    /**
     * @brief Compact index enum for the internal `std::variant` alternative types.
     *
     * Mirrors `Ocp1DataType` but is contiguous (no gaps) as required by `std::variant`.
     * `GetDataType()` maps from `TypeIndex` back to the corresponding `Ocp1DataType`.
     * Used internally by `ToByteVector()` and `ToParamData()` to select the correct
     * serialization path.
     */
    enum TypeIndex
    {
        TypeNone = 0,       ///< Default / unset state (std::monostate). IsValid() returns false.
        TypeBool,           ///< bool
        TypeInt32,          ///< std::int32_t
        TypeUInt8,          ///< std::uint8_t
        TypeUInt16,         ///< std::uint16_t
        TypeUInt32,         ///< std::uint32_t
        TypeUInt64,         ///< std::uint64_t
        TypeFloat,          ///< std::float_t  (32-bit IEEE 754)
        TypeDouble,         ///< std::double_t (64-bit IEEE 754)
        TypeString,         ///< std::string   (OCA string: 2-byte length prefix + UTF-8 bytes)
        TypeByteVector      ///< std::vector<uint8_t> — used for blobs and multi-float spatial types
    };

    /**
     * @brief The underlying `std::variant` that holds the actual value.
     *
     * The active alternative is identified by the `TypeIndex` enum.
     * `std::monostate` (TypeNone) is the default; constructing a `Variant()` with no
     * arguments leaves it in this state.  `IsValid()` returns false in that state.
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
    VariantType m_value; ///< The stored value; active alternative determined by TypeIndex.
};

}
