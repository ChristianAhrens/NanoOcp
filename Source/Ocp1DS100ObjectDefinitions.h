/* Copyright (c) 2023, Christian Ahrens
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

#include <NanoOcp1.h>
#include <Ocp1ObjectDefinitions.h>


namespace NanoOcp1
{

//==============================================================================
// DS100
//==============================================================================
namespace DS100
{

static constexpr std::uint16_t MaxChannelCount = 64;

static constexpr BoxAndObjNo Settings_Box           = 0x01;
static constexpr BoxAndObjNo Settings_DeviceName    = 0x0d;

static constexpr BoxAndObjNo Status_Box                         = 0x03;
static constexpr BoxAndObjNo Status_StatusText                  = 0x03;
static constexpr BoxAndObjNo Status_AudioNetworkSampleStatus    = 0x30;

static constexpr BoxAndObjNo Error_Box          = 0x04;
static constexpr BoxAndObjNo Error_GnrlErr      = 0x01;
static constexpr BoxAndObjNo Error_ErrorText    = 0x03;

static constexpr BoxAndObjNo MatrixSettings_Box                     = 0x02;
static constexpr BoxAndObjNo MatrixSettings_ReverbRoomId            = 0x0a;
static constexpr BoxAndObjNo MatrixSettings_ReverbPredelayFactor    = 0x14;
static constexpr BoxAndObjNo MatrixSettings_ReverbRearLevel         = 0x15;

static constexpr BoxAndObjNo CoordinateMappingSettings_Box          = 0x15;
static constexpr BoxAndObjNo CoordinateMappingSettings_Name         = 0x01;
static constexpr BoxAndObjNo CoordinateMappingSettings_Type         = 0x02;
static constexpr BoxAndObjNo CoordinateMappingSettings_Flip         = 0x03;
static constexpr BoxAndObjNo CoordinateMappingSettings_P1_real      = 0x04;
static constexpr BoxAndObjNo CoordinateMappingSettings_P2_real      = 0x05;
static constexpr BoxAndObjNo CoordinateMappingSettings_P3_real      = 0x06;
static constexpr BoxAndObjNo CoordinateMappingSettings_P4_real      = 0x07;
static constexpr BoxAndObjNo CoordinateMappingSettings_P1_virtual   = 0x08;
static constexpr BoxAndObjNo CoordinateMappingSettings_P3_virtual   = 0x09;

static constexpr BoxAndObjNo CoordinateMapping_Box              = 0x16;
static constexpr BoxAndObjNo CoordinateMapping_Source_Position  = 0x01;

static constexpr BoxAndObjNo MatrixInput_Box                = 0x05;
static constexpr BoxAndObjNo MatrixInput_Mute               = 0x01;
static constexpr BoxAndObjNo MatrixInput_Gain               = 0x02;
static constexpr BoxAndObjNo MatrixInput_Delay              = 0x03;
static constexpr BoxAndObjNo MatrixInput_DelayEnable        = 0x04;
static constexpr BoxAndObjNo MatrixInput_EqEnable           = 0x05;
static constexpr BoxAndObjNo MatrixInput_Polarity           = 0x06;
static constexpr BoxAndObjNo MatrixInput_ChannelName        = 0x07;
static constexpr BoxAndObjNo MatrixInput_LevelMeterPreMute  = 0x09;
static constexpr BoxAndObjNo MatrixInput_LevelMeterPostMute = 0x0a;
static constexpr BoxAndObjNo MatrixInput_ReverbSendGain     = 0x0d;

static constexpr BoxAndObjNo MatrixNode_Box                 = 0x07;
static constexpr BoxAndObjNo MatrixNode_Enable              = 0x01;
static constexpr BoxAndObjNo MatrixNode_Gain                = 0x02;
static constexpr BoxAndObjNo MatrixNode_Delay               = 0x03;
static constexpr BoxAndObjNo MatrixNode_DelayEnable         = 0x04;

static constexpr BoxAndObjNo MatrixOutput_Box                   = 0x08;
static constexpr BoxAndObjNo MatrixOutput_Mute                  = 0x01;
static constexpr BoxAndObjNo MatrixOutput_Gain                  = 0x02;
static constexpr BoxAndObjNo MatrixOutput_Delay                 = 0x03;
static constexpr BoxAndObjNo MatrixOutput_DelayEnable           = 0x04;
static constexpr BoxAndObjNo MatrixOutput_EqEnable              = 0x05;
static constexpr BoxAndObjNo MatrixOutput_Polarity              = 0x06;
static constexpr BoxAndObjNo MatrixOutput_ChannelName           = 0x07;
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPreMute     = 0x09;
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPostMute    = 0x0a;

static constexpr BoxAndObjNo Positioning_Box                = 0x0d;
static constexpr BoxAndObjNo Positioning_Source_Position    = 0x02;
static constexpr BoxAndObjNo Positioning_Source_Spread      = 0x04;
static constexpr BoxAndObjNo Positioning_Source_DelayMode   = 0x0b;
static constexpr BoxAndObjNo Positioning_Speaker_Position   = 0x07;

static constexpr BoxAndObjNo FunctionGroup_Box            = 0x0e;
static constexpr BoxAndObjNo FunctionGroup_Name           = 0x01;
static constexpr BoxAndObjNo FunctionGroup_Delay          = 0x02;
static constexpr BoxAndObjNo FunctionGroup_SpreadFactor   = 0x06;

static constexpr BoxAndObjNo ReverbInput_Box    = 0x10;
static constexpr BoxAndObjNo ReverbInput_Gain   = 0x01;

static constexpr BoxAndObjNo ReverbInputProcessing_Box          = 0x11;
static constexpr BoxAndObjNo ReverbInputProcessing_Mute         = 0x01;
static constexpr BoxAndObjNo ReverbInputProcessing_Gain         = 0x02;
static constexpr BoxAndObjNo ReverbInputProcessing_EqEnable     = 0x03;
static constexpr BoxAndObjNo ReverbInputProcessing_LevelMeter   = 0x05;

static constexpr BoxAndObjNo Scene_Box              = 0x17;
static constexpr BoxAndObjNo Scene_SceneIndex       = 0x01;
static constexpr BoxAndObjNo Scene_SceneName        = 0x03;
static constexpr BoxAndObjNo Scene_SceneComment     = 0x04;

static constexpr BoxAndObjNo SoundObjectRouting_Box     = 0x18;
static constexpr BoxAndObjNo SoundObjectRouting_Mute    = 0x01;
static constexpr BoxAndObjNo SoundObjectRouting_Gain    = 0x02;

static constexpr std::uint32_t  SceneAgentONo   = 0x2714;


/**
 * Settings_DeviceName
 */
