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

#include <Ocp1ObjectDefinitions.h>


namespace NanoOcp1
{

/**
 * @namespace NanoOcp1::DS100
 * @brief OCA object definitions for the d&b audiotechnik DS100 signal engine.
 *
 * ## What is the DS100?
 * The DS100 is a d&b audiotechnik spatial audio signal engine used as the processing
 * core of d&b Soundscape systems.  It acts as an audio matrix and hosts two spatial
 * processing engines:
 * - **En-Scene** — object-based panning: each *sound object* (matrix input) is
 *   positioned in real-world XY coordinates and routed to loudspeakers accordingly.
 * - **En-Space** — convolution reverb with four independent zones (L/C/R stage +
 *   audience area).
 *
 * The DS100 comes in three hardware variants:
 * | Variant | GUID suffix | Notes |
 * |---|---|---|
 * | DS100 (legacy Dante) | `D0` | Original model; stack-ident depends on firmware ≥ `0C`. |
 * | DS100D (updated Dante) | `D1` | Always stack-ident 1. |
 * | DS100M (Milan) | `D2` | Stack-ident 1 from firmware ≥ `02`. |
 *
 * ## OCA object model on the DS100
 * DS100 parameters are organised into functional boxes, each with a `_Box` ONo
 * prefix and individual per-parameter `BoxAndObjNo` constants.  The final ONo is
 * computed by `GetONoTy2()` (most DS100 objects) or `GetONo()` (amp-style objects).
 *
 * | Box | Prefix | Description |
 * |---|---|---|
 * | Fixed | `Fixed_Box = 0x00` | Read-only device identity (GUID, serial, hardware variant). |
 * | Settings | `Settings_Box = 0x01` | Writable device settings (name). |
 * | Status | `Status_Box = 0x03` | Status text, audio network sample status. |
 * | MatrixSettings | `MatrixSettings_Box = 0x02` | En-Scene/En-Space enable, reverb parameters. |
 * | CoordinateMappingSettings | `CoordinateMappingSettings_Box = 0x15` | Corner-point definitions for mapping areas 1–4. |
 * | CoordinateMapping | `CoordinateMapping_Box = 0x16` | Per-source position in virtual/mapped space. |
 * | MatrixInput | `MatrixInput_Box = 0x05` | Per-input-channel (sound object): mute, gain, delay, EQ, level meters. |
 * | MatrixNode | `MatrixNode_Box = 0x07` | Per-crosspoint: enable, gain, delay. |
 * | MatrixOutput | `MatrixOutput_Box = 0x08` | Per-output-channel: mute, gain, delay, EQ, level meters. |
 * | Positioning (source) | `Positioning_Source_Box = 0x0d` | En-Scene position, spread, delay mode per sound object. |
 * | Positioning (speaker) | `Positioning_Speaker_Box = 0x1a` | 6-DOF loudspeaker position per output. Introduced with `GUID DB000CD0`; earlier firmware used `Positioning_Source_Speaker_Position`. |
 * | FunctionGroup | `FunctionGroup_Box = 0x0e` | Named loudspeaker groups: delay, spread factor. |
 * | ReverbInput | `ReverbInput_Box = 0x10` | Send gain per sound object / zone. |
 * | ReverbInputProcessing | `ReverbInputProcessing_Box = 0x11` | Mute, gain, EQ per send. |
 * | Scene | `Scene_Box = 0x17` | Scene index, name, comment; scene recall uses the SceneAgent ONo. |
 * | SoundObjectRouting | `SoundObjectRouting_Box = 0x18` | Mute/gain per sound-object × function-group routing pair. |
 *
 * ## Firmware branching (stack-ident)
 * The speaker-position OCA definition changed between firmware generations.
 * `DeviceController` reads `Fixed_GUID` immediately after connect, determines the
 * stack-ident (0 = legacy, 1 = extended), and selects the correct definition struct:
 * - **Stack 0**: `dbOcaObjectDef_Positioning_Source_Speaker_Position` (deprecated object under `Positioning_Source_Box`).
 * - **Stack 1+**: `dbOcaObjectDef_Positioning_Speaker_Position` (dedicated `Positioning_Speaker_Box`).
 *
 * ## Usage example (Umsci `DeviceController` pattern)
 * ```cpp
 * // Subscribe to and read sound-object 3 position:
 * DS100::dbOcaObjectDef_Positioning_Source_Position def(3);
 * uint32_t subHandle, getHandle;
 * client->sendData(Ocp1CommandResponseRequired(def.AddSubscriptionCommand(), subHandle).GetSerializedData());
 * client->sendData(Ocp1CommandResponseRequired(def.GetValueCommand(), getHandle).GetSerializedData());
 *
 * // In onDataReceived, when a notification arrives:
 * auto* notif = static_cast<Ocp1Notification*>(msg.get());
 * if (def.GetValueCommand().m_targetOno == notif->GetEmitterOno()) {
 *     Variant v(notif->GetParameterData(), OCP1DATATYPE_BLOB);
 *     auto [x, y, z] = v.ToPosition();
 * }
 * ```
 */
//==============================================================================
namespace DS100
{

// ─── Device identity (Fixed box) ──────────────────────────────────────────────
static constexpr BoxAndObjNo Fixed_Box              = 0x00; ///< Box number for all Fixed (read-only) device identity objects.
static constexpr BoxAndObjNo Fixed_HardwareVariant  = 0x02; ///< Hardware variant code (int32).
static constexpr BoxAndObjNo Fixed_SerNr            = 0x0a; ///< Serial number string.
static constexpr BoxAndObjNo Fixed_GUID             = 0x0f; ///< 8-char hex firmware/model GUID (e.g. `"DB000CD0"`). Read on connect to detect stack-ident.

// ─── Device settings ──────────────────────────────────────────────────────────
static constexpr BoxAndObjNo Settings_Box           = 0x01; ///< Box number for writable device settings.
static constexpr BoxAndObjNo Settings_DeviceName    = 0x0d; ///< User-assignable device name string.

// ─── Status ───────────────────────────────────────────────────────────────────
static constexpr BoxAndObjNo Status_Box                         = 0x03; ///< Box number for read-only status objects.
static constexpr BoxAndObjNo Status_StatusText                  = 0x03; ///< Human-readable device status string.
static constexpr BoxAndObjNo Status_AudioNetworkSampleStatus    = 0x30; ///< Audio network sample-rate/lock status.

// ─── Error ────────────────────────────────────────────────────────────────────
static constexpr BoxAndObjNo Error_Box          = 0x04; ///< Box number for error-reporting objects.
static constexpr BoxAndObjNo Error_GnrlErr      = 0x01; ///< General error flag.
static constexpr BoxAndObjNo Error_ErrorText    = 0x03; ///< Human-readable error description string.

// ─── Matrix settings (global En-Scene / En-Space) ────────────────────────────
static constexpr BoxAndObjNo MatrixSettings_Box                     = 0x02; ///< Box number for global matrix / En-Space / En-Scene settings.
static constexpr BoxAndObjNo MatrixSettings_PositioningEnable       = 0x02; ///< Enable / disable En-Scene object-based positioning globally.
static constexpr BoxAndObjNo MatrixSettings_ReverbEnable            = 0x03; ///< Enable / disable En-Space convolution reverb globally.
static constexpr BoxAndObjNo MatrixSettings_ReverbRoomId            = 0x0a; ///< En-Space room impulse-response selection (uint32 room index).
static constexpr BoxAndObjNo MatrixSettings_ReverbPredelayFactor    = 0x14; ///< Pre-delay scaling factor for the selected room (float32).
static constexpr BoxAndObjNo MatrixSettings_ReverbRearLevel         = 0x15; ///< Rear-channel level adjustment for the reverb tail (float32, dB).

// ─── Coordinate mapping settings (corner points for mapping areas 1–4) ───────
static constexpr BoxAndObjNo CoordinateMappingSettings_Box          = 0x15; ///< Box number for coordinate-mapping corner-point configuration (per area).
static constexpr BoxAndObjNo CoordinateMappingSettings_Name         = 0x01; ///< Name string for this mapping area.
static constexpr BoxAndObjNo CoordinateMappingSettings_Type         = 0x02; ///< Mapping type (uint32 enum).
static constexpr BoxAndObjNo CoordinateMappingSettings_Flip         = 0x03; ///< Axis-flip flags (bool).
static constexpr BoxAndObjNo CoordinateMappingSettings_P1_real      = 0x04; ///< Corner point P1 in real-world coordinates (3 × float32 XYZ).
static constexpr BoxAndObjNo CoordinateMappingSettings_P2_real      = 0x05; ///< Corner point P2 in real-world coordinates.
static constexpr BoxAndObjNo CoordinateMappingSettings_P3_real      = 0x06; ///< Corner point P3 in real-world coordinates.
static constexpr BoxAndObjNo CoordinateMappingSettings_P4_real      = 0x07; ///< Corner point P4 in real-world coordinates.
static constexpr BoxAndObjNo CoordinateMappingSettings_P1_virtual   = 0x08; ///< Corner point P1 in virtual (mapped) coordinates.
static constexpr BoxAndObjNo CoordinateMappingSettings_P3_virtual   = 0x09; ///< Corner point P3 in virtual (mapped) coordinates.

// ─── Coordinate mapping (per-source positions in virtual space) ───────────────
static constexpr BoxAndObjNo CoordinateMapping_Box              = 0x16; ///< Box number for per-source coordinate-mapped positions (per area).
static constexpr BoxAndObjNo CoordinateMapping_Source_Position  = 0x01; ///< Source position in the virtual coordinate space of the mapping area (3 × float32 XYZ, blob).

// ─── Matrix inputs (sound objects / En-Scene sources) ────────────────────────
static constexpr BoxAndObjNo MatrixInput_Box                = 0x05; ///< Box number for per-input-channel (sound object) parameters.
static constexpr BoxAndObjNo MatrixInput_Mute               = 0x01; ///< Input mute (bool: 0=unmuted, 1=muted).
static constexpr BoxAndObjNo MatrixInput_Gain               = 0x02; ///< Input gain in dB (float32).
static constexpr BoxAndObjNo MatrixInput_Delay              = 0x03; ///< Input delay in ms (float32).
static constexpr BoxAndObjNo MatrixInput_DelayEnable        = 0x04; ///< Input delay enable (bool).
static constexpr BoxAndObjNo MatrixInput_EqEnable           = 0x05; ///< Input EQ enable (bool).
static constexpr BoxAndObjNo MatrixInput_Polarity           = 0x06; ///< Input polarity invert (bool).
static constexpr BoxAndObjNo MatrixInput_ChannelName        = 0x07; ///< User-assignable input channel name (string).
static constexpr BoxAndObjNo MatrixInput_LevelMeterIn       = 0x08; ///< Pre-processing level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixInput_LevelMeterPreMute  = 0x09; ///< Pre-mute level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixInput_LevelMeterPostMute = 0x0a; ///< Post-mute level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixInput_ISP                = 0x0b; ///< Input signal presence indicator (bool).
static constexpr BoxAndObjNo MatrixInput_ReverbSendGain     = 0x0d; ///< En-Space send gain for this input (float32, dB).

// ─── Matrix nodes (crosspoints in the routing matrix) ─────────────────────────
static constexpr BoxAndObjNo MatrixNode_Box                 = 0x07; ///< Box number for per-crosspoint (input × output) routing parameters.
static constexpr BoxAndObjNo MatrixNode_Enable              = 0x01; ///< Crosspoint enable / mute (bool).
static constexpr BoxAndObjNo MatrixNode_Gain                = 0x02; ///< Crosspoint gain (float32, dB).
static constexpr BoxAndObjNo MatrixNode_Delay               = 0x03; ///< Crosspoint delay (float32, ms).
static constexpr BoxAndObjNo MatrixNode_DelayEnable         = 0x04; ///< Crosspoint delay enable (bool).

// ─── Matrix outputs ───────────────────────────────────────────────────────────
static constexpr BoxAndObjNo MatrixOutput_Box                   = 0x08; ///< Box number for per-output-channel parameters.
static constexpr BoxAndObjNo MatrixOutput_Mute                  = 0x01; ///< Output mute (bool).
static constexpr BoxAndObjNo MatrixOutput_Gain                  = 0x02; ///< Output gain in dB (float32).
static constexpr BoxAndObjNo MatrixOutput_Delay                 = 0x03; ///< Output delay in ms (float32).
static constexpr BoxAndObjNo MatrixOutput_DelayEnable           = 0x04; ///< Output delay enable (bool).
static constexpr BoxAndObjNo MatrixOutput_EqEnable              = 0x05; ///< Output EQ enable (bool).
static constexpr BoxAndObjNo MatrixOutput_Polarity              = 0x06; ///< Output polarity invert (bool).
static constexpr BoxAndObjNo MatrixOutput_ChannelName           = 0x07; ///< User-assignable output channel name (string).
static constexpr BoxAndObjNo MatrixOutput_LevelMeterIn          = 0x08; ///< Pre-processing level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPreMute     = 0x09; ///< Pre-mute level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixOutput_LevelMeterPostMute    = 0x0a; ///< Post-mute level meter reading (float32, dBFS).
static constexpr BoxAndObjNo MatrixOutput_OSP                   = 0x0b; ///< Output signal presence indicator (bool).

// ─── En-Scene source positioning ──────────────────────────────────────────────
static constexpr BoxAndObjNo Positioning_Source_Box                 = 0x0d; ///< Box number for per-sound-object En-Scene positioning parameters.
static constexpr BoxAndObjNo Positioning_Source_Position            = 0x02; ///< Sound object XYZ position in real-world space (3 × float32 blob, normalised 0–1).
static constexpr BoxAndObjNo Positioning_Source_Enable              = 0x03; ///< En-Scene processing enable for this sound object (bool).
static constexpr BoxAndObjNo Positioning_Source_Spread              = 0x04; ///< Sound object spread factor (float32, 0–1).
static constexpr BoxAndObjNo Positioning_Source_Speaker_Group       = 0x06; ///< @deprecated Function-group assignment. Use `Positioning_Speaker_Group` from firmware `DB000CD0` onwards.
static constexpr BoxAndObjNo Positioning_Source_Speaker_Position    = 0x07; ///< @deprecated Loudspeaker position (6-DOF blob). Use `Positioning_Speaker_Position` from firmware `DB000CD0` onwards.
static constexpr BoxAndObjNo Positioning_Source_DelayMode           = 0x0b; ///< Delay mode for this sound object (uint32 enum).

// ─── Function groups (named loudspeaker groups) ───────────────────────────────
static constexpr BoxAndObjNo FunctionGroup_Box            = 0x0e; ///< Box number for per-function-group parameters (groups 1–32).
static constexpr BoxAndObjNo FunctionGroup_Name           = 0x01; ///< User-assignable function group name (string).
static constexpr BoxAndObjNo FunctionGroup_Delay          = 0x02; ///< Group delay in ms (float32).
static constexpr BoxAndObjNo FunctionGroup_SpreadFactor   = 0x06; ///< Group spread factor (float32).

// ─── En-Space reverb inputs ───────────────────────────────────────────────────
static constexpr BoxAndObjNo ReverbInput_Box    = 0x10; ///< Box number for per-(sound-object × reverb-zone) send parameters.
static constexpr BoxAndObjNo ReverbInput_Gain   = 0x01; ///< Send gain from sound object to reverb zone (float32, dB).

// ─── En-Space reverb input processing ────────────────────────────────────────
static constexpr BoxAndObjNo ReverbInputProcessing_Box          = 0x11; ///< Box number for per-reverb-send processing parameters.
static constexpr BoxAndObjNo ReverbInputProcessing_Mute         = 0x01; ///< Reverb send mute (bool).
static constexpr BoxAndObjNo ReverbInputProcessing_Gain         = 0x02; ///< Reverb send gain trim (float32, dB).
static constexpr BoxAndObjNo ReverbInputProcessing_EqEnable     = 0x03; ///< Reverb send EQ enable (bool).
static constexpr BoxAndObjNo ReverbInputProcessing_LevelMeter   = 0x05; ///< Reverb send level meter reading (float32, dBFS).

// ─── Scene management ─────────────────────────────────────────────────────────
static constexpr BoxAndObjNo Scene_Box              = 0x17; ///< Box number for scene-related read/write objects.
static constexpr BoxAndObjNo Scene_SceneIndex       = 0x01; ///< Currently active scene index (uint32).
static constexpr BoxAndObjNo Scene_SceneName        = 0x03; ///< Name of the currently active scene (string).
static constexpr BoxAndObjNo Scene_SceneComment     = 0x04; ///< Comment text for the currently active scene (string).

// ─── Sound object routing (per function-group) ───────────────────────────────
static constexpr BoxAndObjNo SoundObjectRouting_Box     = 0x18; ///< Box number for per-(sound-object × function-group) routing parameters.
static constexpr BoxAndObjNo SoundObjectRouting_Mute    = 0x01; ///< Routing mute for this sound-object/function-group pair (bool).
static constexpr BoxAndObjNo SoundObjectRouting_Gain    = 0x02; ///< Routing gain for this sound-object/function-group pair (float32, dB).

// ─── Loudspeaker positioning (stack 1+, firmware ≥ DB000CD0) ─────────────────
static constexpr BoxAndObjNo Positioning_Speaker_Box        = 0x1a; ///< Box number for per-loudspeaker 6-DOF position parameters. @note Introduced with firmware GUID `DB000CD0`; use `Positioning_Source_Speaker_Position` on earlier firmware.
static constexpr BoxAndObjNo Positioning_Speaker_Group      = 0x01; ///< Function-group assignment for this loudspeaker output (uint32). @note Introduced with firmware GUID `DB000CD0`.
static constexpr BoxAndObjNo Positioning_Speaker_Position   = 0x02; ///< Loudspeaker 6-DOF position: [hor, vert, rot, x, y, z] (6 × float32 blob). @note Introduced with firmware GUID `DB000CD0`.

// ─── Scene agent ONo ──────────────────────────────────────────────────────────
/**
 * @brief Fixed ONo for the DS100 Scene Agent object.
 *
 * Scene recall is sent via a `SetValueCommand` to this specific ONo rather than
 * to a property addressed through the normal box/object-number scheme.
 * `DeviceController` remaps the `Scene_Recall` / `Scene_Previous` / `Scene_Next`
 * remote objects to this ONo via `GetObjectDefinition()`.
 */
static constexpr std::uint32_t  SceneAgentONo   = 0x2714;


/**
 * @struct dbOcaObjectDef_Fixed_HardwareVariant
 * @brief OCA definition for the DS100 hardware-variant identifier (read-only int32).
 */
struct dbOcaObjectDef_Fixed_HardwareVariant : Ocp1CommandDefinition
{
    dbOcaObjectDef_Fixed_HardwareVariant()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_HardwareVariant), // ONO of Fixed_HardwareVariant,
            OCP1DATATYPE_INT32,     // Value type
            DefLevel_OcaInt32Sensor,
            1)                      // Prop_Reading
    {
    }
};

