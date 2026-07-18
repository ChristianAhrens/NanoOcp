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

#include "AmpController.h"

#include "Ocp1DataTypes.h"
#include "Ocp1ObjectDefinitions.h"

#include <cassert>


namespace NanoOcp1
{


// ── Construction / destruction ────────────────────────────────────────────────

AmpController::AmpController() = default;

AmpController::~AmpController()
{
    // Ocp1Controller destructor calls disconnect(), which is sufficient.
}


// ── Configuration ─────────────────────────────────────────────────────────────

void AmpController::setAmpType(AmpType type, std::uint16_t channelCount)
{
    assert(getState() == State::Disconnected);
    m_ampType      = type;
    m_channelCount = channelCount;
    rebuildTrackedObjects();
}

void AmpController::rebuildTrackedObjects()
{
    clearTrackedObjects();

    // ── Power (global, no channel index) ─────────────────────────────────────
    switch (m_ampType)
    {
    case AmpType::FiveD:
        trackObject(
            std::make_unique<Amp5D::dbOcaObjectDef_Settings_PwrOn>(),
            [this](const ByteVector& data) {
                if (onPower) onPower(DataToUint16(data) > 0);
            });
        break;
    default: // Dx and Dy share the same PwrOn definition
        trackObject(
            std::make_unique<AmpDxDy::dbOcaObjectDef_Settings_PwrOn>(),
            [this](const ByteVector& data) {
                if (onPower) onPower(DataToUint16(data) > 0);
            });
        break;
    }

    // ── Per-channel objects ───────────────────────────────────────────────────
    for (std::uint16_t ch = 1; ch <= m_channelCount; ++ch)
    {
        // Gain — common to all amp types
        trackObject(
            std::make_unique<AmpGeneric::dbOcaObjectDef_Config_PotiLevel>(ch),
            [this, ch](const ByteVector& data) {
                if (onChannelGain) onChannelGain(ch, DataToFloat(data));
            });

        // Mute — common to all amp types.
        // Wire encoding from the device: 1 = muted, 2 = unmuted.
        trackObject(
            std::make_unique<AmpGeneric::dbOcaObjectDef_Config_Mute>(ch),
            [this, ch](const ByteVector& data) {
                if (onChannelMute) onChannelMute(ch, DataToUint8(data) == 1);
            });

        // ISP, GR, OVL — Dx/Dy share definitions; 5D uses different ONos
        switch (m_ampType)
        {
        case AmpType::FiveD:
            trackObject(
                std::make_unique<Amp5D::dbOcaObjectDef_ChStatus_Isp>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelISP) onChannelISP(ch, DataToBool(data));
                });
            trackObject(
                std::make_unique<Amp5D::dbOcaObjectDef_ChStatus_Gr>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelGR) onChannelGR(ch, DataToBool(data));
                });
            trackObject(
                std::make_unique<Amp5D::dbOcaObjectDef_ChStatus_Ovl>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelOVL) onChannelOVL(ch, DataToBool(data));
                });
            break;
        default: // Dx and Dy
            trackObject(
                std::make_unique<AmpDxDy::dbOcaObjectDef_ChStatus_Isp>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelISP) onChannelISP(ch, DataToBool(data));
                });
            trackObject(
                std::make_unique<AmpDxDy::dbOcaObjectDef_ChStatus_Gr>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelGR) onChannelGR(ch, DataToBool(data));
                });
            trackObject(
                std::make_unique<AmpDxDy::dbOcaObjectDef_ChStatus_Ovl>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelOVL) onChannelOVL(ch, DataToBool(data));
                });
            break;
        }

        // GrHead (headroom) — all three amp types have distinct ONos
        switch (m_ampType)
        {
        case AmpType::Dx:
            trackObject(
                std::make_unique<AmpDx::dbOcaObjectDef_ChStatus_GrHead>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelHeadroom) onChannelHeadroom(ch, DataToFloat(data));
                });
            break;
        case AmpType::Dy:
            trackObject(
                std::make_unique<AmpDy::dbOcaObjectDef_ChStatus_GrHead>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelHeadroom) onChannelHeadroom(ch, DataToFloat(data));
                });
            break;
        case AmpType::FiveD:
            trackObject(
                std::make_unique<Amp5D::dbOcaObjectDef_ChStatus_GrHead>(ch),
                [this, ch](const ByteVector& data) {
                    if (onChannelHeadroom) onChannelHeadroom(ch, DataToFloat(data));
                });
            break;
        }
    }
}


// ── Typed setters ─────────────────────────────────────────────────────────────

bool AmpController::setPower(bool on)
{
    const Variant v(static_cast<std::uint16_t>(on ? 1 : 0));
    switch (m_ampType)
    {
    case AmpType::FiveD:
    {
        Amp5D::dbOcaObjectDef_Settings_PwrOn def;
        return setValue(def, v);
    }
    default: // Dx and Dy
    {
        AmpDxDy::dbOcaObjectDef_Settings_PwrOn def;
        return setValue(def, v);
    }
    }
}

bool AmpController::setChannelGain(std::uint16_t channel, float gainDb)
{
    AmpGeneric::dbOcaObjectDef_Config_PotiLevel def(channel);
    return setValue(def, Variant(gainDb));
}

bool AmpController::setChannelMute(std::uint16_t channel, bool mute)
{
    AmpGeneric::dbOcaObjectDef_Config_Mute def(channel);
    // d&b amp convention: 1 = muted, 2 = unmuted (not the boolean 0/1)
    return setValue(def, Variant(static_cast<std::uint8_t>(mute ? 1 : 2)));
}


} // namespace NanoOcp1