struct dbOcaObjectDef_Settings_DeviceName : Ocp1CommandDefinition
{
    dbOcaObjectDef_Settings_DeviceName()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Settings_Box, Settings_DeviceName), // ONO of Settings_DeviceName,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};

/**
 * Status_StatusText
 */
struct dbOcaObjectDef_Status_StatusText : Ocp1CommandDefinition
{
    dbOcaObjectDef_Status_StatusText()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Status_Box, Status_StatusText), // ONO of Status_StatusText,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringSensor,
            1)                      // Prop_Setting
    {
    }
};

/**
 * Status_AudioNetworkSampleStatus
 */
struct dbOcaObjectDef_Status_AudioNetworkSampleStatus : Ocp1CommandDefinition
{
    dbOcaObjectDef_Status_AudioNetworkSampleStatus()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Status_Box, Status_AudioNetworkSampleStatus), // ONO of Status_AudioNetworkSampleStatus,
            OCP1DATATYPE_INT32,     // Value type
            DefLevel_OcaInt32Sensor,
            1)                      // Prop_Reading
    {
    }
};

/**
 * Error_GnrlErr
 */
struct dbOcaObjectDef_Error_GnrlErr : Ocp1CommandDefinition
{
    dbOcaObjectDef_Error_GnrlErr()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Error_Box, Error_GnrlErr), // ONO of Error_GnrlErr,
            OCP1DATATYPE_BOOLEAN,   // Value type
            DefLevel_OcaBooleanSensor,
            1)                      // Prop_Reading
    {
    }
};

