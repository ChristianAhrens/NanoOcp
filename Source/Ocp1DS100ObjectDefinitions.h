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

static constexpr BoxAndObjNo Settings_Box = 0x01;
static constexpr BoxAndObjNo Settings_DeviceName = 0x0d;

static constexpr BoxAndObjNo MatrixSettings_Box = 0x02;
static constexpr BoxAndObjNo MatrixSettings_ReverbRoomId = 0x0a;
static constexpr BoxAndObjNo MatrixSettings_ReverbPredelayFactor = 0x04;
static constexpr BoxAndObjNo MatrixSettings_ReverbRearLevel = 0x15;

static constexpr BoxAndObjNo CoordinateMapping_Box              = 0x16;
static constexpr BoxAndObjNo CoordinateMapping_Source_Position  = 0x01;

static constexpr BoxAndObjNo MatrixInput_Box                = 0x05;
static constexpr BoxAndObjNo MatrixInput_Mute               = 0x01;
static constexpr BoxAndObjNo MatrixInput_Gain               = 0x02;
static constexpr BoxAndObjNo MatrixInput_ChannelName        = 0x07;
static constexpr BoxAndObjNo MatrixInput_LevelMeterPreMute  = 0x09;
static constexpr BoxAndObjNo MatrixInput_ReverbSendGain     = 0x0d;

static constexpr BoxAndObjNo MatrixOutput_Box                   = 0x08;
static constexpr BoxAndObjNo MatrixOutput_Mute                  = 0x01;
static constexpr BoxAndObjNo MatrixOutput_Gain                  = 0x02;
static constexpr BoxAndObjNo MatrixOutput_ChannelName           = 0x07;
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPreMute     = 0x09;
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPostMute    = 0x0a;

static constexpr BoxAndObjNo Positioning_Box = 0x0d;
static constexpr BoxAndObjNo Positioning_Source_Position = 0x02;
static constexpr BoxAndObjNo Positioning_Source_Spread = 0x04;
static constexpr BoxAndObjNo Positioning_Source_DelayMode = 0x0b;
static constexpr BoxAndObjNo Positioning_Speaker_Position = 0x07;

static constexpr BoxAndObjNo Scene_Box              = 0x17;
static constexpr BoxAndObjNo Scene_SceneIndex       = 0x01;
static constexpr BoxAndObjNo Scene_SceneName        = 0x03;
static constexpr BoxAndObjNo Scene_SceneComment     = 0x04;
static constexpr BoxAndObjNo Scene_SceneNext        = 0x00; /*todo*/
static constexpr BoxAndObjNo Scene_ScenePrevious    = 0x00; /*todo*/
static constexpr BoxAndObjNo Scene_SceneRecall      = 0x00; /*todo*/


/**
 * Settings_DeviceName
 */
struct dbOcaObjectDef_Settings_DeviceName : Ocp1CommandDefinition
{
    dbOcaObjectDef_Settings_DeviceName()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Settings_Box, Settings_DeviceName), // ONO of Settings_DeviceName,
            OCP1DATATYPE_STRING,    // Value type
            5,                      // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
            1)                      // Prop_Setting
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
            3,                          // CdbOcaPositionAgentDeprecated level
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
            3,                        // CdbOcaPositionAgentDeprecated level
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
            5,                              // OcaFloat32Actuator level - root:worker:actuator:basicactuator:float32actuator
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
            4,                      // OcaSwitch level - root:worker:actuator:switch
            1)                      // Prop_Position
    {
    }
};

/**
 * Positioning_Speaker_Position
 */
struct dbOcaObjectDef_Positioning_Speaker_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Speaker_Position(std::uint32_t record, std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, record, channel, Positioning_Box, Positioning_Speaker_Position), // ONO of Positioning_Speaker_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            3,                        // CdbOcaPositionAgentDeprecated level
            1)                        // Prop_Position
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
            4,                              // OcaMute level - root:worker:actuator:mute
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
            4,                              // OcaGain level - root:worker:actuator:gain
            1)                              // Prop_Gain
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
            5,                              // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
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
            4,                              // OcaAudioLevelSensor level
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
            4,                              // OcaGain level
            1)                              // Prop_Gain
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
            4,                              // OcaMute level - root:worker:actuator:mute
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
            4,                              // OcaGain level - root:worker:actuator:gain
            1)                              // Prop_Gain
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
            5,                              // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
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
            4,                              // OcaAudioLevelSensor level
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
            4,                              // OcaAudioLevelSensor level
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
            4,                      // OcaSwitch level - root:worker:actuator:switch
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
            4,                              // OcaAudioLevelSensor level
            1)                              // Prop_Level
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
            4,                              // OcaGain level - root:worker:actuator:gain
            1)                              // Prop_Gain
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
            5,                      // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
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
            5,                      // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
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
            5,                      // OcaStringActuator level - root:worker:actuator:basicactuator:stringactuator
            1)                      // Prop_Setting
    {
    }
};

}

}