/**
 * @struct dbOcaObjectDef_Fixed_SerNr
 * @brief OCA definition for the DS100 serial number string (read-only).
 */
struct dbOcaObjectDef_Fixed_SerNr : Ocp1CommandDefinition
{
    dbOcaObjectDef_Fixed_SerNr()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_SerNr), // ONO of Fixed_SerNr,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};

/**
 * @struct dbOcaObjectDef_Fixed_GUID
 * @brief OCA definition for the DS100 firmware/model GUID string (read-only).
 *
 * The GUID is an 8-character ASCII hex string that encodes hardware model and
 * firmware version.  `DeviceController` reads this immediately after TCP connect
 * to determine which OCA object definitions (stack-ident) to use:
 *
 * | Characters | Meaning |
 * |---|---|
 * | 0–3 | Always `"DB00"` — d&b manufacturer prefix. |
 * | 4–5 | Firmware version code (hex byte); compared against thresholds per model. |
 * | 6–7 | Hardware model: `"D0"` = DS100, `"D1"` = DS100D, `"D2"` = DS100M. |
 *
 * Example: `"DB000CD0"` = DS100, firmware version 0x0C = stack-ident 1.
 */
struct dbOcaObjectDef_Fixed_GUID : Ocp1CommandDefinition
{
    dbOcaObjectDef_Fixed_GUID()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, Fixed_Box, Fixed_GUID), // ONO of Fixed_GUID,
            OCP1DATATYPE_STRING,    // Value type
            DefLevel_OcaStringActuator,
            1)                      // Prop_Setting
    {
    }
};

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
 * @struct dbOcaObjectDef_CoordinateMapping_Source_Position
 * @brief OCA definition for a sound object's position in a coordinate mapping area's virtual space.
 *
 * The DS100 supports up to 4 independent coordinate mapping areas, each with its own
 * corner-point transform (defined via `CoordinateMappingSettings_P*_real/virtual`).
 * A sound object's position in a mapping area's virtual space is independent of its
 * real-world position (`dbOcaObjectDef_Positioning_Source_Position`).
 *
 * The value is a 3 × float32 blob (XYZ, normalised 0–1 in virtual space).
 * Use `Variant::ToPosition()` to decode.
 *
 * @param record   Mapping area index (1–4).
 * @param channel  1-based sound object (matrix input) index.
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
 * @struct dbOcaObjectDef_Positioning_Source_Position
 * @brief OCA definition for a sound object's 3D position in real-world space (En-Scene).
 *
 * This is the primary parameter read and written by `DeviceController` for every
 * active sound object in Umsci.  The value is encoded as three big-endian IEEE 754
 * float32s (12 bytes total) representing normalised X, Y, Z coordinates in the range
 * [0.0, 1.0].  Use `Variant::ToPosition()` to decode the blob on receipt.
 *
 * @param channel  1-based sound object (matrix input) index.
 *
 * ### Example
 * ```cpp
 * DS100::dbOcaObjectDef_Positioning_Source_Position def(5); // sound object 5
 *
 * // Subscribe:
 * uint32_t h;
 * client->sendData(Ocp1CommandResponseRequired(def.AddSubscriptionCommand(), h).GetSerializedData());
 *
 * // Set position to (0.5, 0.5, 0.0):
 * Variant pos(0.5f, 0.5f, 0.0f);
 * client->sendData(Ocp1CommandResponseRequired(def.SetValueCommand(pos), h).GetSerializedData());
 *
 * // Decode incoming notification:
 * Variant v(notif->GetParameterData(), OCP1DATATYPE_BLOB);
 * auto [x, y, z] = v.ToPosition();
 * ```
 */
