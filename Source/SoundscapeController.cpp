/* Copyright (c) 2026, Christian Ahrens
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

#include "SoundscapeController.h"

#include "Ocp1DataTypes.h"
#include "Ocp1DS100ObjectDefinitions.h"
#include "Ocp1Message.h"

#include <algorithm>


namespace NanoOcp1
{


// ── Construction / destruction ────────────────────────────────────────────────

SoundscapeController::SoundscapeController(bool callbacksOnMessageThread)
    : Ocp1Controller(callbacksOnMessageThread)
{
    createKnownONosMap();
}

SoundscapeController::~SoundscapeController() = default;


// ── Configuration ─────────────────────────────────────────────────────────────

void SoundscapeController::setDeviceIOSize(std::uint16_t inputs, std::uint16_t outputs)
{
    m_activeInputChannelCount  = std::clamp(inputs,  std::uint16_t(1), sc_MAX_INPUT_CHANNELS);
    m_activeOutputChannelCount = std::clamp(outputs, std::uint16_t(1), sc_MAX_OUTPUT_CHANNELS);
    createKnownONosMap();
}

bool SoundscapeController::setActiveRemoteObjects(const std::vector<RemoteObject>& objs)
{
    if (getState() != State::Disconnected)
        return false;
    m_activeRemoteObjects = objs;
    rebuildTrackedObjects();
    return true;
}

const std::vector<SoundscapeController::RemoteObject>& SoundscapeController::getActiveRemoteObjects() const
{
    return m_activeRemoteObjects;
}


// ── SetValue ──────────────────────────────────────────────────────────────────

bool SoundscapeController::setObjectValue(const RemoteObject& obj)
{
    auto defOpt = getObjectDefinition(obj.Id, obj.Addr, /*useRemapping=*/true);
    if (!defOpt || !*defOpt)
        return false;
    return setValue(**defOpt, obj.Var);
}


// ── Connection lifecycle ──────────────────────────────────────────────────────

void SoundscapeController::afterConnected()
{
    // Reset per-connection device state.
    m_deviceGuid   = "";
    m_stackIdent   = -1;
    m_connectedModel = DbDeviceModel::Invalid;

    // Query Fixed_GUID first.  Subscriptions follow in onUntrackedGetValueResponse()
    // once we know which OCA revision the firmware supports.
    DS100::dbOcaObjectDef_Fixed_GUID guidDef;
    queryObjectValue(guidDef);
}

void SoundscapeController::onUntrackedGetValueResponse(std::uint32_t ono,
                                                   const ByteVector& paramData)
{
    DS100::dbOcaObjectDef_Fixed_GUID guidDef;
    if (ono != guidDef.m_targetOno)
        return;

    bool ok = false;
    auto guid = DataToString(paramData, &ok);
    if (ok)
        processGuidAndSubscribe(guid);
}


// ── GUID processing ───────────────────────────────────────────────────────────

/**
 * Called on the socket thread when the Fixed_GUID GetValue response arrives.
 *
 * Sequence:
 * 1. Guard against re-processing the same GUID (e.g. periodic device notification).
 * 2. Validate the GUID and detect model + OCA stack ident.
 * 3. Patch speaker-position definitions in m_ROIsToDefsMap to match the revision.
 * 4. Rebuild tracked objects from the now-correct map.
 * 5. Trigger subscribe + query.  The resulting pending handles prevent the base-class
 *    processMessage() from advancing to Connected until all responses arrive.
 */
void SoundscapeController::processGuidAndSubscribe(const std::string& guid)
{
    if (guid == m_deviceGuid) // already handled this GUID for the current connection
        return;

    if (!setOcaRevisionAndDeviceModel(guid))
        return;
    m_deviceGuid = guid;

    // Patch the speaker-position map entries to match the detected OCA revision.
    RemObjAddr roa;
    if (m_stackIdent >= 1)
    {
        for (std::uint16_t ch = 1; ch <= m_activeOutputChannelCount; ++ch)
        {
            roa.pri = ch;
            m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa]
                = DS100::dbOcaObjectDef_Positioning_Speaker_Position(ch);
            m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerGroup][roa]
                = DS100::dbOcaObjectDef_Positioning_Speaker_Group(ch);
        }
    }
    else
    {
        for (std::uint16_t ch = 1; ch <= m_activeOutputChannelCount; ++ch)
        {
            roa.pri = ch;
            m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa]
                = DS100::dbOcaObjectDef_Positioning_Source_Speaker_Position(ch);
        }
    }

    // Rebuild the reverse-lookup map to include the patched ONos.
    m_ONoToROIMap.clear();
    for (auto& roiKV : m_ROIsToDefsMap)
        for (auto& defKV : roiKV.second)
            m_ONoToROIMap[defKV.second.m_targetOno] = { roiKV.first, defKV.first };

    // Rebuild tracked objects so they reference the correct definitions.
    rebuildTrackedObjects();

    createObjectSubscriptions();
    queryObjectValues();
}

