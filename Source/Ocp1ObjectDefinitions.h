/* Copyright (c) 2023, Bernardo Escalona
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


#include "Ocp1Message.h"
#include "Ocp1DataTypes.h"


namespace NanoOcp1
{

/**
 * @brief Shorthand for the combined box-and-object-number part of an ONo.
 *
 * An OCA Object Number (ONo) encodes `type`, `record`, `channel`, and a
 * `boxAndObjectNumber` packed together by `GetONo()`.  The `BoxAndObjNo` constants
 * in each device namespace (e.g. `DS100::Fixed_GUID = 0x0f`) are the `boxAndObjectNumber`
 * argument to `GetONo()`.  Concrete `dbOcaObjectDef_*` structs call `GetONo()` in
 * their constructors so callers never need to compose ONos manually.
 */
typedef std::uint32_t BoxAndObjNo;

/**
 * @name AES70 class definition levels
 * @brief Inheritance-depth constants used as `m_propertyDefLevel` in `Ocp1CommandDefinition`.
 *
 * In AES70 every property is defined at a specific level in the OCA class hierarchy.
 * The def-level is the number of classes between `OcaRoot` (level 1) and the class
 * that introduces the property.  NanoOcp uses these constants to build the correct
 * `Ocp1CommandDefinition::m_propertyDefLevel` fields.
 *
 * Example: `OcaGain` inherits `OcaRoot → OcaWorker → OcaActuator → OcaGain`, so
 * `DefLevel_OcaGain = 4`.  The `Prop_Gain` property in `OcaGain` therefore has
 * `m_propertyDefLevel = 4`.
 * @{
 */
static constexpr int DefLevel_OcaRoot               = 1;
static constexpr int DefLevel_OcaAgent              = 1 + DefLevel_OcaRoot;
static constexpr int DefLevel_OcaWorker             = 1 + DefLevel_OcaRoot;
static constexpr int DefLevel_OcaActuator           = 1 + DefLevel_OcaWorker;
static constexpr int DefLevel_OcaPolarity           = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaSwitch             = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaGain               = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaMute               = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaDelay              = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaBasicActuator      = 1 + DefLevel_OcaActuator;
static constexpr int DefLevel_OcaInt32Actuator      = 1 + DefLevel_OcaBasicActuator;
static constexpr int DefLevel_OcaFloat32Actuator    = 1 + DefLevel_OcaBasicActuator;
static constexpr int DefLevel_OcaStringActuator     = 1 + DefLevel_OcaBasicActuator;
static constexpr int DefLevel_OcaSensor             = 1 + DefLevel_OcaWorker;
static constexpr int DefLevel_OcaLevelSensor        = 1 + DefLevel_OcaSensor;
static constexpr int DefLevel_OcaAudioLevelSensor   = 1 + DefLevel_OcaLevelSensor;
static constexpr int DefLevel_OcaBasicSensor        = 1 + DefLevel_OcaSensor;
static constexpr int DefLevel_OcaBooleanSensor      = 1 + DefLevel_OcaBasicSensor;
static constexpr int DefLevel_OcaInt32Sensor        = 1 + DefLevel_OcaBasicSensor;
static constexpr int DefLevel_OcaStringSensor       = 1 + DefLevel_OcaBasicSensor;

static constexpr int DefLevel_dbOcaDataTransfer                     = 1 + DefLevel_OcaAgent;         ///< d&b data-transfer agent (DS100 scene control).
static constexpr int DefLevel_dbOcaSceneAgent                       = 1 + DefLevel_dbOcaDataTransfer; ///< DS100 scene agent, extends dbOcaDataTransfer.
static constexpr int DefLevel_dbOcaPositionAgentDeprecated          = 1 + DefLevel_OcaAgent;          ///< Deprecated d&b 3D position agent (pre-firmware DB000CD0).
static constexpr int DefLevel_dbOcaSpeakerPositionAgentDeprecated   = 1 + DefLevel_dbOcaPositionAgentDeprecated; ///< Deprecated d&b speaker-position agent.
/** @} */


/**
 * @namespace NanoOcp1::AmpGeneric
 * @brief Object definitions common to all d&b amplifier platforms.
 *
 * These constants and structs apply to any d&b amp that exposes the generic
 * OCA object model (Dx, Dy, 5D, etc.).  For DS100-specific objects, see
 * `NanoOcp1::DS100`.
 */
//==============================================================================
namespace AmpGeneric
{

static constexpr BoxAndObjNo Config_Mute        = 0x205;
static constexpr BoxAndObjNo Config_PotiLevel   = 0x206;

/**
 * Config_PotiLevel
 */
struct dbOcaObjectDef_Config_PotiLevel : Ocp1CommandDefinition
{
    dbOcaObjectDef_Config_PotiLevel(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, Config_PotiLevel), // ONO of Config_PotiLevel
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaGain,
            1)                              // Prop_Gain
    {
    }
};

/**
 * Config_Mute
 * Parameters for SetValueCommand: setting 1 == MUTE; 2 == UNMUTE
 */
struct dbOcaObjectDef_Config_Mute : Ocp1CommandDefinition
{
    dbOcaObjectDef_Config_Mute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, Config_Mute), // ONO of Config_Mute
            OCP1DATATYPE_UINT8,             // Value type
            DefLevel_OcaMute,
            1)                              // Prop_Setting
    {
    }
};
}


