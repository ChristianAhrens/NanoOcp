#include <gtest/gtest.h>

#include "Ocp1DataTypes.h"

using namespace NanoOcp1;

//==============================================================================
// Bool
//==============================================================================

TEST(Ocp1DataTypesTest, BoolRoundTrip)
{
    bool ok = false;
    EXPECT_EQ(DataFromBool(true), (ByteVector{ 0x01 }));
    EXPECT_EQ(DataFromBool(false), (ByteVector{ 0x00 }));

    EXPECT_TRUE(DataToBool(DataFromBool(true), &ok));
    EXPECT_TRUE(ok);
    EXPECT_FALSE(DataToBool(DataFromBool(false), &ok));
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, BoolInsufficientDataFails)
{
    bool ok = true;
    EXPECT_FALSE(DataToBool(ByteVector{}, &ok));
    EXPECT_FALSE(ok);
}

//==============================================================================
// Int32 / Uint8 / Uint16 / Uint32 / Uint64
//==============================================================================

TEST(Ocp1DataTypesTest, Int32RoundTripAndLayout)
{
    EXPECT_EQ(DataFromInt32(0x01020304), (ByteVector{ 0x01, 0x02, 0x03, 0x04 }));
    EXPECT_EQ(DataFromInt32(-1), (ByteVector{ 0xFF, 0xFF, 0xFF, 0xFF }));

    bool ok = false;
    EXPECT_EQ(DataToInt32(DataFromInt32(-12345), &ok), -12345);
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, Int32IgnoresTrailingExtraBytes)
{
    // Documented "HACK" in DataToInt32: uses >= instead of ==, so extra
    // trailing bytes (e.g. from an over-long response) don't fail the parse.
    ByteVector withExtra = DataFromInt32(42);
    withExtra.push_back(0xAA);
    withExtra.push_back(0xBB);

    bool ok = false;
    EXPECT_EQ(DataToInt32(withExtra, &ok), 42);
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, Int32InsufficientDataFails)
{
    bool ok = true;
    EXPECT_FALSE(DataToInt32(ByteVector{ 0x01, 0x02 }, &ok));
    EXPECT_FALSE(ok);
}

TEST(Ocp1DataTypesTest, Uint8RoundTripAndLayout)
{
    EXPECT_EQ(DataFromUint8(0xAB), (ByteVector{ 0xAB }));

    bool ok = false;
    EXPECT_EQ(DataToUint8(DataFromUint8(0x7F), &ok), 0x7F);
    EXPECT_TRUE(ok);

    ok = true;
    EXPECT_EQ(DataToUint8(ByteVector{}, &ok), 0);
    EXPECT_FALSE(ok);
}

TEST(Ocp1DataTypesTest, Uint16RoundTripAndLayout)
{
    EXPECT_EQ(DataFromUint16(0x1234), (ByteVector{ 0x12, 0x34 }));

    bool ok = false;
    EXPECT_EQ(DataToUint16(DataFromUint16(0xBEEF), &ok), 0xBEEF);
    EXPECT_TRUE(ok);

    ok = true;
    EXPECT_EQ(DataToUint16(ByteVector{ 0x01 }, &ok), 0);
    EXPECT_FALSE(ok);
}

TEST(Ocp1DataTypesTest, Uint32RoundTripAndLayout)
{
    EXPECT_EQ(DataFromUint32(0x12345678), (ByteVector{ 0x12, 0x34, 0x56, 0x78 }));

    bool ok = false;
    EXPECT_EQ(DataToUint32(DataFromUint32(0xDEADBEEF), &ok), 0xDEADBEEFu);
    EXPECT_TRUE(ok);

    ok = true;
    EXPECT_EQ(DataToUint32(ByteVector{ 0x01, 0x02, 0x03 }, &ok), 0u);
    EXPECT_FALSE(ok);
}

TEST(Ocp1DataTypesTest, Uint64RoundTripAndLayout)
{
    EXPECT_EQ(DataFromUint64(0x0102030405060708ULL),
              (ByteVector{ 0x01, 0x02, 0x03, 0x04, 0x05, 0x06, 0x07, 0x08 }));

    bool ok = false;
    EXPECT_EQ(DataToUint64(DataFromUint64(0xFEDCBA9876543210ULL), &ok), 0xFEDCBA9876543210ULL);
    EXPECT_TRUE(ok);

    ok = true;
    EXPECT_EQ(DataToUint64(ByteVector{ 0x01, 0x02 }, &ok), 0u);
    EXPECT_FALSE(ok);
}

//==============================================================================
// String
//==============================================================================

TEST(Ocp1DataTypesTest, StringRoundTripAndLayout)
{
    const std::string original = "Hello";
    ByteVector expected{ 0x00, 0x05, 'H', 'e', 'l', 'l', 'o' };
    EXPECT_EQ(DataFromString(original), expected);

    bool ok = false;
    EXPECT_EQ(DataToString(DataFromString(original), &ok), original);
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, EmptyStringRoundTrip)
{
    EXPECT_EQ(DataFromString(""), (ByteVector{ 0x00, 0x00 }));

    bool ok = false;
    EXPECT_EQ(DataToString(DataFromString(""), &ok), std::string(""));
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, StringInsufficientDataFails)
{
    bool ok = true;
    DataToString(ByteVector{ 0x00 }, &ok);
    EXPECT_FALSE(ok);
}