/**
 * Validates a GUID string and sets m_stackIdent / m_connectedModel.
 *
 * GUID format (8 ASCII hex characters):
 * - [0–3] "DB00" — d&b manufacturer prefix.
 * - [4–5] Firmware version code (hex); compared against thresholds per model.
 * - [6–7] Device model: "D0"=DS100, "D1"=DS110, "D2"=DS100M, "DA"=vCore.
 *
 * Stack-ident 0 = legacy OCA definitions; 1 = extended/scalable.
 */
bool SoundscapeController::setOcaRevisionAndDeviceModel(const std::string& guid)
{
    if (guid.size() != 8)
        return false;
    if (guid.substr(0, 4) != "DB00")
        return false;

    DbDeviceModel model;
    const auto suffix = guid.substr(6, 2);
    if      (suffix == "D0") model = DbDeviceModel::DS100;
    else if (suffix == "D1") model = DbDeviceModel::DS110;
    else if (suffix == "D2") model = DbDeviceModel::DS100M;
    else if (suffix == "DA") model = DbDeviceModel::vCore;
    else return false;

    const auto versionChars = guid.substr(4, 2);
    int stackIdent = 0;
    switch (model)
    {
    case DbDeviceModel::DS100:
        stackIdent = (versionChars >= "0C") ? 1 : 0;
        break;
    case DbDeviceModel::DS110:
        stackIdent = 1; // pre-release firmware without scalability was never released
        break;
    case DbDeviceModel::DS100M:
        stackIdent = (versionChars >= "02") ? 1 : 0;
        break;
    case DbDeviceModel::vCore:
        stackIdent = 1;
        break;
    default:
        return false;
    }

    m_connectedModel = model;
    m_stackIdent     = stackIdent;
    return true;
}


// ── Tracked-object management ─────────────────────────────────────────────────

/**
 * Rebuild the base-class tracked-object list from m_activeRemoteObjects.
 *
 * For each active object whose ROI+addr is present in m_ROIsToDefsMap, registers
 * a ValueCallback that decodes the raw OCA parameter bytes into a typed Variant
 * and delivers a populated RemoteObject to onRemoteObjectReceived.
 *
 * ROIs that have no OCA counterpart (X/Y/XY split views, Scene_Prev/Next/Recall)
 * are silently skipped — they are not in m_ROIsToDefsMap.
 *
 * Called while Disconnected (before connect()) and again from processGuidAndSubscribe()
 * once the correct speaker-position definitions are known.
 */
void SoundscapeController::rebuildTrackedObjects()
{
    clearTrackedObjects();

    for (const auto& obj : m_activeRemoteObjects)
    {
        auto roiIt = m_ROIsToDefsMap.find(obj.Id);
        if (roiIt == m_ROIsToDefsMap.end())
            continue;
        auto defIt = roiIt->second.find(obj.Addr);
        if (defIt == roiIt->second.end())
            continue;

        const RemoteObject::RemObjIdent roi = obj.Id;
        const RemObjAddr                addr = obj.Addr;
        const Ocp1DataType              dt   = dataTypeForRoi(roi);

        auto defCopy = std::unique_ptr<Ocp1CommandDefinition>(defIt->second.Clone());

        trackObject(std::move(defCopy), [this, roi, addr, dt](const ByteVector& data) {
            Variant val(data, dt);
            RemoteObject ro(roi, addr, std::move(val));
            if (onRemoteObjectReceived)
                onRemoteObjectReceived(ro);
        });
    }
}


// ── OCA object definition lookup ──────────────────────────────────────────────

/**
 * Returns a freshly heap-allocated Ocp1CommandDefinition for the given ROI+addr.
 *
 * With useRemapping=true:
 * - Positioning_SourcePosition_X/Y/XY → Positioning_SourcePosition (XYZ)
 * - CoordinateMapping_SourcePosition_X/Y/XY → CoordinateMapping_SourcePosition
 * - Scene_Previous/Next/Recall → SceneAgent
 * These convenience identifiers map to a single underlying OCA object.
 *
 * Positioning_SpeakerPosition returns the revision-appropriate definition based
 * on m_stackIdent (legacy vs. extended OCA object).
 */
