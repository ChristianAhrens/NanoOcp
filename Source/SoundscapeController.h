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

#pragma once

#include "Ocp1Controller.h"
#include "Ocp1ObjectDefinitions.h"
#include "Variant.h"

#include <cstdint>
#include <functional>
#include <map>
#include <optional>
#include <string>
#include <unordered_map>
#include <vector>


namespace NanoOcp1
{


/**
 * @brief OCA/OCP.1 controller for the d&b audiotechnik DS100 signal engine.
 *
 * Specialises Ocp1Controller with the full DS100 parameter vocabulary
 * (RemObjIdent, RemObjAddr, RemoteObject) and the DS100-specific connection
 * handshake that queries `Fixed_GUID` before subscribing, so that
 * firmware-dependent object definitions (speaker position) can be selected.
 *
 * ## Usage
 * ```cpp
 * SoundscapeController ctrl;
 * ctrl.setDeviceIOSize(64, 32);   // optional — defaults to max
 *
 * // Choose which objects to subscribe to and query on connect:
 * ctrl.setActiveRemoteObjects({
 *     { SoundscapeController::RemoteObject::Positioning_SourcePosition, {1, 0} },
 *     { SoundscapeController::RemoteObject::MatrixInput_Gain,           {1, 0} },
 * });
 *
 * ctrl.onRemoteObjectReceived = [](const SoundscapeController::RemoteObject& ro) -> bool {
 *     // decode ro.Var according to ro.Id
 *     return true;
 * };
 *
 * ctrl.connect("192.168.1.100", 50014);
 * ```
 *
 * ## Connection lifecycle
 * After TCP connect, the controller queries `Fixed_GUID` first.  On receipt it
 * determines the OCA revision (stack-ident) and, if necessary, patches the
 * speaker-position object definitions before subscribing and querying.
 *
 * ## Threading
 * See `Ocp1Controller`'s "Threading" documentation — `onRemoteObjectReceived` and
 * `onStateChanged` follow the same `callbacksOnMessageThread` constructor parameter.
 */
class SoundscapeController : public Ocp1Controller
{
public:
    // ── Hardware detection ────────────────────────────────────────────────────

    /** d&b DS100 hardware variants detected from the device GUID. */
    enum class DbDeviceModel
    {
        Invalid = 0, ///< Not yet determined or unsupported.
        DS100,       ///< Standard DS100 (Dante network audio).
        DS110,       ///< DS110.
        DS100M,      ///< DS100M (Milan network audio).
        vCore        ///< vCore software signal engine.
    };

    /** DS100 coordinate-mapping area index (1–4). */
    enum class MappingAreaId
    {
        First  = 1,
        Second = 2,
        Third  = 3,
        Fourth = 4
    };

    static constexpr std::uint16_t sc_MAX_INPUT_CHANNELS  = 128;
    static constexpr std::uint16_t sc_MAX_OUTPUT_CHANNELS = 64;
    static constexpr std::uint16_t sc_MAX_FUNCTION_GROUPS = 32;
    static constexpr std::uint16_t sc_MAX_REVERB_ZONES    = 4;

    // ── Two-dimensional object address ────────────────────────────────────────

    /**
     * @brief Two-dimensional address of a DS100 remote object.
     *
     * - `pri` (primary): channel number — matrix input (1–128), output (1–64),
     *   function group (1–32), or reverb zone (1–4).
     * - `sec` (secondary): only used for two-dimensional parameters, e.g.
     *   mapping area index for CoordinateMapping or output channel for MatrixNode.
     * - Fields equal to `sc_INV` (0) mean "not used".
     */
    struct RemObjAddr
    {
        std::int16_t pri{0};
        std::int16_t sec{0};
        static constexpr std::int16_t sc_INV = 0;

        RemObjAddr() = default;
        RemObjAddr(std::int16_t p, std::int16_t s) : pri(p), sec(s) {}

        bool operator==(const RemObjAddr& r) const { return pri == r.pri && sec == r.sec; }
        bool operator!=(const RemObjAddr& r) const { return !(*this == r); }
        bool operator<(const RemObjAddr& r) const
        {
            return pri < r.pri || (pri == r.pri && sec < r.sec);
        }
    };

    // ── Remote parameter ──────────────────────────────────────────────────────

