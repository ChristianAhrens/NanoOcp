#include <gtest/gtest.h>

#include "Variant.h"

using namespace NanoOcp1;

//==============================================================================
// Construction / native type
//==============================================================================

TEST(VariantTest, DefaultConstructedIsInvalid)
{
    Variant v;
    EXPECT_FALSE(v.IsValid());
    EXPECT_EQ(v.GetDataType(), OCP1DATATYPE_NONE);
}

TEST(VariantTest, NativeTypesAreReportedCorrectly)
{
    EXPECT_EQ(Variant(true).GetDataType(), OCP1DATATYPE_BOOLEAN);
    EXPECT_EQ(Variant(std::int32_t(-1)).GetDataType(), OCP1DATATYPE_INT32);
    EXPECT_EQ(Variant(std::uint8_t(1)).GetDataType(), OCP1DATATYPE_UINT8);
    EXPECT_EQ(Variant(std::uint16_t(1)).GetDataType(), OCP1DATATYPE_UINT16);
    EXPECT_EQ(Variant(std::uint32_t(1)).GetDataType(), OCP1DATATYPE_UINT32);
    EXPECT_EQ(Variant(std::uint64_t(1)).GetDataType(), OCP1DATATYPE_UINT64);
    EXPECT_EQ(Variant(std::float_t(1.0f)).GetDataType(), OCP1DATATYPE_FLOAT32);
    EXPECT_EQ(Variant(std::double_t(1.0)).GetDataType(), OCP1DATATYPE_FLOAT64);
    EXPECT_EQ(Variant(std::string("abc")).GetDataType(), OCP1DATATYPE_STRING);
    EXPECT_EQ(Variant("abc").GetDataType(), OCP1DATATYPE_STRING);
    EXPECT_EQ(Variant(1.0f, 2.0f, 3.0f).GetDataType(), OCP1DATATYPE_BLOB);

    for (const auto& v : { Variant(true), Variant(std::int32_t(1)), Variant("x") })
        EXPECT_TRUE(v.IsValid());
}

TEST(VariantTest, EqualityComparesTypeAndValue)
{
    EXPECT_EQ(Variant(std::int32_t(5)), Variant(std::int32_t(5)));
    EXPECT_NE(Variant(std::int32_t(5)), Variant(std::int32_t(6)));
    // Same numeric value but different held type must not compare equal
    // (std::variant equality requires matching alternative index).
    EXPECT_NE(Variant(std::uint8_t(5)), Variant(std::int32_t(5)));
}

//==============================================================================
// Cross-type ToX() conversions
//==============================================================================

TEST(VariantTest, ToBoolFromNumericTypes)
{
    bool ok = false;
    EXPECT_TRUE(Variant(std::int32_t(1)).ToBool(&ok));
    EXPECT_TRUE(ok);
    EXPECT_FALSE(Variant(std::int32_t(0)).ToBool(&ok));
    EXPECT_TRUE(ok);
    EXPECT_TRUE(Variant(std::float_t(0.5f)).ToBool());
}

TEST(VariantTest, ToInt32FromFloatRounds)
{
    bool ok = false;
    EXPECT_EQ(Variant(std::float_t(2.6f)).ToInt32(&ok), 3);
    EXPECT_TRUE(ok);
}

TEST(VariantTest, ToUInt8FromStringParsesOrFails)
{
    bool ok = false;
    EXPECT_EQ(Variant(std::string("42")).ToUInt8(&ok), 42);
    EXPECT_TRUE(ok);

    ok = true;
    Variant(std::string("not a number")).ToUInt8(&ok);
    EXPECT_FALSE(ok);
}

TEST(VariantTest, ToStringConvertsNumericTypesAndFailsOnInvalidVariant)
{
    bool ok = false;
    EXPECT_EQ(Variant(std::string("hello")).ToString(&ok), "hello");
    EXPECT_TRUE(ok);

    // Numeric types are converted via std::to_string, not rejected.
    ok = false;
    EXPECT_EQ(Variant(std::uint32_t(42)).ToString(&ok), "42");
    EXPECT_TRUE(ok);

    // Only a default-constructed (typeless) Variant fails to convert.
    ok = true;
    Variant().ToString(&ok);
    EXPECT_FALSE(ok);
}