std::optional<std::unique_ptr<Ocp1CommandDefinition>>
SoundscapeController::getObjectDefinition(RemoteObject::RemObjIdent roi,
                                      const RemObjAddr& addr,
                                      bool useRemapping) const
{
    const auto first  = static_cast<std::int32_t>(addr.pri);
    const auto second = static_cast<std::int32_t>(addr.sec);

    using namespace DS100;
    switch (roi)
    {
    case RemoteObject::Fixed_GUID:
        return std::make_unique<dbOcaObjectDef_Fixed_GUID>();
    case RemoteObject::Settings_DeviceName:
        return std::make_unique<dbOcaObjectDef_Settings_DeviceName>();
    case RemoteObject::Status_StatusText:
        return std::make_unique<dbOcaObjectDef_Status_StatusText>();
    case RemoteObject::Status_AudioNetworkSampleStatus:
        return std::make_unique<dbOcaObjectDef_Status_AudioNetworkSampleStatus>();
    case RemoteObject::Error_GnrlErr:
        return std::make_unique<dbOcaObjectDef_Error_GnrlErr>();
    case RemoteObject::Error_ErrorText:
        return std::make_unique<dbOcaObjectDef_Error_ErrorText>();
    case RemoteObject::CoordinateMappingSettings_Name:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_Name>(first);
    case RemoteObject::CoordinateMappingSettings_Flip:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_Flip>(first);
    case RemoteObject::CoordinateMappingSettings_P1real:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P1_real>(first);
    case RemoteObject::CoordinateMappingSettings_P2real:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P2_real>(first);
    case RemoteObject::CoordinateMappingSettings_P3real:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P3_real>(first);
    case RemoteObject::CoordinateMappingSettings_P4real:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P4_real>(first);
    case RemoteObject::CoordinateMappingSettings_P1virtual:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P1_virtual>(first);
    case RemoteObject::CoordinateMappingSettings_P3virtual:
        return std::make_unique<dbOcaObjectDef_CoordinateMappingSettings_P3_virtual>(first);

    // XY/X/Y split variants — only resolvable when remapping is requested
    case RemoteObject::Positioning_SourcePosition_XY:
    case RemoteObject::Positioning_SourcePosition_X:
    case RemoteObject::Positioning_SourcePosition_Y:
        if (!useRemapping) return {};
        [[fallthrough]];
    case RemoteObject::Positioning_SourcePosition:
        return std::make_unique<dbOcaObjectDef_Positioning_Source_Position>(first);

    case RemoteObject::CoordinateMapping_SourcePosition_XY:
    case RemoteObject::CoordinateMapping_SourcePosition_X:
    case RemoteObject::CoordinateMapping_SourcePosition_Y:
        if (!useRemapping) return {};
        [[fallthrough]];
    case RemoteObject::CoordinateMapping_SourcePosition:
        return std::make_unique<dbOcaObjectDef_CoordinateMapping_Source_Position>(second, first);

    case RemoteObject::Positioning_SourceSpread:
        return std::make_unique<dbOcaObjectDef_Positioning_Source_Spread>(first);
    case RemoteObject::Positioning_SourceDelayMode:
        return std::make_unique<dbOcaObjectDef_Positioning_Source_DelayMode>(first);
    case RemoteObject::Positioning_SourceEnable:
        return std::make_unique<dbOcaObjectDef_Positioning_Source_Enable>(first);
    case RemoteObject::Positioning_SpeakerPosition:
        if (m_stackIdent >= 1)
            return std::make_unique<dbOcaObjectDef_Positioning_Speaker_Position>(first);
        else
            return std::make_unique<dbOcaObjectDef_Positioning_Source_Speaker_Position>(first);
    case RemoteObject::Positioning_SpeakerGroup:
        return std::make_unique<dbOcaObjectDef_Positioning_Speaker_Group>(first);
    case RemoteObject::FunctionGroup_Name:
        return std::make_unique<dbOcaObjectDef_FunctionGroup_Name>(first);
    case RemoteObject::FunctionGroup_Delay:
        return std::make_unique<dbOcaObjectDef_FunctionGroup_Delay>(first);
    case RemoteObject::FunctionGroup_Mode:
        return std::make_unique<dbOcaObjectDef_FunctionGroup_Mode>(first);
    case RemoteObject::FunctionGroup_SpreadFactor:
        return std::make_unique<dbOcaObjectDef_FunctionGroup_SpreadFactor>(first);
    case RemoteObject::MatrixInput_Mute:
        return std::make_unique<dbOcaObjectDef_MatrixInput_Mute>(first);
    case RemoteObject::MatrixInput_Gain:
        return std::make_unique<dbOcaObjectDef_MatrixInput_Gain>(first);
    case RemoteObject::MatrixInput_Delay:
        return std::make_unique<dbOcaObjectDef_MatrixInput_Delay>(first);
    case RemoteObject::MatrixInput_DelayEnable:
        return std::make_unique<dbOcaObjectDef_MatrixInput_DelayEnable>(first);
    case RemoteObject::MatrixInput_EqEnable:
        return std::make_unique<dbOcaObjectDef_MatrixInput_EqEnable>(first);
    case RemoteObject::MatrixInput_Polarity:
        return std::make_unique<dbOcaObjectDef_MatrixInput_Polarity>(first);
    case RemoteObject::MatrixInput_ChannelName:
        return std::make_unique<dbOcaObjectDef_MatrixInput_ChannelName>(first);
    case RemoteObject::MatrixInput_LevelMeterPreMute:
        return std::make_unique<dbOcaObjectDef_MatrixInput_LevelMeterPreMute>(first);
    case RemoteObject::MatrixInput_LevelMeterPostMute:
        return std::make_unique<dbOcaObjectDef_MatrixInput_LevelMeterPostMute>(first);
    case RemoteObject::MatrixInput_ReverbSendGain:
        return std::make_unique<dbOcaObjectDef_MatrixInput_ReverbSendGain>(first);
    case RemoteObject::MatrixNode_Enable:
        return std::make_unique<dbOcaObjectDef_MatrixNode_Enable>(first, second);
    case RemoteObject::MatrixNode_Gain:
        return std::make_unique<dbOcaObjectDef_MatrixNode_Gain>(first, second);
    case RemoteObject::MatrixNode_Delay:
        return std::make_unique<dbOcaObjectDef_MatrixNode_Delay>(first, second);
    case RemoteObject::MatrixNode_DelayEnable:
        return std::make_unique<dbOcaObjectDef_MatrixNode_DelayEnable>(first, second);
    case RemoteObject::MatrixOutput_Mute:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_Mute>(first);
    case RemoteObject::MatrixOutput_Gain:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_Gain>(first);
    case RemoteObject::MatrixOutput_Delay:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_Delay>(first);
    case RemoteObject::MatrixOutput_DelayEnable:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_DelayEnable>(first);
    case RemoteObject::MatrixOutput_EqEnable:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_EqEnable>(first);
    case RemoteObject::MatrixOutput_Polarity:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_Polarity>(first);
    case RemoteObject::MatrixOutput_ChannelName:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_ChannelName>(first);
    case RemoteObject::MatrixOutput_LevelMeterPreMute:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_LevelMeterPreMute>(first);
    case RemoteObject::MatrixOutput_LevelMeterPostMute:
        return std::make_unique<dbOcaObjectDef_MatrixOutput_LevelMeterPostMute>(first);
    case RemoteObject::MatrixSettings_ReverbRoomId:
        return std::make_unique<dbOcaObjectDef_MatrixSettings_ReverbRoomId>();
    case RemoteObject::MatrixSettings_ReverbPredelayFactor:
        return std::make_unique<dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor>();
    case RemoteObject::MatrixSettings_ReverbRearLevel:
        return std::make_unique<dbOcaObjectDef_MatrixSettings_ReverbRearLevel>();
    case RemoteObject::ReverbInput_Gain:
        return std::make_unique<dbOcaObjectDef_ReverbInput_Gain>(second, first);
    case RemoteObject::ReverbInputProcessing_Mute:
        return std::make_unique<dbOcaObjectDef_ReverbInputProcessing_Mute>(first);
    case RemoteObject::ReverbInputProcessing_Gain:
        return std::make_unique<dbOcaObjectDef_ReverbInputProcessing_Gain>(first);
    case RemoteObject::ReverbInputProcessing_EqEnable:
        return std::make_unique<dbOcaObjectDef_ReverbInputProcessing_EqEnable>(first);
    case RemoteObject::ReverbInputProcessing_LevelMeter:
        return std::make_unique<dbOcaObjectDef_ReverbInputProcessing_LevelMeter>(first);
    case RemoteObject::Scene_SceneIndex:
        return std::make_unique<dbOcaObjectDef_Scene_SceneIndex>();
    case RemoteObject::Scene_SceneName:
        return std::make_unique<dbOcaObjectDef_Scene_SceneName>();
    case RemoteObject::Scene_SceneComment:
        return std::make_unique<dbOcaObjectDef_Scene_SceneComment>();
    case RemoteObject::Scene_Previous:
    case RemoteObject::Scene_Next:
    case RemoteObject::Scene_Recall:
        if (!useRemapping) return {};
        return std::make_unique<dbOcaObjectDef_SceneAgent>();
    case RemoteObject::SoundObjectRouting_Mute:
        return std::make_unique<dbOcaObjectDef_SoundObjectRouting_Mute>(second, first);
    case RemoteObject::SoundObjectRouting_Gain:
        return std::make_unique<dbOcaObjectDef_SoundObjectRouting_Gain>(second, first);
    default:
        return {};
    }
}


