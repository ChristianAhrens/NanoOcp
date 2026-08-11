#include <gtest/gtest.h>

#include "Ocp1ObjectDefinitions.h"
#include "Ocp1DS100ObjectDefinitions.h"

using namespace NanoOcp1;

//==============================================================================
// Table-driven identity check for every dbOcaObjectDef_* struct in
// Ocp1ObjectDefinitions.h and Ocp1DS100ObjectDefinitions.h.
//
// Each struct only differs from its base Ocp1CommandDefinition in the four
// values wired through its constructor's initializer list (target ONo,
// property type, property def-level, property index). With ~90 structs that
// are all shaped identically, a data table catches copy-paste mistakes (e.g.
// wrong BoxAndObjNo constant, wrong def-level) far more legibly than 90
// near-duplicate TEST() bodies.
//==============================================================================

namespace
{

// Representative non-zero arguments so that record/channel wiring bugs
// (e.g. swapped constructor parameters) are also caught.
constexpr std::uint32_t kChannel = 3;
constexpr std::uint32_t kRecord = 2;

struct ObjectDefCase
{
    std::string name;
    Ocp1CommandDefinition def;
    std::uint32_t expectedOno;
    Ocp1DataType expectedType;
    std::uint16_t expectedDefLevel;
    std::uint16_t expectedPropIdx;
};

std::vector<ObjectDefCase> MakeObjectDefCases()
{
    std::vector<ObjectDefCase> cases;

    // ── Ocp1ObjectDefinitions.h : AmpGeneric ──────────────────────────────
    cases.push_back({ "AmpGeneric_Config_PotiLevel",
        AmpGeneric::dbOcaObjectDef_Config_PotiLevel(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpGeneric::Config_PotiLevel),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "AmpGeneric_Config_Mute",
        AmpGeneric::dbOcaObjectDef_Config_Mute(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpGeneric::Config_Mute),
        OCP1DATATYPE_UINT8, DefLevel_OcaMute, 1 });

    // ── Ocp1ObjectDefinitions.h : AmpDxDy ─────────────────────────────────
    cases.push_back({ "AmpDxDy_Settings_PwrOn",
        AmpDxDy::dbOcaObjectDef_Settings_PwrOn(),
        GetONo(0x01, 0x00, 0x00, AmpDxDy::Settings_PwrOn),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "AmpDxDy_ChStatus_Isp",
        AmpDxDy::dbOcaObjectDef_ChStatus_Isp(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpDxDy::ChStatus_Isp),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "AmpDxDy_ChStatus_Gr",
        AmpDxDy::dbOcaObjectDef_ChStatus_Gr(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpDxDy::ChStatus_Gr),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "AmpDxDy_ChStatus_Ovl",
        AmpDxDy::dbOcaObjectDef_ChStatus_Ovl(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpDxDy::ChStatus_Ovl),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    // ── Ocp1ObjectDefinitions.h : AmpDx / AmpDy ───────────────────────────
    cases.push_back({ "AmpDx_ChStatus_GrHead",
        AmpDx::dbOcaObjectDef_ChStatus_GrHead(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpDx::ChStatus_GrHead),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaAudioLevelSensor, 1 });

    cases.push_back({ "AmpDy_ChStatus_GrHead",
        AmpDy::dbOcaObjectDef_ChStatus_GrHead(kChannel),
        GetONo(0x01, 0x00, kChannel, AmpDy::ChStatus_GrHead),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaAudioLevelSensor, 1 });

    // ── Ocp1ObjectDefinitions.h : Amp5D ───────────────────────────────────
    cases.push_back({ "Amp5D_Settings_PwrOn",
        Amp5D::dbOcaObjectDef_Settings_PwrOn(),
        GetONo(0x01, 0x00, 0x00, Amp5D::Settings_PwrOn),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "Amp5D_ChStatus_Isp",
        Amp5D::dbOcaObjectDef_ChStatus_Isp(kChannel),
        GetONo(0x01, 0x00, kChannel, Amp5D::ChStatus_Isp),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "Amp5D_ChStatus_Gr",
        Amp5D::dbOcaObjectDef_ChStatus_Gr(kChannel),
        GetONo(0x01, 0x00, kChannel, Amp5D::ChStatus_Gr),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "Amp5D_ChStatus_Ovl",
        Amp5D::dbOcaObjectDef_ChStatus_Ovl(kChannel),
        GetONo(0x01, 0x00, kChannel, Amp5D::ChStatus_Ovl),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "Amp5D_ChStatus_GrHead",
        Amp5D::dbOcaObjectDef_ChStatus_GrHead(kChannel),
        GetONo(0x01, 0x00, kChannel, Amp5D::ChStatus_GrHead),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaAudioLevelSensor, 1 });

    // ── Ocp1DS100ObjectDefinitions.h : device identity / settings / status ──
    using namespace DS100;

    cases.push_back({ "DS100_Fixed_HardwareVariant",
        dbOcaObjectDef_Fixed_HardwareVariant(),
        GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_HardwareVariant),
        OCP1DATATYPE_INT32, DefLevel_OcaInt32Sensor, 1 });

    cases.push_back({ "DS100_Fixed_SerNr",
        dbOcaObjectDef_Fixed_SerNr(),
        GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_SerNr),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_Fixed_GUID",
        dbOcaObjectDef_Fixed_GUID(),
        GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_GUID),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_Settings_DeviceName",
        dbOcaObjectDef_Settings_DeviceName(),
        GetONoTy2(0x02, 0x00, 0x00, Settings_Box, Settings_DeviceName),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_Status_StatusText",
        dbOcaObjectDef_Status_StatusText(),
        GetONoTy2(0x02, 0x00, 0x00, Status_Box, Status_StatusText),
        OCP1DATATYPE_STRING, DefLevel_OcaStringSensor, 1 });