struct dbOcaObjectDef_Positioning_Source_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Position(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_Position), // ONO of Positioning_Source_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            DefLevel_dbOcaPositionAgentDeprecated,
            1)                        // Prop_Position
    {
    }
};

/**
 * Positioning_Source_Enable
 */
struct dbOcaObjectDef_Positioning_Source_Enable : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Enable(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_Enable), // ONO of Positioning_Source_Enable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * Positioning_Source_Spread
 */
struct dbOcaObjectDef_Positioning_Source_Spread : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Spread(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_Spread), // ONO of Positioning_Source_Spread
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaFloat32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * Positioning_Source_Speaker_Group
 */
struct dbOcaObjectDef_Positioning_Source_Speaker_Group : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Speaker_Group(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_Speaker_Group), // ONO of Positioning_Source_Speaker_Group
            OCP1DATATYPE_INT32,             // Value type
            DefLevel_OcaInt32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * @struct dbOcaObjectDef_Positioning_Source_Speaker_Position
 * @brief OCA definition for loudspeaker 6-DOF position — **legacy firmware only** (stack-ident 0).
 *
 * On DS100 firmware earlier than `DB000CD0` (stack-ident 0) loudspeaker positions are
 * stored under `Positioning_Source_Box`.  On newer firmware (stack-ident 1) use
 * `dbOcaObjectDef_Positioning_Speaker_Position` instead.
 *
 * `DeviceController::CreateKnownONosMap()` initially populates speaker-position entries
 * with this definition, then `ProcessGuidAndSubscribe()` patches them to the newer
 * definition if the detected firmware version calls for stack-ident 1.
 *
 * The value is a 6 × float32 blob decoded by `Variant::ToAimingAndPosition()` as
 * [hor, vert, rot, x, y, z].
 *
 * @param channel  1-based output (loudspeaker) channel index.
 * @deprecated Use `dbOcaObjectDef_Positioning_Speaker_Position` for firmware ≥ `DB000CD0`.
 */