    /**
     * @brief A fully-qualified DS100 remote parameter: identifier, address, value.
     *
     * Callers receive populated RemoteObjects via `onRemoteObjectReceived`, and
     * send them (with the desired value in `Var`) via `setObjectValue()`.
     */
    struct RemoteObject
    {
        /**
         * @brief Enumerates every controllable or observable DS100 parameter.
         *
         * Naming mirrors the OCA object hierarchy on the device.
         * X/Y/XY split variants (Positioning_SourcePosition_X, _Y, _XY and the
         * CoordinateMapping equivalents) and Scene_Previous/Next/Recall are
         * Umsci-level conveniences mapped to the underlying XYZ or SceneAgent
         * OCA objects at the protocol level.
         */
        enum RemObjIdent
        {
            HeartbeatPing = 0,
            HeartbeatPong,
            Invalid,
            Fixed_GUID,                         ///< Read-only 8-char GUID; queried before subscriptions.
            Settings_DeviceName,
            Status_StatusText,
            Status_AudioNetworkSampleStatus,
            Error_GnrlErr,
            Error_ErrorText,
            MatrixInput_Mute,
            MatrixInput_Gain,
            MatrixInput_Delay,
            MatrixInput_DelayEnable,
            MatrixInput_EqEnable,
            MatrixInput_Polarity,
            MatrixInput_ChannelName,
            MatrixInput_LevelMeterPreMute,
            MatrixInput_LevelMeterPostMute,
            MatrixInput_ReverbSendGain,
            MatrixNode_Enable,
            MatrixNode_Gain,
            MatrixNode_DelayEnable,
            MatrixNode_Delay,
            MatrixOutput_Mute,
            MatrixOutput_Gain,
            MatrixOutput_Delay,
            MatrixOutput_DelayEnable,
            MatrixOutput_EqEnable,
            MatrixOutput_Polarity,
            MatrixOutput_ChannelName,
            MatrixOutput_LevelMeterPreMute,
            MatrixOutput_LevelMeterPostMute,
            Positioning_SourceSpread,
            Positioning_SourceDelayMode,
            Positioning_SourceEnable,           ///< En-Scene participation switch (OcaSwitch, positions 0=matrix-only, 1=En-Scene).
            Positioning_SourcePosition_XY,      ///< Convenience split; maps to Positioning_SourcePosition OCA object.
            Positioning_SourcePosition_X,        ///< Convenience split; maps to Positioning_SourcePosition OCA object.
            Positioning_SourcePosition_Y,        ///< Convenience split; maps to Positioning_SourcePosition OCA object.
            Positioning_SourcePosition,          ///< XYZ blob (3×float32 normalised 0–1).
            CoordinateMapping_SourcePosition_XY, ///< Convenience split.
            CoordinateMapping_SourcePosition_X,  ///< Convenience split.
            CoordinateMapping_SourcePosition_Y,  ///< Convenience split.
            CoordinateMapping_SourcePosition,    ///< XYZ blob in virtual mapping space.
            MatrixSettings_ReverbRoomId,
            MatrixSettings_ReverbPredelayFactor,
            MatrixSettings_ReverbRearLevel,
            FunctionGroup_Name,
            FunctionGroup_Delay,
            FunctionGroup_Mode,
            FunctionGroup_SpreadFactor,
            ReverbInput_Gain,
            ReverbInputProcessing_Mute,
            ReverbInputProcessing_Gain,
            ReverbInputProcessing_EqEnable,
            ReverbInputProcessing_LevelMeter,
            Scene_SceneIndex,
            Scene_SceneName,
            Scene_SceneComment,
            Scene_Previous,                     ///< Maps to SceneAgent PreviousScene command.
            Scene_Next,                         ///< Maps to SceneAgent NextScene command.
            Scene_Recall,                       ///< Maps to SceneAgent ApplyScene command.
            CoordinateMappingSettings_P1real,
            CoordinateMappingSettings_P2real,
            CoordinateMappingSettings_P3real,
            CoordinateMappingSettings_P4real,
            CoordinateMappingSettings_P1virtual,
            CoordinateMappingSettings_P3virtual,
            CoordinateMappingSettings_Flip,
            CoordinateMappingSettings_Name,
            Positioning_SpeakerPosition,        ///< 6-DOF loudspeaker position blob (6×float32).
            Positioning_SpeakerGroup,
            SoundObjectRouting_Mute,
            SoundObjectRouting_Gain,
            Device_Clear,
            InvalidMAX
        };