//==============================================================================
// Dx and Dy amplifiers
//==============================================================================
namespace AmpDxDy
{

static constexpr BoxAndObjNo Settings_PwrOn     = 0x100;
static constexpr BoxAndObjNo ChStatus_Isp       = 0x400;
static constexpr BoxAndObjNo ChStatus_Gr        = 0x401;
static constexpr BoxAndObjNo ChStatus_Ovl       = 0x402;

/**
 * Settings_PwrOn
 * Parameters for SetValueCommand: setting 1 == ON; 0 == OFF
 */
struct dbOcaObjectDef_Settings_PwrOn : Ocp1CommandDefinition
{
    dbOcaObjectDef_Settings_PwrOn() 
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, 0x00, Settings_PwrOn), // ONO of Settings_PwrOn
            OCP1DATATYPE_UINT16,             // Value type
            DefLevel_OcaSwitch,
            1)                              // Prop_Setting
    {
    }
};

/**
 * ChStatus_Isp
 */
struct dbOcaObjectDef_ChStatus_Isp : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Isp(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Isp), // ONO of ChStatus_Isp
                                OCP1DATATYPE_BOOLEAN,           // Value type
                                DefLevel_OcaBooleanSensor,
                                1)                              // Prop_Reading
    {
    }
};

/**
 * ChStatus_Gr
 */
struct dbOcaObjectDef_ChStatus_Gr : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Gr(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Gr), // ONO of ChStatus_Gr
                                OCP1DATATYPE_BOOLEAN,           // Value type
                                DefLevel_OcaBooleanSensor,
                                1)                              // Prop_Reading
    {
    }
};

/**
 * ChStatus_Ovl
 */
struct dbOcaObjectDef_ChStatus_Ovl : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Ovl(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Ovl), // ONO of ChStatus_Ovl
            OCP1DATATYPE_BOOLEAN,           // Value type
            DefLevel_OcaBooleanSensor,
            1)                              // Prop_Reading
    {
    }
};

}


//==============================================================================
// Dx amplifier
//==============================================================================
namespace AmpDx // NOTE: namespaces starting with numbers cause problems
{

static constexpr BoxAndObjNo ChStatus_GrHead = 0x40c;

/**
 * ChStatus_GrHead
 */
struct dbOcaObjectDef_ChStatus_GrHead : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_GrHead(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_GrHead), // ONO of ChStatus_GrHead
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Reading ?
    {
    }
};

}


//==============================================================================
// Dy amplifier
//==============================================================================
namespace AmpDy // NOTE: namespaces starting with numbers cause problems
{

static constexpr BoxAndObjNo ChStatus_GrHead = 0x40a;

/**
 * ChStatus_GrHead
 */
struct dbOcaObjectDef_ChStatus_GrHead : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_GrHead(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_GrHead), // ONO of ChStatus_GrHead
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Reading ?
    {
    }
};

}


//==============================================================================
// 5D amplifier
//==============================================================================
namespace Amp5D // NOTE: namespaces starting with numbers cause problems
{

static constexpr BoxAndObjNo Settings_PwrOn     = 0x101; 
static constexpr BoxAndObjNo ChStatus_Isp       = 0x401;
static constexpr BoxAndObjNo ChStatus_Gr        = 0x402;
static constexpr BoxAndObjNo ChStatus_Ovl       = 0x403;
static constexpr BoxAndObjNo ChStatus_GrHead    = 0x404;

/**
 * Settings_PwrOn
 * Parameters for SetValueCommand: setting 1 == ON; 0 == OFF
 */
struct dbOcaObjectDef_Settings_PwrOn : Ocp1CommandDefinition
{
    dbOcaObjectDef_Settings_PwrOn()
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, 0x00, Settings_PwrOn), // ONO of Settings_PwrOn
            OCP1DATATYPE_UINT16,             // Value type
            DefLevel_OcaSwitch,
            1)                              // Prop_Setting
    {
    }
};

/**
 * ChStatus_Isp
 */
struct dbOcaObjectDef_ChStatus_Isp : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Isp(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Isp), // ONO of ChStatus_Isp
            OCP1DATATYPE_BOOLEAN,           // Value type
            DefLevel_OcaBooleanSensor,
            1)                              // Prop_Reading
    {
    }
};

/**
 * ChStatus_Gr
 */
struct dbOcaObjectDef_ChStatus_Gr : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Gr(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Gr), // ONO of ChStatus_Gr
            OCP1DATATYPE_BOOLEAN,           // Value type
            DefLevel_OcaBooleanSensor,
            1)                              // Prop_Reading
    {
    }
};

/**
 * ChStatus_Ovl
 */
struct dbOcaObjectDef_ChStatus_Ovl : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_Ovl(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_Ovl), // ONO of ChStatus_Ovl
            OCP1DATATYPE_BOOLEAN,           // Value type
            DefLevel_OcaBooleanSensor,
            1)                              // Prop_Reading
    {
    }
};

/**
 * ChStatus_GrHead
 */
struct dbOcaObjectDef_ChStatus_GrHead : Ocp1CommandDefinition
{
    dbOcaObjectDef_ChStatus_GrHead(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONo(0x01, 0x00, channel, ChStatus_GrHead), // ONO of ChStatus_GrHead
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaAudioLevelSensor,
            1)                              // Prop_Reading ?
    {
    }
};

}

}