/**
 * Error_ErrorText
 */
struct dbOcaObjectDef_Error_ErrorText : Ocp1CommandDefinition
{
    dbOcaObjectDef_Error_ErrorText()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Error_Box, Error_ErrorText), // ONO of Error_ErrorText,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringSensor,
            1)                      // Prop_String
    {
    }
};

/**
 * CoordinateMappingSettings_Name
 */
struct dbOcaObjectDef_CoordinateMappingSettings_Name : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_Name(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Name), // ONO of CoordinateMappingSettings_Name
            OCP1DATATYPE_STRING,            // Value type
            DefLevel_OcaStringActuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * CoordinateMappingSettings_Type
 */
struct dbOcaObjectDef_CoordinateMappingSettings_Type : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_Type(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Type), // ONO of CoordinateMappingSettings_Type
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_Flip
 */
struct dbOcaObjectDef_CoordinateMappingSettings_Flip : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_Flip(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_Flip), // ONO of CoordinateMappingSettings_Flip
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P1_real
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P1_real : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P1_real(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P1_real), // ONO of CoordinateMappingSettings_P1_real,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P2_real
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P2_real : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P2_real(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P2_real), // ONO of CoordinateMappingSettings_P2_real,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P3_real
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P3_real : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P3_real(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P3_real), // ONO of CoordinateMappingSettings_P3_real,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P4_real
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P4_real : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P4_real(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P4_real), // ONO of CoordinateMappingSettings_P4_real,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P1_virtual
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P1_virtual : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P1_virtual), // ONO of CoordinateMappingSettings_P1_virtual,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMappingSettings_P3_virtual
 */
struct dbOcaObjectDef_CoordinateMappingSettings_P3_virtual : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(std::uint32_t record)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, 0x00, CoordinateMappingSettings_Box, CoordinateMappingSettings_P3_virtual), // ONO of CoordinateMappingSettings_P3_virtual,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * CoordinateMapping_Source_Position
 */
struct dbOcaObjectDef_CoordinateMapping_Source_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_CoordinateMapping_Source_Position(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, CoordinateMapping_Box, CoordinateMapping_Source_Position), // ONO of CoordinateMapping_Source_Position,
            OCP1DATATYPE_DB_POSITION,   // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                          // Prop_Position
    {
    }
};

/**
 * Positioning_Source_Position
 */
struct dbOcaObjectDef_Positioning_Source_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Position(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Box, Positioning_Source_Position), // ONO of Positioning_Source_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                        // Prop_Position
    {
    }
};

/**
 * Positioning_Source_Spread
 */
struct dbOcaObjectDef_Positioning_Source_Spread : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Spread(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Box, Positioning_Source_Spread), // ONO of Positioning_Source_Spread
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaFloat32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * Positioning_Source_DelayMode
 */
struct dbOcaObjectDef_Positioning_Source_DelayMode : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_DelayMode(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Box, Positioning_Source_DelayMode), // ONO of Positioning_Source_DelayMode
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * Positioning_Speaker_Position
 */
struct dbOcaObjectDef_Positioning_Speaker_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Speaker_Position(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Box, Positioning_Speaker_Position), // ONO of Positioning_Speaker_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            DefLevel_dbOcaSpeakerPositionAgentDeprecated,
            1)                        // Prop_Aiming_and_Position
    {
    }
};


/**
 * FunctionGroup_Name
 */
struct dbOcaObjectDef_FunctionGroup_Name : Ocp1CommandDefinition
{
    dbOcaObjectDef_FunctionGroup_Name(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, FunctionGroup_Name), // ONO of FunctionGroup_Name
            OCP1DATATYPE_STRING,            // Value type
            DefLevel_OcaStringActuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * FunctionGroup_Delay
 */
struct dbOcaObjectDef_FunctionGroup_Delay : Ocp1CommandDefinition
{
    dbOcaObjectDef_FunctionGroup_Delay(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, FunctionGroup_Box, FunctionGroup_Delay), // ONO of FunctionGroup_Delay
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaDelay,
            1)                              // Prop_Delay_Time
    {
    }
};