struct dbOcaObjectDef_Positioning_Source_Speaker_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_Speaker_Position(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_Speaker_Position), // ONO of Positioning_Source_Speaker_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            DefLevel_dbOcaSpeakerPositionAgentDeprecated,
            1)                        // Prop_Aiming_and_Position
    {
    }
};

/**
 * Positioning_Source_DelayMode
 */
struct dbOcaObjectDef_Positioning_Source_DelayMode : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Source_DelayMode(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Source_Box, Positioning_Source_DelayMode), // ONO of Positioning_Source_DelayMode
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};


/**
 * FunctionGroup_Name
 */
struct dbOcaObjectDef_FunctionGroup_Name : Ocp1CommandDefinition
{
    dbOcaObjectDef_FunctionGroup_Name(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, FunctionGroup_Box, FunctionGroup_Name), // ONO of FunctionGroup_Name
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
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, FunctionGroup_Box, FunctionGroup_SpreadFactor), // ONO of FunctionGroup_SpreadFactor
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
 * MatrixInput_LevelMeterIn
 */
struct dbOcaObjectDef_MatrixInput_LevelMeterIn : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_LevelMeterIn(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_LevelMeterIn), // ONO of MatrixInput_LevelMeterIn
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaLevelSensor,
            1)                              // Prop_Level
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
            DefLevel_OcaLevelSensor,
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
            DefLevel_OcaLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixInput_ISP
 */