//==============================================================================
// Float / Double
//==============================================================================

TEST(Ocp1DataTypesTest, FloatRoundTripAndLayout)
{
    // 1.0f == 0x3F800000 in IEEE 754 single precision.
    EXPECT_EQ(DataFromFloat(1.0f), (ByteVector{ 0x3F, 0x80, 0x00, 0x00 }));

    bool ok = false;
    EXPECT_FLOAT_EQ(DataToFloat(DataFromFloat(-3.25f), &ok), -3.25f);
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, FloatInsufficientDataFails)
{
    bool ok = true;
    DataToFloat(ByteVector{ 0x01, 0x02, 0x03 }, &ok);
    EXPECT_FALSE(ok);
}

TEST(Ocp1DataTypesTest, DoubleRoundTripAndLayout)
{
    // 1.0 == 0x3FF0000000000000 in IEEE 754 double precision.
    EXPECT_EQ(DataFromDouble(1.0), (ByteVector{ 0x3F, 0xF0, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00 }));

    bool ok = false;
    EXPECT_DOUBLE_EQ(DataToDouble(DataFromDouble(-3.25), &ok), -3.25);
    EXPECT_TRUE(ok);
}

TEST(Ocp1DataTypesTest, DoubleInsufficientDataFails)
{
    bool ok = true;
    DataToDouble(ByteVector{ 0x01, 0x02, 0x03 }, &ok);
    EXPECT_FALSE(ok);
}

//==============================================================================
// Position / AimingAndPosition
//==============================================================================

TEST(Ocp1DataTypesTest, PositionLayoutIsThreeBigEndianFloats)
{
    ByteVector expected;
    ByteVector x = DataFromFloat(1.0f);
    ByteVector y = DataFromFloat(2.0f);
    ByteVector z = DataFromFloat(3.0f);
    expected.insert(expected.end(), x.begin(), x.end());
    expected.insert(expected.end(), y.begin(), y.end());
    expected.insert(expected.end(), z.begin(), z.end());

    EXPECT_EQ(DataFromPosition(1.0f, 2.0f, 3.0f), expected);
}