/**
 * FunctionGroup_SpreadFactor
 */
struct dbOcaObjectDef_FunctionGroup_SpreadFactor : Ocp1CommandDefinition
{
    dbOcaObjectDef_FunctionGroup_SpreadFactor(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Box, FunctionGroup_SpreadFactor), // ONO of FunctionGroup_SpreadFactor
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaFloat32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixInput_Mute
 * Parameters for SetValueCommand: setting 1 == MUTE; 2 == UNMUTE
 */
struct dbOcaObjectDef_MatrixInput_Mute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_Mute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_Mute), // ONO of MatrixInput_Mute
            OCP1DATATYPE_UINT8,             // Value type
            DefLevel_OcaMute,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixInput_Gain
 */
struct dbOcaObjectDef_MatrixInput_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_Gain(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_Gain), // ONO of MatrixInput_Gain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * MatrixInput_Delay
 */
struct dbOcaObjectDef_MatrixInput_Delay : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_Delay(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_Delay), // ONO of MatrixInput_Delay
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaDelay,
            1)                              // Prop_Delay_Time
    {
    }
};

/**
 * MatrixInput_DelayEnable
 */
struct dbOcaObjectDef_MatrixInput_DelayEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_DelayEnable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_DelayEnable), // ONO of MatrixInput_DelayEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixInput_EqEnable
 */
struct dbOcaObjectDef_MatrixInput_EqEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_EqEnable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_EqEnable), // ONO of MatrixInput_EqEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixInput_Polarity
 */
struct dbOcaObjectDef_MatrixInput_Polarity : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_Polarity(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_Polarity), // ONO of MatrixInput_Polarity
            OCP1DATATYPE_UINT8,     // Value type
            DefLevel_OcaPolarity,
            1)                      // Prop_State
    {
    }
};

/**
 * MatrixInput_ChannelName
 */
struct dbOcaObjectDef_MatrixInput_ChannelName : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_ChannelName(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_ChannelName), // ONO of MatrixInput_ChannelName
            OCP1DATATYPE_STRING,            // Value type
            DefLevel_OcaStringActuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixInput_LevelMeterPreMute
 */
struct dbOcaObjectDef_MatrixInput_LevelMeterPreMute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_LevelMeterPreMute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_LevelMeterPreMute), // ONO of MatrixInput_LevelMeterPreMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixInput_LevelMeterPostMute
 */
struct dbOcaObjectDef_MatrixInput_LevelMeterPostMute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_LevelMeterPostMute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_LevelMeterPostMute), // ONO of MatrixInput_LevelMeterPostMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixInput_ReverbSendGain
 */
struct dbOcaObjectDef_MatrixInput_ReverbSendGain : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_ReverbSendGain(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_ReverbSendGain), // ONO of MatrixInput_ReverbSendGain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * MatrixNode_Enable
 */
struct dbOcaObjectDef_MatrixNode_Enable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixNode_Enable(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, MatrixNode_Box, MatrixNode_Enable), // ONO of MatrixNode_Enable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixNode_Gain
 */
struct dbOcaObjectDef_MatrixNode_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixNode_Gain(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, MatrixNode_Box, MatrixNode_Gain), // ONO of MatrixNode_Gain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * MatrixNode_Delay
 */
struct dbOcaObjectDef_MatrixNode_Delay : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixNode_Delay(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, MatrixNode_Box, MatrixNode_Delay), // ONO of MatrixNode_Delay
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaDelay,
            1)                              // Prop_Delay_Time
    {
    }
};

/**
 * MatrixNode_DelayEnable
 */