struct dbOcaObjectDef_MatrixInput_ISP : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixInput_ISP(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixInput_Box, MatrixInput_ISP), // ONO of MatrixInput_ISP,
            OCP1DATATYPE_BOOLEAN,   // Value type
            DefLevel_OcaBooleanSensor,
            1)                      // Prop_Reading
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
 * MatrixOutput_LevelMeterIn
 */
struct dbOcaObjectDef_MatrixOutput_LevelMeterIn : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_LevelMeterIn(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_LevelMeterIn), // ONO of MatrixOutput_LevelMeterIn
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixOutput_LevelMeterPreMute
 */
struct dbOcaObjectDef_MatrixOutput_LevelMeterPreMute : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_LevelMeterPreMute), // ONO of MatrixOutput_LevelMeterPreMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaLevelSensor,
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
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_LevelMeterPostMute), // ONO of MatrixOutput_LevelMeterPostMute
            OCP1DATATYPE_FLOAT32,           // Value type
            DefLevel_OcaLevelSensor,
            1)                              // Prop_Level
    {
    }
};

/**
 * MatrixOutput_OSP
 */
struct dbOcaObjectDef_MatrixOutput_OSP : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixOutput_OSP(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, MatrixOutput_Box, MatrixOutput_OSP), // ONO of MatrixOutput_OSP,
            OCP1DATATYPE_BOOLEAN,   // Value type
            DefLevel_OcaBooleanSensor,
            1)                      // Prop_Reading
    {
    }
};