        RemObjIdent Id  {Invalid};
        RemObjAddr  Addr;
        Variant     Var;

        RemoteObject() = default;
        RemoteObject(RemObjIdent id, RemObjAddr addr, Variant v = Variant{})
            : Id(id), Addr(addr), Var(std::move(v)) {}

        bool operator==(const RemoteObject& o) const
        {
            return Id == o.Id && Addr == o.Addr && Var == o.Var;
        }
        bool operator!=(const RemoteObject& o) const { return !(*this == o); }
        bool operator<(const RemoteObject& o) const
        {
            return Id < o.Id || (Id == o.Id && Addr < o.Addr);
        }

        static std::string GetObjectDescription(RemObjIdent roi);

        /** Returns true for objects that update at meter-rate and would flood a log. */
        static bool IsFlickering(RemObjIdent roi);
    };

    // ── Construction / destruction ────────────────────────────────────────────

    /** @param callbacksOnMessageThread  See `Ocp1Controller`'s constructor. */
    explicit SoundscapeController(bool callbacksOnMessageThread = true);
    ~SoundscapeController() override;

    //==========================================================================
    /**
     * Set the number of input and output channels known to be present on the
     * target device.  Rebuilds the internal ONo map.  Call before connect().
     * Values are clamped to [1, sc_MAX_*].
     */
    void setDeviceIOSize(std::uint16_t inputs, std::uint16_t outputs);

    /**
     * Set the list of remote objects to subscribe to and query on every connection.
     * Must be called while Disconnected.  Returns false if not Disconnected.
     *
     * Objects in the X/Y/XY split-view variants and Scene_Previous/Next/Recall are
     * not directly subscribed (no matching OCA object exists); omit them or include
     * the parent XYZ / SceneAgent object instead.
     */
    bool setActiveRemoteObjects(const std::vector<RemoteObject>& objs);

    const std::vector<RemoteObject>& getActiveRemoteObjects() const;

    /**
     * Send a SetValue command for the given remote object.
     * Only valid when Connected.  Returns false if not Connected or if no
     * OCA definition is available for the given ROI+address combination.
     */
    bool setObjectValue(const RemoteObject& obj);

    /** Returns the hardware model detected from the device GUID. */
    DbDeviceModel getConnectedDeviceModel() const { return m_connectedModel; }

    /** Returns the OCA stack identifier (0 = legacy, 1 = extended, −1 = unknown). */
    int getOcaStackIdent() const { return m_stackIdent; }

    //==========================================================================
    /**
     * Fired (see class-level "Threading" documentation) when a subscribed or
     * queried object delivers a new value.  Return true if the object was
     * handled; false is ignored.
     *
     * The `RemoteObject::Var` field contains the decoded value.
     */
    std::function<bool(const RemoteObject&)> onRemoteObjectReceived;

protected:
    //==========================================================================
    void afterConnected() override;
    void onUntrackedGetValueResponse(std::uint32_t ono, const ByteVector& paramData) override;

private:
    //==========================================================================
    void createKnownONosMap();
    void rebuildTrackedObjects();
    void processGuidAndSubscribe(const std::string& guid);
    bool setOcaRevisionAndDeviceModel(const std::string& guid);

    std::optional<std::unique_ptr<Ocp1CommandDefinition>>
        getObjectDefinition(RemoteObject::RemObjIdent roi,
                            const RemObjAddr& addr,
                            bool useRemapping = false) const;

    static Ocp1DataType dataTypeForRoi(RemoteObject::RemObjIdent roi);

    //==========================================================================
    using ROIToDefsMap = std::map<RemoteObject::RemObjIdent,
                                  std::map<RemObjAddr, Ocp1CommandDefinition>>;
    using ONoToROIMap  = std::unordered_map<std::uint32_t,
                           std::pair<RemoteObject::RemObjIdent, RemObjAddr>>;

    ROIToDefsMap m_ROIsToDefsMap;
    ONoToROIMap  m_ONoToROIMap;

    std::vector<RemoteObject> m_activeRemoteObjects;

    std::uint16_t m_activeInputChannelCount { sc_MAX_INPUT_CHANNELS  };
    std::uint16_t m_activeOutputChannelCount{ sc_MAX_OUTPUT_CHANNELS };

    std::string   m_deviceGuid;
    int           m_stackIdent   { -1 };
    DbDeviceModel m_connectedModel{ DbDeviceModel::Invalid };
};


} // namespace NanoOcp1