// ── Data-type mapping ─────────────────────────────────────────────────────────

Ocp1DataType SoundscapeController::dataTypeForRoi(RemoteObject::RemObjIdent roi)
{
    switch (roi)
    {
    case RemoteObject::Error_GnrlErr:
    case RemoteObject::MatrixInput_Polarity:
    case RemoteObject::MatrixOutput_Polarity:
    case RemoteObject::MatrixInput_Mute:
    case RemoteObject::MatrixOutput_Mute:
    case RemoteObject::ReverbInputProcessing_Mute:
    case RemoteObject::SoundObjectRouting_Mute:
        return OCP1DATATYPE_UINT8;

    case RemoteObject::CoordinateMappingSettings_Flip:
    case RemoteObject::MatrixNode_Enable:
    case RemoteObject::MatrixNode_DelayEnable:
    case RemoteObject::MatrixInput_DelayEnable:
    case RemoteObject::MatrixInput_EqEnable:
    case RemoteObject::MatrixOutput_DelayEnable:
    case RemoteObject::MatrixOutput_EqEnable:
    case RemoteObject::Positioning_SourceDelayMode:
    case RemoteObject::Positioning_SourceEnable:
    case RemoteObject::MatrixSettings_ReverbRoomId:
    case RemoteObject::FunctionGroup_Mode:
    case RemoteObject::ReverbInputProcessing_EqEnable:
        return OCP1DATATYPE_UINT16;

    case RemoteObject::Status_AudioNetworkSampleStatus:
    case RemoteObject::Positioning_SpeakerGroup:
        return OCP1DATATYPE_INT32;

    case RemoteObject::MatrixNode_Delay:
    case RemoteObject::MatrixInput_Delay:
    case RemoteObject::MatrixOutput_Delay:
    case RemoteObject::FunctionGroup_Delay:
    case RemoteObject::MatrixNode_Gain:
    case RemoteObject::Positioning_SourceSpread:
    case RemoteObject::MatrixInput_ReverbSendGain:
    case RemoteObject::MatrixInput_Gain:
    case RemoteObject::MatrixInput_LevelMeterPreMute:
    case RemoteObject::MatrixInput_LevelMeterPostMute:
    case RemoteObject::MatrixOutput_Gain:
    case RemoteObject::MatrixOutput_LevelMeterPreMute:
    case RemoteObject::MatrixOutput_LevelMeterPostMute:
    case RemoteObject::MatrixSettings_ReverbPredelayFactor:
    case RemoteObject::MatrixSettings_ReverbRearLevel:
    case RemoteObject::FunctionGroup_SpreadFactor:
    case RemoteObject::ReverbInput_Gain:
    case RemoteObject::ReverbInputProcessing_Gain:
    case RemoteObject::ReverbInputProcessing_LevelMeter:
    case RemoteObject::SoundObjectRouting_Gain:
        return OCP1DATATYPE_FLOAT32;

    case RemoteObject::CoordinateMappingSettings_Name:
    case RemoteObject::Settings_DeviceName:
    case RemoteObject::Status_StatusText:
    case RemoteObject::Error_ErrorText:
    case RemoteObject::MatrixInput_ChannelName:
    case RemoteObject::MatrixOutput_ChannelName:
    case RemoteObject::Scene_SceneIndex:
    case RemoteObject::Scene_SceneName:
    case RemoteObject::Scene_SceneComment:
    case RemoteObject::FunctionGroup_Name:
    case RemoteObject::Fixed_GUID:
        return OCP1DATATYPE_STRING;

    case RemoteObject::CoordinateMapping_SourcePosition:
    case RemoteObject::Positioning_SpeakerPosition:
    case RemoteObject::Positioning_SourcePosition:
    case RemoteObject::CoordinateMappingSettings_P1real:
    case RemoteObject::CoordinateMappingSettings_P2real:
    case RemoteObject::CoordinateMappingSettings_P3real:
    case RemoteObject::CoordinateMappingSettings_P4real:
    case RemoteObject::CoordinateMappingSettings_P1virtual:
    case RemoteObject::CoordinateMappingSettings_P3virtual:
        return OCP1DATATYPE_DB_POSITION;

    default:
        return OCP1DATATYPE_NONE;
    }
}


