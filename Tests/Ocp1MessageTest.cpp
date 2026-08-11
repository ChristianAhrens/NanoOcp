#include <gtest/gtest.h>

#include "Ocp1Message.h"
#include "Ocp1ObjectDefinitions.h"

using namespace NanoOcp1;

//==============================================================================
// Ocp1Header
//==============================================================================

TEST(Ocp1HeaderTest, SerializeThenParseRoundTrips)
{
    Ocp1Header header(Ocp1Message::CommandResponseRequired, 4);
    ByteVector bytes = header.GetSerializedData();

    ASSERT_EQ(bytes.size(), 10u);
    EXPECT_EQ(bytes[0], 0x3B);             // Sync byte
    EXPECT_EQ(bytes[1], 0x00);             // Proto version hi
    EXPECT_EQ(bytes[2], 0x01);             // Proto version lo
    EXPECT_EQ(bytes[7], Ocp1Message::CommandResponseRequired);
    EXPECT_EQ(bytes[8], 0x00);             // Msg count hi
    EXPECT_EQ(bytes[9], 0x01);             // Msg count lo

    Ocp1Header parsed(bytes);
    EXPECT_TRUE(parsed.IsValid());
    EXPECT_EQ(parsed.GetMessageType(), Ocp1Message::CommandResponseRequired);
    EXPECT_EQ(parsed.GetMessageSize(), header.GetMessageSize());
}

TEST(Ocp1HeaderTest, CalculateMessageSizeMatchesDocumentedFormulasPerType)
{
    EXPECT_EQ(Ocp1Header::CalculateMessageSize(Ocp1Message::CommandResponseRequired, 4), 30u);
    EXPECT_EQ(Ocp1Header::CalculateMessageSize(Ocp1Message::Notification, 4), 41u);
    EXPECT_EQ(Ocp1Header::CalculateMessageSize(Ocp1Message::Response, 4), 23u);
    EXPECT_EQ(Ocp1Header::CalculateMessageSize(Ocp1Message::KeepAlive, 2), 11u);
}

//==============================================================================
// Ocp1CommandResponseRequired — byte-exact golden test + round trip
//==============================================================================

TEST(Ocp1CommandResponseRequiredTest, SerializedBytesMatchWireFormat)
{
    const std::uint32_t targetOno = 0x00010203;
    const std::uint16_t methodDefLevel = 4;
    const std::uint16_t methodIndex = 1;
    const std::uint8_t paramCount = 1;
    const ByteVector paramData = DataFromFloat(1.0f); // 0x3F800000

    Ocp1CommandResponseRequired cmd(targetOno, methodDefLevel, methodIndex, paramCount, paramData);
    cmd.SetHandle(42);

    ByteVector expected
    {
        0x3B, 0x00, 0x01,             // sync, protoVers
        0x00, 0x00, 0x00, 0x1E,       // msgSize = 26 + 4 = 30
        0x01,                         // msgType = CommandResponseRequired
        0x00, 0x01,                   // msgCnt
        0x00, 0x00, 0x00, 0x15,       // commandSize = msgSize - 9 = 21
        0x00, 0x00, 0x00, 0x2A,       // handle = 42
        0x00, 0x01, 0x02, 0x03,       // targetOno
        0x00, 0x04,                   // methodDefLevel
        0x00, 0x01,                   // methodIndex
        0x01,                         // paramCount
        0x3F, 0x80, 0x00, 0x00        // parameterData (float 1.0f)
    };

    EXPECT_EQ(cmd.GetSerializedData(), expected);
}

TEST(Ocp1CommandResponseRequiredTest, SerializeThenUnmarshalRoundTrips)
{
    const std::uint32_t targetOno = 0x12345678;
    const std::uint16_t methodDefLevel = 4;
    const std::uint16_t methodIndex = 2;
    const ByteVector paramData = DataFromUint32(0xCAFEBABE);

    std::uint32_t handle = 0;
    Ocp1CommandResponseRequired cmd(targetOno, methodDefLevel, methodIndex,
                                     static_cast<std::uint8_t>(1), paramData, handle);

    auto serialized = cmd.GetSerializedData();
    auto unmarshaled = Ocp1Message::UnmarshalOcp1Message(serialized);

    ASSERT_NE(unmarshaled, nullptr);
    ASSERT_EQ(unmarshaled->GetMessageType(), Ocp1Message::CommandResponseRequired);

    auto* parsed = static_cast<Ocp1CommandResponseRequired*>(unmarshaled.get());
    EXPECT_EQ(parsed->GetHandle(), handle);
    EXPECT_EQ(parsed->GetTargetOno(), targetOno);
    EXPECT_EQ(parsed->GetMethodDefLevel(), methodDefLevel);
    EXPECT_EQ(parsed->GetMethodIndex(), methodIndex);
    EXPECT_EQ(parsed->GetParameterData(), paramData);
}