TEST(VariantTest, ToFloatFromUInt32)
{
    bool ok = false;
    EXPECT_FLOAT_EQ(Variant(std::uint32_t(7)).ToFloat(&ok), 7.0f);
    EXPECT_TRUE(ok);
}

TEST(VariantTest, ToDoubleFromBool)
{
    bool ok = false;
    EXPECT_DOUBLE_EQ(Variant(true).ToDouble(&ok), 1.0);
    EXPECT_TRUE(ok);
}

//==============================================================================
// ToParamData() marshaling round trip
//==============================================================================

TEST(VariantTest, ToParamDataUsesNativeTypeByDefault)
{
    bool ok = false;
    EXPECT_EQ(Variant(std::uint32_t(0x11223344)).ToParamData(OCP1DATATYPE_NONE, &ok),
              (ByteVector{ 0x11, 0x22, 0x33, 0x44 }));
    EXPECT_TRUE(ok);
}

TEST(VariantTest, ToParamDataConvertsToRequestedType)
{
    bool ok = false;
    // A float-valued Variant marshaled explicitly as UINT8.
    auto data = Variant(std::float_t(5.0f)).ToParamData(OCP1DATATYPE_UINT8, &ok);
    EXPECT_TRUE(ok);
    EXPECT_EQ(data, (ByteVector{ 0x05 }));
}

TEST(VariantTest, ToParamDataUnimplementedTypeFails)
{
    bool ok = true;
    Variant(std::uint32_t(1)).ToParamData(OCP1DATATYPE_BIT_STRING, &ok);
    EXPECT_FALSE(ok);
}

//==============================================================================
// Unmarshaling constructor
//==============================================================================

TEST(VariantTest, UnmarshalBoolean)
{
    Variant v(ByteVector{ 0x01 }, OCP1DATATYPE_BOOLEAN);
    EXPECT_TRUE(v.IsValid());
    EXPECT_EQ(v.GetDataType(), OCP1DATATYPE_BOOLEAN);
    EXPECT_TRUE(v.ToBool());
}

TEST(VariantTest, UnmarshalInt32)
{
    Variant v(DataFromInt32(-99), OCP1DATATYPE_INT32);
    EXPECT_EQ(v.ToInt32(), -99);
}

TEST(VariantTest, UnmarshalUint8)
{
    Variant v(ByteVector{ 0x2A }, OCP1DATATYPE_UINT8);
    EXPECT_EQ(v.ToUInt8(), 0x2A);
}

TEST(VariantTest, UnmarshalUint16)
{
    Variant v(DataFromUint16(0xBEEF), OCP1DATATYPE_UINT16);
    EXPECT_EQ(v.ToUInt16(), 0xBEEF);
}

TEST(VariantTest, UnmarshalUint32)
{
    Variant v(DataFromUint32(0xDEADBEEF), OCP1DATATYPE_UINT32);
    EXPECT_EQ(v.ToUInt32(), 0xDEADBEEFu);
}

TEST(VariantTest, UnmarshalUint64)
{
    Variant v(DataFromUint64(0x0102030405060708ULL), OCP1DATATYPE_UINT64);
    EXPECT_EQ(v.ToUInt64(), 0x0102030405060708ULL);
}

TEST(VariantTest, UnmarshalFloat32)
{
    Variant v(DataFromFloat(-2.5f), OCP1DATATYPE_FLOAT32);
    EXPECT_FLOAT_EQ(v.ToFloat(), -2.5f);
}

TEST(VariantTest, UnmarshalFloat64)
{
    Variant v(DataFromDouble(-2.5), OCP1DATATYPE_FLOAT64);
    EXPECT_DOUBLE_EQ(v.ToDouble(), -2.5);
}

TEST(VariantTest, UnmarshalString)
{
    Variant v(DataFromString("abc"), OCP1DATATYPE_STRING);
    EXPECT_EQ(v.ToString(), "abc");
}

TEST(VariantTest, UnmarshalBlobKeepsRawBytes)
{
    ByteVector blob{ 0x00, 0x03, 0x01, 0x02, 0x03 };
    Variant v(blob, OCP1DATATYPE_BLOB);
    EXPECT_EQ(v.GetDataType(), OCP1DATATYPE_BLOB);
    EXPECT_EQ(v.ToParamData(), blob);
}