// ── Full ONo map construction ─────────────────────────────────────────────────

/**
 * Pre-builds m_ROIsToDefsMap and m_ONoToROIMap for the entire DS100 parameter space.
 *
 * Speaker-position entries are initially populated with stack-1 definitions.
 * processGuidAndSubscribe() patches them to the correct revision once the GUID
 * is received and the device model is known.
 */
void SoundscapeController::createKnownONosMap()
{
    using namespace DS100;

    m_ROIsToDefsMap.clear();
    m_ONoToROIMap.clear();

    // ── Unindexed objects (no channel/record) ────────────────────────────────
    m_ROIsToDefsMap[RemoteObject::Fixed_GUID][RemObjAddr()]                       = dbOcaObjectDef_Fixed_GUID();
    m_ROIsToDefsMap[RemoteObject::Settings_DeviceName][RemObjAddr()]              = dbOcaObjectDef_Settings_DeviceName();
    m_ROIsToDefsMap[RemoteObject::Status_StatusText][RemObjAddr()]                = dbOcaObjectDef_Status_StatusText();
    m_ROIsToDefsMap[RemoteObject::Status_AudioNetworkSampleStatus][RemObjAddr()]  = dbOcaObjectDef_Status_AudioNetworkSampleStatus();
    m_ROIsToDefsMap[RemoteObject::Error_GnrlErr][RemObjAddr()]                    = dbOcaObjectDef_Error_GnrlErr();
    m_ROIsToDefsMap[RemoteObject::Error_ErrorText][RemObjAddr()]                  = dbOcaObjectDef_Error_ErrorText();
    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbRoomId][RemObjAddr()]      = dbOcaObjectDef_MatrixSettings_ReverbRoomId();
    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbPredelayFactor][RemObjAddr()] = dbOcaObjectDef_MatrixSettings_ReverbPredelayFactor();
    m_ROIsToDefsMap[RemoteObject::MatrixSettings_ReverbRearLevel][RemObjAddr()]   = dbOcaObjectDef_MatrixSettings_ReverbRearLevel();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneIndex][RemObjAddr()]                 = dbOcaObjectDef_Scene_SceneIndex();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneName][RemObjAddr()]                  = dbOcaObjectDef_Scene_SceneName();
    m_ROIsToDefsMap[RemoteObject::Scene_SceneComment][RemObjAddr()]               = dbOcaObjectDef_Scene_SceneComment();

    // ── Per-input-channel (sound objects) ────────────────────────────────────
    for (std::int16_t ch = 1; ch <= m_activeInputChannelCount; ++ch)
    {
        RemObjAddr roa(ch, RemObjAddr::sc_INV);

        m_ROIsToDefsMap[RemoteObject::Positioning_SourcePosition][roa] = dbOcaObjectDef_Positioning_Source_Position(ch);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourceSpread][roa]   = dbOcaObjectDef_Positioning_Source_Spread(ch);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourceDelayMode][roa] = dbOcaObjectDef_Positioning_Source_DelayMode(ch);
        m_ROIsToDefsMap[RemoteObject::Positioning_SourceEnable][roa]   = dbOcaObjectDef_Positioning_Source_Enable(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Mute][roa]           = dbOcaObjectDef_MatrixInput_Mute(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Gain][roa]           = dbOcaObjectDef_MatrixInput_Gain(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Delay][roa]          = dbOcaObjectDef_MatrixInput_Delay(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_DelayEnable][roa]    = dbOcaObjectDef_MatrixInput_DelayEnable(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_EqEnable][roa]       = dbOcaObjectDef_MatrixInput_EqEnable(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_Polarity][roa]       = dbOcaObjectDef_MatrixInput_Polarity(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_ChannelName][roa]    = dbOcaObjectDef_MatrixInput_ChannelName(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_LevelMeterPreMute][roa]  = dbOcaObjectDef_MatrixInput_LevelMeterPreMute(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_LevelMeterPostMute][roa] = dbOcaObjectDef_MatrixInput_LevelMeterPostMute(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixInput_ReverbSendGain][roa] = dbOcaObjectDef_MatrixInput_ReverbSendGain(ch);

        // Per-channel × mapping-area
        for (std::uint16_t area = static_cast<std::uint16_t>(MappingAreaId::First);
             area <= static_cast<std::uint16_t>(MappingAreaId::Fourth); ++area)
        {
            roa.sec = static_cast<std::int16_t>(area);
            m_ROIsToDefsMap[RemoteObject::CoordinateMapping_SourcePosition][roa]
                = dbOcaObjectDef_CoordinateMapping_Source_Position(area, ch);
        }

        // Per-channel × function-group
        const auto numGroups = std::min(sc_MAX_FUNCTION_GROUPS, m_activeOutputChannelCount);
        for (std::uint16_t fg = 1; fg <= numGroups; ++fg)
        {
            roa.sec = static_cast<std::int16_t>(fg);
            m_ROIsToDefsMap[RemoteObject::SoundObjectRouting_Mute][roa]
                = dbOcaObjectDef_SoundObjectRouting_Mute(fg, ch);
            m_ROIsToDefsMap[RemoteObject::SoundObjectRouting_Gain][roa]
                = dbOcaObjectDef_SoundObjectRouting_Gain(fg, ch);
        }

        // Per-channel × output-channel (matrix nodes)
        for (std::uint16_t out = 1; out <= m_activeOutputChannelCount; ++out)
        {
            roa.sec = static_cast<std::int16_t>(out);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Enable][roa]      = dbOcaObjectDef_MatrixNode_Enable(ch, out);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Gain][roa]        = dbOcaObjectDef_MatrixNode_Gain(ch, out);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_Delay][roa]       = dbOcaObjectDef_MatrixNode_Delay(ch, out);
            m_ROIsToDefsMap[RemoteObject::MatrixNode_DelayEnable][roa] = dbOcaObjectDef_MatrixNode_DelayEnable(ch, out);
        }
    }

    // ── Per-output-channel (loudspeakers) ─────────────────────────────────────
    for (std::int16_t ch = 1; ch <= m_activeOutputChannelCount; ++ch)
    {
        RemObjAddr roa(ch, RemObjAddr::sc_INV);

        // Default to stack-1 definitions; patched in processGuidAndSubscribe() if needed.
        m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerPosition][roa] = dbOcaObjectDef_Positioning_Speaker_Position(ch);
        m_ROIsToDefsMap[RemoteObject::Positioning_SpeakerGroup][roa]    = dbOcaObjectDef_Positioning_Speaker_Group(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Mute][roa]           = dbOcaObjectDef_MatrixOutput_Mute(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Gain][roa]           = dbOcaObjectDef_MatrixOutput_Gain(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Delay][roa]          = dbOcaObjectDef_MatrixOutput_Delay(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_DelayEnable][roa]    = dbOcaObjectDef_MatrixOutput_DelayEnable(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_EqEnable][roa]       = dbOcaObjectDef_MatrixOutput_EqEnable(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_Polarity][roa]       = dbOcaObjectDef_MatrixOutput_Polarity(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_ChannelName][roa]    = dbOcaObjectDef_MatrixOutput_ChannelName(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_LevelMeterPreMute][roa]  = dbOcaObjectDef_MatrixOutput_LevelMeterPreMute(ch);
        m_ROIsToDefsMap[RemoteObject::MatrixOutput_LevelMeterPostMute][roa] = dbOcaObjectDef_MatrixOutput_LevelMeterPostMute(ch);
    }

    // ── Function groups ───────────────────────────────────────────────────────
    const auto numGroups = std::min(sc_MAX_FUNCTION_GROUPS, m_activeOutputChannelCount);
    for (std::int16_t fg = 1; fg <= numGroups; ++fg)
    {
        RemObjAddr roa(fg, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_Name][roa]        = dbOcaObjectDef_FunctionGroup_Name(fg);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_Delay][roa]       = dbOcaObjectDef_FunctionGroup_Delay(fg);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_Mode][roa]        = dbOcaObjectDef_FunctionGroup_Mode(fg);
        m_ROIsToDefsMap[RemoteObject::FunctionGroup_SpreadFactor][roa] = dbOcaObjectDef_FunctionGroup_SpreadFactor(fg);
    }

    // ── En-Space reverb zones ─────────────────────────────────────────────────
    for (std::int16_t zone = 1; zone <= sc_MAX_REVERB_ZONES; ++zone)
    {
        RemObjAddr roa(zone, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_Mute][roa]     = dbOcaObjectDef_ReverbInputProcessing_Mute(zone);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_Gain][roa]     = dbOcaObjectDef_ReverbInputProcessing_Gain(zone);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_EqEnable][roa] = dbOcaObjectDef_ReverbInputProcessing_EqEnable(zone);
        m_ROIsToDefsMap[RemoteObject::ReverbInputProcessing_LevelMeter][roa] = dbOcaObjectDef_ReverbInputProcessing_LevelMeter(zone);

        for (std::int16_t so = 1; so <= m_activeInputChannelCount; ++so)
        {
            roa.sec = so;
            m_ROIsToDefsMap[RemoteObject::ReverbInput_Gain][roa]
                = dbOcaObjectDef_ReverbInput_Gain(so, zone);
        }
    }

    // ── Coordinate mapping settings (mapping areas 1–4) ───────────────────────
    for (std::int16_t area = 1; area <= 4; ++area)
    {
        RemObjAddr roa(area, RemObjAddr::sc_INV);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P1real][roa]    = dbOcaObjectDef_CoordinateMappingSettings_P1_real(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P2real][roa]    = dbOcaObjectDef_CoordinateMappingSettings_P2_real(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P3real][roa]    = dbOcaObjectDef_CoordinateMappingSettings_P3_real(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P4real][roa]    = dbOcaObjectDef_CoordinateMappingSettings_P4_real(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P1virtual][roa] = dbOcaObjectDef_CoordinateMappingSettings_P1_virtual(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_P3virtual][roa] = dbOcaObjectDef_CoordinateMappingSettings_P3_virtual(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_Flip][roa]      = dbOcaObjectDef_CoordinateMappingSettings_Flip(area);
        m_ROIsToDefsMap[RemoteObject::CoordinateMappingSettings_Name][roa]      = dbOcaObjectDef_CoordinateMappingSettings_Name(area);
    }

    // Build reverse ONo → {ROI, addr} map from completed m_ROIsToDefsMap.
    for (const auto& roiKV : m_ROIsToDefsMap)
        for (const auto& defKV : roiKV.second)
            m_ONoToROIMap[defKV.second.m_targetOno] = { roiKV.first, defKV.first };
}