TEST(Ocp1DataTypesTest, AimingAndPositionOrderIsAimingFirstThenPosition)
{
    // Documented order: hor, vert, rot, x, y, z.
    ByteVector expected;
    for (float v : { 10.0f, 20.0f, 30.0f, 1.0f, 2.0f, 3.0f })
    {
        ByteVector b = DataFromFloat(v);
        expected.insert(expected.end(), b.begin(), b.end());
    }

    EXPECT_EQ(DataFromAimingAndPosition(10.0f, 20.0f, 30.0f, 1.0f, 2.0f, 3.0f), expected);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic push
#pragma GCC diagnostic ignored "-Wdeprecated-declarations"
#elif defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif

TEST(Ocp1DataTypesTest, DeprecatedPositionAndRotationMatchesAimingAndPosition)
{
    auto deprecated = DataFromPositionAndRotation(1.0f, 2.0f, 3.0f, 10.0f, 20.0f, 30.0f);
    auto current = DataFromAimingAndPosition(10.0f, 20.0f, 30.0f, 1.0f, 2.0f, 3.0f);

    EXPECT_EQ(deprecated, current);
}

#if defined(__GNUC__) || defined(__clang__)
#pragma GCC diagnostic pop
#elif defined(_MSC_VER)
#pragma warning(pop)
#endif

//==============================================================================
// Subscription parameter data
//==============================================================================

TEST(Ocp1DataTypesTest, OnoForAddSubscriptionLayout)
{
    const std::uint32_t ono = 0x00010203;
    ByteVector expected
    {
        0x00, 0x01, 0x02, 0x03, // Emitter ONo
        0x00, 0x01,             // EventID def level: OcaRoot
        0x00, 0x01,             // EventID idx: PropertyChanged
        0x00, 0x01, 0x02, 0x03, // Subscriber ONo
        0x00, 0x03,             // Method def level: OcaSubscriptionManager
        0x00, 0x01,             // Method idx: AddSubscription
        0x00, 0x00,             // Context size: 0
        0x01,                   // Delivery mode: Reliable
        0x00, 0x04,             // Destination info length
        0x00, 0x00, 0x00, 0x00  // Destination info
    };

    EXPECT_EQ(DataFromOnoForSubscription(ono, true), expected);
    EXPECT_EQ(expected.size(), 25u);
}

TEST(Ocp1DataTypesTest, OnoForRemoveSubscriptionLayout)
{
    const std::uint32_t ono = 0x00010203;
    ByteVector expected
    {
        0x00, 0x01, 0x02, 0x03, // Emitter ONo
        0x00, 0x01,             // EventID def level: OcaRoot
        0x00, 0x01,             // EventID idx: PropertyChanged
        0x00, 0x01, 0x02, 0x03, // Subscriber ONo
        0x00, 0x03,             // Method def level: OcaSubscriptionManager
        0x00, 0x01              // Method idx: AddSubscription
    };

    EXPECT_EQ(DataFromOnoForSubscription(ono, false), expected);
    EXPECT_EQ(expected.size(), 16u);
}

//==============================================================================
// String conversion helpers
//==============================================================================

TEST(Ocp1DataTypesTest, StatusToStringKnownCodes)
{
    EXPECT_EQ(StatusToString(0), "OK");
    EXPECT_EQ(StatusToString(4), "BadFormat");
    EXPECT_EQ(StatusToString(15), "PermissionDenied");
}

TEST(Ocp1DataTypesTest, StatusToStringUnknownCodeFallsBackToNumber)
{
    EXPECT_EQ(StatusToString(200), "200");
}

TEST(Ocp1DataTypesTest, DataTypeToStringKnownTypes)
{
    EXPECT_EQ(DataTypeToString(OCP1DATATYPE_BOOLEAN), "Boolean");
    EXPECT_EQ(DataTypeToString(OCP1DATATYPE_FLOAT32), "Float32");
    EXPECT_EQ(DataTypeToString(OCP1DATATYPE_BLOB), "Blob");
    EXPECT_EQ(DataTypeToString(OCP1DATATYPE_CUSTOM), "Custom");
}

TEST(Ocp1DataTypesTest, DataTypeToStringUnknownTypeIsEmpty)
{
    EXPECT_EQ(DataTypeToString(OCP1DATATYPE_NONE), "");
    EXPECT_EQ(DataTypeToString(999), "");
}

TEST(Ocp1DataTypesTest, HandleToStringSpecialAndNumeric)
{
    EXPECT_EQ(HandleToString(0), "InvalidSessionID");
    EXPECT_EQ(HandleToString(1), "LocalSessionID");
    EXPECT_EQ(HandleToString(42), "42");
}

//==============================================================================
// Buffer readers
//==============================================================================

TEST(Ocp1DataTypesTest, ReadUint32FromUint8Buffer)
{
    std::uint8_t buffer[4] = { 0x01, 0x02, 0x03, 0x04 };
    EXPECT_EQ(ReadUint32(buffer), 0x01020304u);
}

TEST(Ocp1DataTypesTest, ReadUint32FromCharBufferDoesNotSignExtend)
{
    // Regression check: buffer[i] is cast to uint8_t before shifting, so a
    // high-bit-set char (which may be negative if char is signed) must not
    // sign-extend into the assembled value.
    char buffer[4] = { char(0xFF), char(0xFF), char(0xFF), char(0xFF) };
    EXPECT_EQ(ReadUint32(buffer), 0xFFFFFFFFu);
}

TEST(Ocp1DataTypesTest, ReadUint16FromUint8Buffer)
{
    std::uint8_t buffer[2] = { 0xAB, 0xCD };
    EXPECT_EQ(ReadUint16(buffer), 0xABCDu);
}

TEST(Ocp1DataTypesTest, ReadUint16FromCharBufferDoesNotSignExtend)
{
    char buffer[2] = { char(0xFF), char(0xFF) };
    EXPECT_EQ(ReadUint16(buffer), 0xFFFFu);
}

//==============================================================================
// ONo packing
//==============================================================================

TEST(Ocp1DataTypesTest, GetONoPacksFieldsAtDocumentedBitPositions)
{
    // type<<28 | record<<20 | channel<<15 | boxAndObjectNumber
    EXPECT_EQ(GetONo(0x1, 0x00, 5, 0x206), 0x10028206u);
    EXPECT_EQ(GetONo(0x0, 0x00, 0, 0), 0u);
}

TEST(Ocp1DataTypesTest, GetONoTruncatesOutOfRangeFields)
{
    // Fields wider than their allotted bits must be masked, not overflow into
    // neighboring fields.
    EXPECT_EQ(GetONo(0xFF, 0xFF, 0xFF, 0xFFFF), GetONo(0xF, 0xFF, 0x1F, 0x7FFF));
}

TEST(Ocp1DataTypesTest, GetONoTy2PacksFieldsAtDocumentedBitPositions)
{
    // type<<28 | record<<20 | channel<<12 | boxNumber<<7 | objectNumber
    EXPECT_EQ(GetONoTy2(0x1, 0x02, 0x03, 0x04, 0x05),
              (std::uint32_t(0x1) << 28) | (std::uint32_t(0x02) << 20) |
              (std::uint32_t(0x03) << 12) | (std::uint32_t(0x04) << 7) | std::uint32_t(0x05));
}

TEST(Ocp1DataTypesTest, GetONoTy2TruncatesOutOfRangeFields)
{
    EXPECT_EQ(GetONoTy2(0xFF, 0xFF, 0xFF, 0xFF, 0xFF), GetONoTy2(0xF, 0xFF, 0xFF, 0x1F, 0x7F));
}