//==============================================================================
// Position / AimingAndPosition
//==============================================================================

TEST(VariantTest, ConstructAndDecodePosition)
{
    Variant v(0.5f, 0.25f, -1.0f);

    bool ok = false;
    auto pos = v.ToPosition(&ok);
    EXPECT_TRUE(ok);
    EXPECT_FLOAT_EQ(pos[0], 0.5f);
    EXPECT_FLOAT_EQ(pos[1], 0.25f);
    EXPECT_FLOAT_EQ(pos[2], -1.0f);
}

TEST(VariantTest, PositionStringFormat)
{
    Variant v(1.0f, 2.0f, 3.0f);
    bool ok = false;
    auto str = v.ToPositionString(&ok);
    EXPECT_TRUE(ok);
    EXPECT_NE(str.find("1"), std::string::npos);
    EXPECT_NE(str.find("2"), std::string::npos);
    EXPECT_NE(str.find("3"), std::string::npos);
}

TEST(VariantTest, DecodeAimingAndPosition)
{
    ByteVector data = DataFromAimingAndPosition(10.0f, 20.0f, 30.0f, 1.0f, 2.0f, 3.0f);
    Variant v(data, OCP1DATATYPE_BLOB);

    bool ok = false;
    auto result = v.ToAimingAndPosition(&ok);
    EXPECT_TRUE(ok);
    EXPECT_FLOAT_EQ(result[0], 10.0f); // hor
    EXPECT_FLOAT_EQ(result[1], 20.0f); // ver
    EXPECT_FLOAT_EQ(result[2], 30.0f); // rot
    EXPECT_FLOAT_EQ(result[3], 1.0f);  // x
    EXPECT_FLOAT_EQ(result[4], 2.0f);  // y
    EXPECT_FLOAT_EQ(result[5], 3.0f);  // z
}

TEST(VariantTest, PositionOnNonBlobVariantFails)
{
    bool ok = true;
    Variant(std::uint32_t(1)).ToPosition(&ok);
    EXPECT_FALSE(ok);
}

//==============================================================================
// OcaList<OcaBoolean> / OcaList<OcaString>
//==============================================================================

TEST(VariantTest, DecodeBoolVector)
{
    // OcaList layout: 2-byte element count, then N single-byte booleans.
    ByteVector data = DataFromUint16(3);
    data.push_back(0x01); // true
    data.push_back(0x00); // false
    data.push_back(0x01); // true

    Variant v(data, OCP1DATATYPE_BLOB);
    bool ok = false;
    auto result = v.ToBoolVector(&ok);
    EXPECT_TRUE(ok);
    ASSERT_EQ(result.size(), 3u);
    EXPECT_TRUE(result[0]);
    EXPECT_FALSE(result[1]);
    EXPECT_TRUE(result[2]);
}

TEST(VariantTest, DecodeEmptyBoolVector)
{
    ByteVector data = DataFromUint16(0);
    Variant v(data, OCP1DATATYPE_BLOB);
    bool ok = false;
    auto result = v.ToBoolVector(&ok);
    EXPECT_TRUE(ok);
    EXPECT_TRUE(result.empty());
}

TEST(VariantTest, DecodeStringVector)
{
    // OcaList layout: 2-byte element count, then N (2-byte length + bytes) strings.
    ByteVector data = DataFromUint16(2);
    ByteVector s1 = DataFromString("ab");
    ByteVector s2 = DataFromString("xyz");
    data.insert(data.end(), s1.begin(), s1.end());
    data.insert(data.end(), s2.begin(), s2.end());

    Variant v(data, OCP1DATATYPE_BLOB);
    bool ok = false;
    auto result = v.ToStringVector(&ok);
    EXPECT_TRUE(ok);
    ASSERT_EQ(result.size(), 2u);
    EXPECT_EQ(result[0], "ab");
    EXPECT_EQ(result[1], "xyz");
}

TEST(VariantTest, StringVectorOnNonBlobVariantFails)
{
    bool ok = true;
    Variant(std::uint32_t(1)).ToStringVector(&ok);
    EXPECT_FALSE(ok);
}