/**
 * MatrixSettings_PositioningEnable
 */
struct dbOcaObjectDef_MatrixSettings_PositioningEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixSettings_PositioningEnable()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_PositioningEnable), // ONO of MatrixSettings_PositioningEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
    {
    }
};

/**
 * MatrixSettings_ReverbEnable
 */
struct dbOcaObjectDef_MatrixSettings_ReverbEnable : Ocp1CommandDefinition
{
    dbOcaObjectDef_MatrixSettings_ReverbEnable()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbEnable), // ONO of MatrixSettings_ReverbEnable
            OCP1DATATYPE_UINT16,    // Value type
            DefLevel_OcaSwitch,
            1)                      // Prop_Position
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
 * MatrixSettings_ReverbRoomID - specialization to access the switche's position names
 */
struct dbOcaObjDef_MatrixSettings_ReverbRoomIdNames : Ocp1CommandDefinition
{
    dbOcaObjDef_MatrixSettings_ReverbRoomIdNames()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId), // ONO of MatrixSettings_ReverbRoomId
            OCP1DATATYPE_BLOB,      // Actual datatype is OcaList<OcaString>
            DefLevel_OcaSwitch,
            2)                      // Prop_Position_Names
    {
    }

    Ocp1CommandDefinition GetValueCommand() const override
    {
        return Ocp1CommandDefinition(m_targetOno,
            m_propertyType,
            m_propertyDefLevel,
            5,                                 // GetPositionNames has MethodIdx 5
            0,                                 // GetPositionNames needs 0 input params
            ByteVector());      // Empty parameters
    }

    dbOcaObjDef_MatrixSettings_ReverbRoomIdNames* Clone() const override
    {
        return new dbOcaObjDef_MatrixSettings_ReverbRoomIdNames(*this);
    }
};