// ── Static helpers ────────────────────────────────────────────────────────────

std::string SoundscapeController::RemoteObject::GetObjectDescription(RemObjIdent roi)
{
    switch (roi)
    {
    case HeartbeatPing:                         return "PING";
    case HeartbeatPong:                         return "PONG";
    case Fixed_GUID:                            return "Fixed GUID";
    case Settings_DeviceName:                   return "Device Name";
    case Error_GnrlErr:                         return "General Error";
    case Error_ErrorText:                       return "Error Text";
    case Status_StatusText:                     return "Status Text";
    case Status_AudioNetworkSampleStatus:       return "Audio Network Sample Status";
    case MatrixInput_Mute:                      return "Matrix Input Mute";
    case MatrixInput_Gain:                      return "Matrix Input Gain";
    case MatrixInput_Delay:                     return "Matrix Input Delay";
    case MatrixInput_DelayEnable:               return "Matrix Input DelayEnable";
    case MatrixInput_EqEnable:                  return "Matrix Input EqEnable";
    case MatrixInput_Polarity:                  return "Matrix Input Polarity";
    case MatrixInput_ChannelName:               return "Matrix Input ChannelName";
    case MatrixInput_LevelMeterPreMute:         return "Matrix Input LevelMeterPreMute";
    case MatrixInput_LevelMeterPostMute:        return "Matrix Input LevelMeterPostMute";
    case MatrixInput_ReverbSendGain:            return "Matrix Input ReverbSendGain";
    case MatrixNode_Enable:                     return "Matrix Node Enable";
    case MatrixNode_Gain:                       return "Matrix Node Gain";
    case MatrixNode_DelayEnable:                return "Matrix Node DelayEnable";
    case MatrixNode_Delay:                      return "Matrix Node Delay";
    case MatrixOutput_Mute:                     return "Matrix Output Mute";
    case MatrixOutput_Gain:                     return "Matrix Output Gain";
    case MatrixOutput_Delay:                    return "Matrix Output Delay";
    case MatrixOutput_DelayEnable:              return "Matrix Output DelayEnable";
    case MatrixOutput_EqEnable:                 return "Matrix Output EqEnable";
    case MatrixOutput_Polarity:                 return "Matrix Output Polarity";
    case MatrixOutput_ChannelName:              return "Matrix Output ChannelName";
    case MatrixOutput_LevelMeterPreMute:        return "Matrix Output LevelMeterPreMute";
    case MatrixOutput_LevelMeterPostMute:       return "Matrix Output LevelMeterPostMute";
    case Positioning_SourceSpread:              return "Sound Object Spread";
    case Positioning_SourceDelayMode:           return "Sound Object Delay Mode";
    case Positioning_SourceEnable:              return "Sound Object En-Scene Enable";
    case Positioning_SourcePosition:            return "Absolute Sound Object Position XYZ";
    case Positioning_SourcePosition_XY:         return "Absolute Sound Object Position XY";
    case Positioning_SourcePosition_X:          return "Absolute Sound Object Position X";
    case Positioning_SourcePosition_Y:          return "Absolute Sound Object Position Y";
    case CoordinateMapping_SourcePosition:      return "Mapped Sound Object Position XYZ";
    case CoordinateMapping_SourcePosition_XY:   return "Mapped Sound Object Position XY";
    case CoordinateMapping_SourcePosition_X:    return "Mapped Sound Object Position X";
    case CoordinateMapping_SourcePosition_Y:    return "Mapped Sound Object Position Y";
    case MatrixSettings_ReverbRoomId:           return "Reverb Room ID";
    case MatrixSettings_ReverbPredelayFactor:   return "Reverb Predelay Factor";
    case MatrixSettings_ReverbRearLevel:        return "Reverb Rear Level";
    case FunctionGroup_Name:                    return "FunctionGroup Name";
    case FunctionGroup_Delay:                   return "FunctionGroup Delay";
    case FunctionGroup_Mode:                    return "FunctionGroup Mode";
    case FunctionGroup_SpreadFactor:            return "FunctionGroup SpreadFactor";
    case ReverbInput_Gain:                      return "Reverb Input Gain";
    case ReverbInputProcessing_Mute:            return "Reverb Input Processing Mute";
    case ReverbInputProcessing_Gain:            return "Reverb Input Processing Gain";
    case ReverbInputProcessing_EqEnable:        return "Reverb Input Processing EqEnable";
    case ReverbInputProcessing_LevelMeter:      return "Reverb Input Processing LevelMeter";
    case Scene_SceneIndex:                      return "Scene Index";
    case Scene_SceneName:                       return "Scene Name";
    case Scene_SceneComment:                    return "Scene Comment";
    case Scene_Previous:                        return "Scene Previous";
    case Scene_Next:                            return "Scene Next";
    case Scene_Recall:                          return "Scene Recall";
    case CoordinateMappingSettings_P1real:      return "Mapping Area P1 real";
    case CoordinateMappingSettings_P2real:      return "Mapping Area P2 real";
    case CoordinateMappingSettings_P3real:      return "Mapping Area P3 real";
    case CoordinateMappingSettings_P4real:      return "Mapping Area P4 real";
    case CoordinateMappingSettings_P1virtual:   return "Mapping Area P1 virtual";
    case CoordinateMappingSettings_P3virtual:   return "Mapping Area P3 virtual";
    case CoordinateMappingSettings_Flip:        return "Mapping Area flip";
    case CoordinateMappingSettings_Name:        return "Mapping Area name";
    case Positioning_SpeakerPosition:           return "Speaker Position";
    case Positioning_SpeakerGroup:              return "Speaker Group";
    case SoundObjectRouting_Mute:               return "Soundobject Routing Mute";
    case SoundObjectRouting_Gain:               return "Soundobject Routing Gain";
    case Invalid:                               return "INVALID";
    default:                                    return "";
    }
}

bool SoundscapeController::RemoteObject::IsFlickering(RemObjIdent roi)
{
    switch (roi)
    {
    case MatrixInput_LevelMeterPreMute:
    case MatrixInput_LevelMeterPostMute:
    case MatrixOutput_LevelMeterPreMute:
    case MatrixOutput_LevelMeterPostMute:
    case ReverbInputProcessing_LevelMeter:
        return true;
    default:
        return false;
    }
}


} // namespace NanoOcp1