    cases.push_back({ "DS100_Status_AudioNetworkSampleStatus",
        dbOcaObjectDef_Status_AudioNetworkSampleStatus(),
        GetONoTy2(0x02, 0x00, 0x00, Status_Box, Status_AudioNetworkSampleStatus),
        OCP1DATATYPE_INT32, DefLevel_OcaInt32Sensor, 1 });

    cases.push_back({ "DS100_Error_GnrlErr",
        dbOcaObjectDef_Error_GnrlErr(),
        GetONoTy2(0x02, 0x00, 0x00, Error_Box, Error_GnrlErr),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "DS100_Error_ErrorText",
        dbOcaObjectDef_Error_ErrorText(),
        GetONoTy2(0x02, 0x00, 0x00, Error_Box, Error_ErrorText),
        OCP1DATATYPE_STRING, DefLevel_OcaStringSensor, 1 });

    // ── DS100 : Coordinate mapping settings (per mapping-area record) ───────
    cases.push_back({ "DS100_CoordinateMappingSettings_Name",
        dbOcaObjectDef_CoordinateMappingSettings_Name(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Name),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_Type",
        dbOcaObjectDef_CoordinateMappingSettings_Type(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Type),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_Flip",
        dbOcaObjectDef_CoordinateMappingSettings_Flip(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Flip),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P1_real",
        dbOcaObjectDef_CoordinateMappingSettings_P1_real(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P1_real),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P2_real",
        dbOcaObjectDef_CoordinateMappingSettings_P2_real(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P2_real),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P3_real",
        dbOcaObjectDef_CoordinateMappingSettings_P3_real(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P3_real),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P4_real",
        dbOcaObjectDef_CoordinateMappingSettings_P4_real(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P4_real),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P1_virtual",
        dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P1_virtual),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMappingSettings_P3_virtual",
        dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(kRecord),
        GetONoTy2(0x02, kRecord, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P3_virtual),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_CoordinateMapping_Source_Position",
        dbOcaObjectDef_CoordinateMapping_Source_Position(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, CoordinateMapping_Box, CoordinateMapping_Source_Position),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    // ── DS100 : En-Scene source positioning ──────────────────────────────
    cases.push_back({ "DS100_Positioning_Source_Position",
        dbOcaObjectDef_Positioning_Source_Position(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_Position),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_Positioning_Source_Enable",
        dbOcaObjectDef_Positioning_Source_Enable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_Enable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_Positioning_Source_Spread",
        dbOcaObjectDef_Positioning_Source_Spread(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_Spread),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaFloat32Actuator, 1 });

    cases.push_back({ "DS100_Positioning_Source_Speaker_Group",
        dbOcaObjectDef_Positioning_Source_Speaker_Group(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_Speaker_Group),
        OCP1DATATYPE_INT32, DefLevel_OcaInt32Actuator, 1 });

    cases.push_back({ "DS100_Positioning_Source_Speaker_Position",
        dbOcaObjectDef_Positioning_Source_Speaker_Position(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_Speaker_Position),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaSpeakerPositionAgentDeprecated, 1 });

    cases.push_back({ "DS100_Positioning_Source_DelayMode",
        dbOcaObjectDef_Positioning_Source_DelayMode(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Source_Box, Positioning_Source_DelayMode),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    // ── DS100 : Function groups ──────────────────────────────────────────
    cases.push_back({ "DS100_FunctionGroup_Name",
        dbOcaObjectDef_FunctionGroup_Name(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, FunctionGroup_Box, FunctionGroup_Name),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_FunctionGroup_Delay",
        dbOcaObjectDef_FunctionGroup_Delay(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, FunctionGroup_Box, FunctionGroup_Delay),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaDelay, 1 });

    cases.push_back({ "DS100_FunctionGroup_Mode",
        dbOcaObjectDef_FunctionGroup_Mode(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, FunctionGroup_Box, FunctionGroup_Mode),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_FunctionGroup_SpreadFactor",
        dbOcaObjectDef_FunctionGroup_SpreadFactor(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, FunctionGroup_Box, FunctionGroup_SpreadFactor),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaFloat32Actuator, 1 });

    // ── DS100 : Matrix inputs ────────────────────────────────────────────
    cases.push_back({ "DS100_MatrixInput_Mute",
        dbOcaObjectDef_MatrixInput_Mute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_Mute),
        OCP1DATATYPE_UINT8, DefLevel_OcaMute, 1 });

    cases.push_back({ "DS100_MatrixInput_Gain",
        dbOcaObjectDef_MatrixInput_Gain(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "DS100_MatrixInput_Delay",
        dbOcaObjectDef_MatrixInput_Delay(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_Delay),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaDelay, 1 });

    cases.push_back({ "DS100_MatrixInput_DelayEnable",
        dbOcaObjectDef_MatrixInput_DelayEnable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_DelayEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixInput_EqEnable",
        dbOcaObjectDef_MatrixInput_EqEnable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_EqEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixInput_Polarity",
        dbOcaObjectDef_MatrixInput_Polarity(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_Polarity),
        OCP1DATATYPE_UINT8, DefLevel_OcaPolarity, 1 });

    cases.push_back({ "DS100_MatrixInput_ChannelName",
        dbOcaObjectDef_MatrixInput_ChannelName(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_ChannelName),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_MatrixInput_LevelMeterIn",
        dbOcaObjectDef_MatrixInput_LevelMeterIn(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_LevelMeterIn),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixInput_LevelMeterPreMute",
        dbOcaObjectDef_MatrixInput_LevelMeterPreMute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_LevelMeterPreMute),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixInput_LevelMeterPostMute",
        dbOcaObjectDef_MatrixInput_LevelMeterPostMute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_LevelMeterPostMute),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixInput_ISP",
        dbOcaObjectDef_MatrixInput_ISP(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_ISP),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    cases.push_back({ "DS100_MatrixInput_ReverbSendGain",
        dbOcaObjectDef_MatrixInput_ReverbSendGain(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixInput_Box, MatrixInput_ReverbSendGain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    // ── DS100 : Matrix nodes (crosspoints) ───────────────────────────────
    cases.push_back({ "DS100_MatrixNode_Enable",
        dbOcaObjectDef_MatrixNode_Enable(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, MatrixNode_Box, MatrixNode_Enable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixNode_Gain",
        dbOcaObjectDef_MatrixNode_Gain(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, MatrixNode_Box, MatrixNode_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "DS100_MatrixNode_Delay",
        dbOcaObjectDef_MatrixNode_Delay(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, MatrixNode_Box, MatrixNode_Delay),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaDelay, 1 });

    cases.push_back({ "DS100_MatrixNode_DelayEnable",
        dbOcaObjectDef_MatrixNode_DelayEnable(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, MatrixNode_Box, MatrixNode_DelayEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    // ── DS100 : Matrix outputs ───────────────────────────────────────────
    cases.push_back({ "DS100_MatrixOutput_Mute",
        dbOcaObjectDef_MatrixOutput_Mute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_Mute),
        OCP1DATATYPE_UINT8, DefLevel_OcaMute, 1 });

    cases.push_back({ "DS100_MatrixOutput_Gain",
        dbOcaObjectDef_MatrixOutput_Gain(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "DS100_MatrixOutput_Delay",
        dbOcaObjectDef_MatrixOutput_Delay(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_Delay),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaDelay, 1 });

    cases.push_back({ "DS100_MatrixOutput_DelayEnable",
        dbOcaObjectDef_MatrixOutput_DelayEnable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_DelayEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixOutput_EqEnable",
        dbOcaObjectDef_MatrixOutput_EqEnable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_EqEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixOutput_Polarity",
        dbOcaObjectDef_MatrixOutput_Polarity(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_Polarity),
        OCP1DATATYPE_UINT8, DefLevel_OcaPolarity, 1 });

    cases.push_back({ "DS100_MatrixOutput_ChannelName",
        dbOcaObjectDef_MatrixOutput_ChannelName(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_ChannelName),
        OCP1DATATYPE_STRING, DefLevel_OcaStringActuator, 1 });

    cases.push_back({ "DS100_MatrixOutput_LevelMeterIn",
        dbOcaObjectDef_MatrixOutput_LevelMeterIn(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_LevelMeterIn),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixOutput_LevelMeterPreMute",
        dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_LevelMeterPreMute),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixOutput_LevelMeterPostMute",
        dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_LevelMeterPostMute),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    cases.push_back({ "DS100_MatrixOutput_OSP",
        dbOcaObjectDef_MatrixOutput_OSP(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, MatrixOutput_Box, MatrixOutput_OSP),
        OCP1DATATYPE_BOOLEAN, DefLevel_OcaBooleanSensor, 1 });

    // ── DS100 : Matrix / En-Space global settings ────────────────────────
    cases.push_back({ "DS100_MatrixSettings_PositioningEnable",
        dbOcaObjectDef_MatrixSettings_PositioningEnable(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_PositioningEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixSettings_ReverbEnable",
        dbOcaObjectDef_MatrixSettings_ReverbEnable(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixSettings_ReverbRoomId",
        dbOcaObjectDef_MatrixSettings_ReverbRoomId(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_MatrixSettings_ReverbRoomIdNames_Identity",
        dbOcaObjDef_MatrixSettings_ReverbRoomIdNames(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId),
        OCP1DATATYPE_BLOB, DefLevel_OcaSwitch, 2 });

    cases.push_back({ "DS100_MatrixSettings_ReverbRoomIdEnableds_Identity",
        dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId),
        OCP1DATATYPE_BLOB, DefLevel_OcaSwitch, 3 });

    cases.push_back({ "DS100_MatrixSettings_ReverbPredelayFactor",
        dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbPredelayFactor),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaFloat32Actuator, 1 });

    cases.push_back({ "DS100_MatrixSettings_ReverbRearLevel",
        dbOcaObjectDef_MatrixSettings_ReverbRearLevel(),
        GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRearLevel),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    // ── DS100 : Reverb input / processing ────────────────────────────────
    cases.push_back({ "DS100_ReverbInput_Gain",
        dbOcaObjectDef_ReverbInput_Gain(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, ReverbInput_Box, ReverbInput_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "DS100_ReverbInputProcessing_Mute",
        dbOcaObjectDef_ReverbInputProcessing_Mute(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, ReverbInputProcessing_Box, ReverbInputProcessing_Mute),
        OCP1DATATYPE_UINT8, DefLevel_OcaMute, 1 });

    cases.push_back({ "DS100_ReverbInputProcessing_Gain",
        dbOcaObjectDef_ReverbInputProcessing_Gain(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, ReverbInputProcessing_Box, ReverbInputProcessing_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    cases.push_back({ "DS100_ReverbInputProcessing_EqEnable",
        dbOcaObjectDef_ReverbInputProcessing_EqEnable(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, ReverbInputProcessing_Box, ReverbInputProcessing_EqEnable),
        OCP1DATATYPE_UINT16, DefLevel_OcaSwitch, 1 });

    cases.push_back({ "DS100_ReverbInputProcessing_LevelMeter",
        dbOcaObjectDef_ReverbInputProcessing_LevelMeter(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, ReverbInputProcessing_Box, ReverbInputProcessing_LevelMeter),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaLevelSensor, 1 });

    // ── DS100 : Scene management ─────────────────────────────────────────
    cases.push_back({ "DS100_Scene_SceneIndex",
        dbOcaObjectDef_Scene_SceneIndex(),
        GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneIndex),
        OCP1DATATYPE_STRING, DefLevel_OcaStringSensor, 1 });

    cases.push_back({ "DS100_Scene_SceneName",
        dbOcaObjectDef_Scene_SceneName(),
        GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneName),
        OCP1DATATYPE_STRING, DefLevel_OcaStringSensor, 1 });

    cases.push_back({ "DS100_Scene_SceneComment",
        dbOcaObjectDef_Scene_SceneComment(),
        GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneComment),
        OCP1DATATYPE_STRING, DefLevel_OcaStringSensor, 1 });

    // ── DS100 : Sound-object routing (per function group) ───────────────
    cases.push_back({ "DS100_SoundObjectRouting_Mute",
        dbOcaObjectDef_SoundObjectRouting_Mute(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, SoundObjectRouting_Box, SoundObjectRouting_Mute),
        OCP1DATATYPE_UINT8, DefLevel_OcaMute, 1 });

    cases.push_back({ "DS100_SoundObjectRouting_Gain",
        dbOcaObjectDef_SoundObjectRouting_Gain(kRecord, kChannel),
        GetONoTy2(0x02, kRecord, kChannel, SoundObjectRouting_Box, SoundObjectRouting_Gain),
        OCP1DATATYPE_FLOAT32, DefLevel_OcaGain, 1 });

    // ── DS100 : Loudspeaker positioning (stack 1+) ───────────────────────
    cases.push_back({ "DS100_Positioning_Speaker_Group",
        dbOcaObjectDef_Positioning_Speaker_Group(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Speaker_Box, Positioning_Speaker_Group),
        OCP1DATATYPE_INT32, DefLevel_OcaInt32Actuator, 1 });

    cases.push_back({ "DS100_Positioning_Speaker_Position",
        dbOcaObjectDef_Positioning_Speaker_Position(kChannel),
        GetONoTy2(0x02, 0x00, kChannel, Positioning_Speaker_Box, Positioning_Speaker_Position),
        OCP1DATATYPE_DB_POSITION, DefLevel_dbOcaSpeakerPositionAgentDeprecated, 1 });

    // ── DS100 : Scene agent (fixed ONo, not GetONoTy2-derived) ───────────
    cases.push_back({ "DS100_SceneAgent_Identity",
        dbOcaObjectDef_SceneAgent(),
        SceneAgentONo,
        OCP1DATATYPE_UINT32, DefLevel_dbOcaSceneAgent, 0 });

    return cases;
}

class ObjectDefinitionTest : public ::testing::TestWithParam<ObjectDefCase>
{
};

TEST_P(ObjectDefinitionTest, WiresExpectedIdentityIntoBaseCommandDefinition)
{
    const auto& c = GetParam();

    EXPECT_EQ(c.def.m_targetOno, c.expectedOno) << "target ONo mismatch for " << c.name;
    EXPECT_EQ(c.def.m_propertyType, static_cast<std::uint16_t>(c.expectedType)) << "property type mismatch for " << c.name;
    EXPECT_EQ(c.def.GetDataType(), c.expectedType) << "GetDataType() mismatch for " << c.name;
    EXPECT_EQ(c.def.m_propertyDefLevel, c.expectedDefLevel) << "def-level mismatch for " << c.name;
    EXPECT_EQ(c.def.m_propertyIndex, c.expectedPropIdx) << "property index mismatch for " << c.name;

    // None of these structs pre-populate parameters; that only happens once
    // GetValueCommand()/SetValueCommand() is called on them.
    EXPECT_EQ(c.def.m_paramCount, 0) << "unexpected paramCount for " << c.name;
    EXPECT_TRUE(c.def.m_parameterData.empty()) << "unexpected parameterData for " << c.name;
}

INSTANTIATE_TEST_SUITE_P(
    AllObjectDefinitions,
    ObjectDefinitionTest,
    ::testing::ValuesIn(MakeObjectDefCases()),
    [](const ::testing::TestParamInfo<ObjectDefCase>& info) { return info.param.name; });

} // anonymous namespace

//==============================================================================
// Structs with custom logic beyond the base Ocp1CommandDefinition constructor.
//==============================================================================

TEST(MatrixSettingsReverbRoomIdNamesTest, GetValueCommandUsesDedicatedMethodIndex)
{
    NanoOcp1::DS100::dbOcaObjDef_MatrixSettings_ReverbRoomIdNames def;
    auto getCmd = def.GetValueCommand();

    EXPECT_EQ(getCmd.m_targetOno, def.m_targetOno);
    EXPECT_EQ(getCmd.m_propertyDefLevel, def.m_propertyDefLevel);
    EXPECT_EQ(getCmd.m_propertyIndex, 5); // GetPositionNames MethodIdx
    EXPECT_EQ(getCmd.m_paramCount, 0);
    EXPECT_TRUE(getCmd.m_parameterData.empty());
}

TEST(MatrixSettingsReverbRoomIdEnabledsTest, GetValueCommandUsesDedicatedMethodIndex)
{
    NanoOcp1::DS100::dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds def;
    auto getCmd = def.GetValueCommand();

    EXPECT_EQ(getCmd.m_targetOno, def.m_targetOno);
    EXPECT_EQ(getCmd.m_propertyDefLevel, def.m_propertyDefLevel);
    EXPECT_EQ(getCmd.m_propertyIndex, 9); // GetPositionEnableds MethodIdx
    EXPECT_EQ(getCmd.m_paramCount, 0);
    EXPECT_TRUE(getCmd.m_parameterData.empty());
}

TEST(SceneAgentTest, ApplyCommandEncodesMajorMinorIntoSingleUint32Param)
{
    NanoOcp1::DS100::dbOcaObjectDef_SceneAgent def;
    auto cmd = def.ApplyCommand(/*major*/ 1, /*minor*/ 5);

    EXPECT_EQ(cmd.m_targetOno, NanoOcp1::DS100::SceneAgentONo);
    EXPECT_EQ(cmd.m_propertyIndex, 7); // ApplyScene MethodIdx
    EXPECT_EQ(cmd.m_paramCount, 1);
    EXPECT_EQ(cmd.m_parameterData, DataFromUint32(5 + (1u << 16)));
}

TEST(SceneAgentTest, PreviousCommandUsesMethodIndexEightWithNoParams)
{
    NanoOcp1::DS100::dbOcaObjectDef_SceneAgent def;
    auto cmd = def.PreviousCommand();

    EXPECT_EQ(cmd.m_propertyIndex, 8); // PreviousScene MethodIdx
    EXPECT_EQ(cmd.m_paramCount, 0);
    EXPECT_TRUE(cmd.m_parameterData.empty());
}

TEST(SceneAgentTest, NextCommandUsesMethodIndexNineWithNoParams)
{
    NanoOcp1::DS100::dbOcaObjectDef_SceneAgent def;
    auto cmd = def.NextCommand();

    EXPECT_EQ(cmd.m_propertyIndex, 9); // NextScene MethodIdx
    EXPECT_EQ(cmd.m_paramCount, 0);
    EXPECT_TRUE(cmd.m_parameterData.empty());
}