/**
 * MatrixSettings_ReverbRoomID - specialization to access the switche's position enabled values
 */
struct dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds : Ocp1CommandDefinition
{
    dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds()
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, 0x00, MatrixSettings_Box, MatrixSettings_ReverbRoomId), // ONO of MatrixSettings_ReverbRoomId
            OCP1DATATYPE_BLOB,      // Actual datatype is OcaList<OcaBoolean>
            DefLevel_OcaSwitch,
            3)                      // Prop_Position_Enabled
    {
    }

    Ocp1CommandDefinition GetValueCommand() const override
    {
        return Ocp1CommandDefinition(m_targetOno,
            m_propertyType,
            m_propertyDefLevel,
            9,                                 // GetPositionEnableds has MethodIdx 9
            0,                                 // GetPositionEnableds needs 0 input params
            ByteVector());      // Empty parameters
    }

    dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds* Clone() const override
    {
        return new dbOcaObjDef_MatrixSettings_ReverbRoomIdEnableds(*this);
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
            DefLevel_OcaLevelSensor,
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
            DefLevel_OcaStringSensor,
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
            DefLevel_OcaStringSensor,
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
            DefLevel_OcaStringSensor,
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
 * Positioning_Speaker_Group
 */
struct dbOcaObjectDef_Positioning_Speaker_Group : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Speaker_Group(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Speaker_Box, Positioning_Speaker_Group), // ONO of Positioning_Speaker_Group
            OCP1DATATYPE_INT32,             // Value type
            DefLevel_OcaInt32Actuator,
            1)                              // Prop_Setting
    {
    }
};