struct dbOcaObjectDef_MatrixNode_DelayEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixNode_DelayEnable(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, MatrixNode_Box, MatrixNode_DelayEnable), // ONO of MatrixNode_DelayEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixOutput_Mute
 * Parameters for SetValueCommand: setting 1 == MUTE; 2 == UNMUTE
 */
struct dbOcaObjectDef_MatrixOutput_Mute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_Mute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_Mute), // ONO of MatrixOutput_Mute
            OCP1DATATYPE_UINT8,             // Value type
            DefLevel_OcaMute,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixOutput_Gain
 */
struct dbOcaObjectDef_MatrixOutput_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_Gain(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_Gain), // ONO of MatrixOutput_Gain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * MatrixOutput_Delay
 */
struct dbOcaObjectDef_MatrixOutput_Delay : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_Delay(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_Delay), // ONO of MatrixOutput_Delay
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaDelay,
            1)                              // Prop_Delay_Time
    {
    }
};

/**
 * MatrixOutput_DelayEnable
 */
struct dbOcaObjectDef_MatrixOutput_DelayEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_DelayEnable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_DelayEnable), // ONO of MatrixOutput_DelayEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixOutput_EqEnable
 */
struct dbOcaObjectDef_MatrixOutput_EqEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_EqEnable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_EqEnable), // ONO of MatrixOutput_EqEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixOutput_Polarity
 */
struct dbOcaObjectDef_MatrixOutput_Polarity : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_Polarity(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_Polarity), // ONO of MatrixOutput_Polarity
            OCP1DATATYPE_UINT8,     // Value type
            DefLevel_OcaPolarity,
            1)                      // Prop_State
    {
    }
};

/**
 * MatrixOutput_ChannelName
 */
struct dbOcaObjectDef_MatrixOutput_ChannelName : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_ChannelName(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_ChannelName), // ONO of MatrixOutput_ChannelName
            OCP1DATATYPE_STRING,            // Value type
            DefLevel_OcaStringActuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixOutput_LevelMeterPreMute
 */
struct dbOcaObjectDef_MatrixOutput_LevelMeterPreMute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixOutput_LevelMeterPreMute), // ONO of MatrixOutput_LevelMeterPreMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixOutput_LevelMeterPostMute
 */
struct dbOcaObjectDef_MatrixOutput_LevelMeterPostMute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixOutput_LevelMeterPostMute), // ONO of MatrixOutput_LevelMeterPostMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixSettings_ReverbRoomId
 */
struct dbOcaObjectDef_MatrixSettings_ReverbRoomId : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixSettings_ReverbRoomId()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId), // ONO of MatrixSettings_ReverbRoomId
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixSettings_ReverbPredelayFactor
 */
struct dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbPredelayFactor), // ONO of MatrixSettings_ReverbPredelayFactor
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaFloat32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * MatrixSettings_ReverbRearLevel
 */
struct dbOcaObjectDef_MatrixSettings_ReverbRearLevel : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixSettings_ReverbRearLevel()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRearLevel), // ONO of MatrixSettings_ReverbRearLevel
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * ReverbInput_Gain
 */
struct dbOcaObjectDef_ReverbInput_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_ReverbInput_Gain(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, ReverbInput_Box, ReverbInput_Gain), // ONO of ReverbInput_Gain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};


/**
 * En-Space zone Mute 
 */
struct dbOcaObjectDef_ReverbInputProcessing_Mute : Ocp1CommandDefinition
{
    dbOcaObjectDef_ReverbInputProcessing_Mute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, ReverbInputProcessing_Box, ReverbInputProcessing_Mute), // ONO of ReverbInputProcessing_Mute,
            OCP1DATATYPE_UINT8,             // Value type
            DefLevel_OcaMute,
            1)                              // Prop_Setting
    {
    }
};

/**
 * En-Space zone Gain
 */
struct dbOcaObjectDef_ReverbInputProcessing_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_ReverbInputProcessing_Gain(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, ReverbInputProcessing_Box, ReverbInputProcessing_Gain), // ONO of ReverbInputProcessing_Gain,
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * En-Space zone EQ Enable
 */