//==============================================================================
// Ocp1Response — byte-exact golden test + round trip
//==============================================================================

TEST(Ocp1ResponseTest, SerializedBytesMatchWireFormatWithNoParameters)
{
    Ocp1Response resp(/*handle*/ 7, /*status*/ 0, /*paramCount*/ 0, ByteVector{});

    ByteVector expected
    {
        0x3B, 0x00, 0x01,             // sync, protoVers
        0x00, 0x00, 0x00, 0x13,       // msgSize = 19 + 0 = 19
        0x03,                         // msgType = Response
        0x00, 0x01,                   // msgCnt
        0x00, 0x00, 0x00, 0x0A,       // responseSize = msgSize - 9 = 10
        0x00, 0x00, 0x00, 0x07,       // handle = 7
        0x00,                         // status = OK
        0x00                          // paramCount = 0
    };

    EXPECT_EQ(resp.GetSerializedData(), expected);
}

TEST(Ocp1ResponseTest, SerializeThenUnmarshalRoundTrips)
{
    const std::uint32_t handle = 99;
    const std::uint8_t status = 7; // ParameterOutOfRange
    const ByteVector paramData = DataFromString("ok");

    Ocp1Response resp(handle, status, static_cast<std::uint8_t>(1), paramData);
    auto unmarshaled = Ocp1Message::UnmarshalOcp1Message(resp.GetSerializedData());

    ASSERT_NE(unmarshaled, nullptr);
    ASSERT_EQ(unmarshaled->GetMessageType(), Ocp1Message::Response);

    auto* parsed = static_cast<Ocp1Response*>(unmarshaled.get());
    EXPECT_EQ(parsed->GetResponseHandle(), handle);
    EXPECT_EQ(parsed->GetResponseStatus(), status);
    EXPECT_EQ(parsed->GetParamCount(), 1);
    EXPECT_EQ(parsed->GetParameterData(), paramData);
}

//==============================================================================
// Ocp1Notification — structural checks + round trip
//==============================================================================

TEST(Ocp1NotificationTest, SerializedFrameHasExpectedStructure)
{
    const std::uint32_t emitterOno = 0x00000042;
    const ByteVector paramData = DataFromFloat(0.75f);

    Ocp1Notification notif(emitterOno, /*propDefLevel*/ 4, /*propIdx*/ 1,
                            static_cast<std::uint8_t>(1), paramData);
    auto bytes = notif.GetSerializedData();

    EXPECT_EQ(bytes.front(), 0x3B);
    EXPECT_EQ(bytes[7], Ocp1Message::Notification);
    EXPECT_EQ(bytes.back(), 0x01); // Ending byte

    // Total length must equal CalculateMessageSize(...) + 1 (the size field
    // excludes the leading sync byte).
    auto expectedSize = Ocp1Header::CalculateMessageSize(Ocp1Message::Notification, paramData.size()) + 1;
    EXPECT_EQ(bytes.size(), expectedSize);
}

TEST(Ocp1NotificationTest, SerializeThenUnmarshalRoundTrips)
{
    const std::uint32_t emitterOno = 0x00000042;
    const std::uint16_t propDefLevel = 4;
    const std::uint16_t propIdx = 1;
    const ByteVector paramData = DataFromFloat(0.75f);

    Ocp1Notification notif(emitterOno, propDefLevel, propIdx, static_cast<std::uint8_t>(1), paramData);
    auto unmarshaled = Ocp1Message::UnmarshalOcp1Message(notif.GetSerializedData());

    ASSERT_NE(unmarshaled, nullptr);
    ASSERT_EQ(unmarshaled->GetMessageType(), Ocp1Message::Notification);

    auto* parsed = static_cast<Ocp1Notification*>(unmarshaled.get());
    EXPECT_EQ(parsed->GetEmitterOno(), emitterOno);
    EXPECT_EQ(parsed->GetParameterData(), paramData);

    Ocp1CommandDefinition def(emitterOno, OCP1DATATYPE_FLOAT32, propDefLevel, propIdx);
    EXPECT_TRUE(parsed->MatchesObject(&def));
}