/**
 * @struct dbOcaObjectDef_Positioning_Speaker_Position
 * @brief OCA definition for loudspeaker 6-DOF position — firmware ≥ `DB000CD0` (stack-ident 1).
 *
 * Replaces `dbOcaObjectDef_Positioning_Source_Speaker_Position` on DS100 hardware from
 * firmware `DB000CD0` onwards (DS100 ≥ 0x0C, DS100D all, DS100M ≥ 0x02).
 *
 * The value is a 6 × float32 blob in the order [hor, vert, rot, x, y, z],
 * decoded by `Variant::ToAimingAndPosition()`.
 * Umsci converts this to a `std::array<float, 6>` for loudspeaker visualisation.
 *
 * `DeviceController::ProcessGuidAndSubscribe()` patches the speaker-position entries
 * in `m_ROIsToDefsMap` from the legacy to this definition when stack-ident 1 is detected.
 *
 * @param channel  1-based output (loudspeaker) channel index.
 */
struct dbOcaObjectDef_Positioning_Speaker_Position : Ocp1CommandDefinition
{
    dbOcaObjectDef_Positioning_Speaker_Position(std::uint32_t channel)
        : Ocp1CommandDefinition(GetONoTy2(0x02, 0x00, channel, Positioning_Speaker_Box, Positioning_Speaker_Position), // ONO of Positioning_Speaker_Position
            OCP1DATATYPE_DB_POSITION, // Value type
            DefLevel_dbOcaSpeakerPositionAgentDeprecated,
            1)                        // Prop_Aiming_and_Position
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
        ByteVector newParamData = DataFromUint32(newValue);

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