struct dbOcaObjectDef_ReverbInputProcessing_EqEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_ReverbInputProcessing_EqEnable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, ReverbInputProcessing_Box, ReverbInputProcessing_EqEnable), // ONO of ReverbInputProcessing_EqEnable,
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * En-Space zone Level 
 */
struct dbOcaObjectDef_ReverbInputProcessing_LevelMeter : Ocp1CommandDefinition
{
    dbOcaObjectDef_ReverbInputProcessing_LevelMeter(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, ReverbInputProcessing_Box, ReverbInputProcessing_LevelMeter), // ONO of ReverbInputProcessing_LevelMeter,
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * Scene_SceneIndex
 */
struct dbOcaObjectDef_Scene_SceneIndex : Ocp1CommandDefinition
{
    dbOcaObjectDef_Scene_SceneIndex()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneIndex), // ONO of Scene_SceneIndex,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};

/**
 * Scene_SceneName
 */
struct dbOcaObjectDef_Scene_SceneName : Ocp1CommandDefinition
{
    dbOcaObjectDef_Scene_SceneName()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneName), // ONO of Scene_SceneName,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};

/**
 * Scene_SceneComment
 */
struct dbOcaObjectDef_Scene_SceneComment : Ocp1CommandDefinition
{
    dbOcaObjectDef_Scene_SceneComment()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Scene_Box, Scene_SceneComment), // ONO of Scene_SceneComment,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};


/**
 * SoundObjectRouting_Mute
 * Parameters for SetValueCommand: setting 1 == MUTE; 2 == UNMUTE
 */
struct dbOcaObjectDef_SoundObjectRouting_Mute : Ocp1CommandDefinition
{
    dbOcaObjectDef_SoundObjectRouting_Mute(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, SoundObjectRouting_Box, SoundObjectRouting_Mute), // ONO of SoundObjectRouting_Mute
            OCP1DATATYPE_UINT8,             // Value type
            DefLevel_OcaMute,
            1)                              // Prop_Setting
    {
    }
};

/**
 * SoundObjectRouting_Gain
 */
struct dbOcaObjectDef_SoundObjectRouting_Gain : Ocp1CommandDefinition
{
    dbOcaObjectDef_SoundObjectRouting_Gain(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, SoundObjectRouting_Box, SoundObjectRouting_Gain), // ONO of SoundObjectRouting_Gain
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * SceneAgent
 */
struct dbOcaObjectDef_SceneAgent : Ocp1CommandDefinition
{
    dbOcaObjectDef_SceneAgent()
        : Ocp1CommandDefinition(SceneAgentONo, // ONO of custom SceneAgent,
            OCP1DATATYPE_UINT32,    // Value type
            DefLevel_dbOcaSceneAgent,
            0)                      // Dummy
    {
    }

    Ocp1CommandDefinition ApplyCommand(std::uint16_t major, std::uint16_t minor)
    {
        std::uint32_t newValue = minor + (major << 16);

        std::uint8_t paramCount(1);
        std::vector<std::uint8_t> newParamData = DataFromUint32(newValue);

        return Ocp1CommandDefinition(m_targetOno,
            OCP1DATATYPE_UINT32,
            m_propertyDefLevel,
            7,                     // ApplyScene is MethodIdx 7
            paramCount,
            newParamData);
    }

    Ocp1CommandDefinition PreviousCommand()
    {
        return Ocp1CommandDefinition(m_targetOno,
            OCP1DATATYPE_NONE,
            m_propertyDefLevel,
            8);                     // PreviousScene is MethodIdx 8
    }

    Ocp1CommandDefinition NextCommand()
    {
        return Ocp1CommandDefinition(m_targetOno,
            OCP1DATATYPE_NONE,
            m_propertyDefLevel,
            9);                     // NextScene is MethodIdx 9
    }

    Ocp1CommandDefinition* Clone() const override
    {
        return std::unique_ptr<Ocp1CommandDefinition>(new dbOcaObjectDef_SceneAgent(*this)).release();
    }
};

}

}