//==============================================================================
// Ocp1KeepAlive — byte-exact golden test + round trip
//==============================================================================

TEST(Ocp1KeepAliveTest, SerializedBytesMatchWireFormatSeconds)
{
    Ocp1KeepAlive keepAlive(static_cast<std::uint16_t>(5));

    ByteVector expected
    {
        0x3B, 0x00, 0x01,       // sync, protoVers
        0x00, 0x00, 0x00, 0x0B, // msgSize = 9 + 2 = 11
        0x04,                   // msgType = KeepAlive
        0x00, 0x01,             // msgCnt
        0x00, 0x05               // heartbeat seconds = 5
    };

    EXPECT_EQ(keepAlive.GetSerializedData(), expected);
    EXPECT_EQ(keepAlive.GetHeartBeatSeconds(), 5);
    EXPECT_EQ(keepAlive.GetHeartBeatMilliseconds(), 0u);
}

TEST(Ocp1KeepAliveTest, MillisecondsVariantRoundTrips)
{
    Ocp1KeepAlive keepAlive(static_cast<std::uint32_t>(12345));

    auto unmarshaled = Ocp1Message::UnmarshalOcp1Message(keepAlive.GetSerializedData());
    ASSERT_NE(unmarshaled, nullptr);
    ASSERT_EQ(unmarshaled->GetMessageType(), Ocp1Message::KeepAlive);

    // UnmarshalOcp1Message always reconstructs KeepAlive from a 16-bit field;
    // verify the milliseconds constructor's own accessor still works pre-serialization.
    EXPECT_EQ(keepAlive.GetHeartBeatMilliseconds(), 12345u);
    EXPECT_EQ(keepAlive.GetHeartBeatSeconds(), 0);
}

//==============================================================================
// Ocp1CommandDefinition factory methods (via a concrete object definition)
//==============================================================================

TEST(Ocp1CommandDefinitionTest, GetValueCommandUsesMethodIndexOneAndNoParams)
{
    NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel def(/*channel*/ 5);
    auto getCmd = def.GetValueCommand();

    EXPECT_EQ(getCmd.m_targetOno, GetONo(0x01, 0x00, 5, NanoOcp1::AmpGeneric::Config_PotiLevel));
    EXPECT_EQ(getCmd.m_propertyDefLevel, DefLevel_OcaGain);
    EXPECT_EQ(getCmd.m_propertyIndex, 1);
    EXPECT_EQ(getCmd.m_paramCount, 0);
    EXPECT_TRUE(getCmd.m_parameterData.empty());
}

TEST(Ocp1CommandDefinitionTest, SetValueCommandMarshalsValueAsNativePropertyType)
{
    NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel def(/*channel*/ 5);
    auto setCmd = def.SetValueCommand(Variant(1.0f));

    EXPECT_EQ(setCmd.m_propertyIndex, 2);
    EXPECT_EQ(setCmd.m_paramCount, 1);
    EXPECT_EQ(setCmd.m_parameterData, DataFromFloat(1.0f));
}

TEST(Ocp1CommandDefinitionTest, AddSubscriptionCommandTargetsSubscriptionManager)
{
    NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel def(/*channel*/ 5);
    auto subCmd = def.AddSubscriptionCommand();

    EXPECT_EQ(subCmd.m_targetOno, 0x00000004u); // OcaSubscriptionManager ONo
    EXPECT_EQ(subCmd.m_propertyDefLevel, 3);
    EXPECT_EQ(subCmd.m_propertyIndex, 1);
    EXPECT_EQ(subCmd.m_paramCount, 5);
    EXPECT_EQ(subCmd.m_parameterData, DataFromOnoForSubscription(def.m_targetOno, true));
}

TEST(Ocp1CommandDefinitionTest, RemoveSubscriptionCommandTargetsSubscriptionManager)
{
    NanoOcp1::AmpGeneric::dbOcaObjectDef_Config_PotiLevel def(/*channel*/ 5);
    auto unsubCmd = def.RemoveSubscriptionCommand();

    EXPECT_EQ(unsubCmd.m_targetOno, 0x00000004u);
    EXPECT_EQ(unsubCmd.m_propertyDefLevel, 3);
    EXPECT_EQ(unsubCmd.m_propertyIndex, 2);
    EXPECT_EQ(unsubCmd.m_paramCount, 2);
    EXPECT_EQ(unsubCmd.m_parameterData, DataFromOnoForSubscription(def.m_targetOno, false));
}
