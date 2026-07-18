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

#include <cstdint>
#include <functional>


namespace NanoOcp1
{


/**
 * @brief OCA/OCP.1 controller for d&b audiotechnik amplifier products.
 *
 * Specialises Ocp1Controller for the d&b Dx, Dy, and 5D amplifier families.
 * On every connection the controller automatically subscribes to and queries
 * all relevant objects for the configured amp type and channel count, then
 * delivers decoded values via typed callbacks.
 *
 * ## Supported properties
 * | Property   | Object definition | Direction |
 * |------------|-------------------|-----------|
 * | Power      | Settings_PwrOn    | R/W       |
 * | Gain       | Config_PotiLevel  | R/W       |
 * | Mute       | Config_Mute       | R/W       |
 * | ISP        | ChStatus_Isp      | R (status)|
 * | GR         | ChStatus_Gr       | R (status)|
 * | OVL        | ChStatus_Ovl      | R (status)|
 * | Headroom   | ChStatus_GrHead   | R (status)|
 *
 * ## Usage
 * ```cpp
 * AmpController ctrl;
 * ctrl.setAmpType(AmpController::AmpType::Dy, 4);
 *
 * ctrl.onPower       = [](bool on) { ... };
 * ctrl.onChannelGain = [](uint16_t ch, float dB) { ... };
 * // ... wire remaining callbacks ...
 *
 * ctrl.connect("192.168.1.100", 50014);
 * ```
 *
 * ## AmpType variants
 * - **Dx** — d&b Dx series.  Shares PwrOn/ISP/GR/OVL ONos with Dy; GrHead has a
 *   distinct ONO.
 * - **Dy** — d&b Dy series.  Same as Dx except for the GrHead ONO.
 * - **FiveD** — d&b 5D amplifier.  All status objects have different ONos from
 *   the Dx/Dy family.
 */
class AmpController : public Ocp1Controller
{
public:
    /** d&b amplifier product family — determines which OCA object definitions are used. */
    enum class AmpType
    {
        Dx,    ///< d&b Dx series.
        Dy,    ///< d&b Dy series.
        FiveD  ///< d&b 5D amplifier (named FiveD because identifiers cannot start with a digit).
    };

    AmpController();
    ~AmpController() override;

    //==========================================================================
    /**
     * Configure the amplifier type and output-channel count.
     *
     * Rebuilds the internal tracked-object list to match the OCA object numbers
     * for the given amp type and channel count.  Must be called while Disconnected;
     * invoke disconnect() first if the controller is active.
     *
     * @param type          Hardware variant of the target amplifier.
     * @param channelCount  Number of output channels to subscribe (typically 2 or 4).
     */
    void setAmpType(AmpType type, std::uint16_t channelCount);

    AmpType       getAmpType()      const { return m_ampType; }
    std::uint16_t getChannelCount() const { return m_channelCount; }

    //==========================================================================
    /**
     * Send a power on/off command.  Only succeeds when Connected.
     * Value: 1 = ON, 0 = OFF.
     */
    bool setPower(bool on);

    /**
     * Send a per-channel gain command.  Only succeeds when Connected.
     * Corresponds to OCA Config_PotiLevel (OcaGain Prop_Gain).
     */
    bool setChannelGain(std::uint16_t channel, float gainDb);

    /**
     * Send a per-channel mute command.  Only succeeds when Connected.
     * Wire encoding: 1 = muted, 2 = unmuted (d&b amp convention).
     */
    bool setChannelMute(std::uint16_t channel, bool mute);

    //==========================================================================
    // Typed callbacks — all fired on the NanoOcp1 socket thread.

    /** Fired when the amplifier power state changes or is queried. */
    std::function<void(bool on)>                                 onPower;

    /** Fired when a channel gain value changes or is queried. */
    std::function<void(std::uint16_t channel, float gainDb)>     onChannelGain;

    /**
     * Fired when a channel mute state changes or is queried.
     * @param muted  true if the channel is currently muted.
     */
    std::function<void(std::uint16_t channel, bool muted)>       onChannelMute;

    /**
     * Fired when the In-Signal-Present (ISP) status of a channel changes.
     * @param active  true if signal is present above threshold.
     */
    std::function<void(std::uint16_t channel, bool active)>      onChannelISP;

    /**
     * Fired when the Gain-Reduction (GR) status of a channel changes.
     * @param active  true if gain reduction is currently applied.
     */
    std::function<void(std::uint16_t channel, bool active)>      onChannelGR;

    /**
     * Fired when the Overload (OVL) status of a channel changes.
     * @param active  true if the channel is in overload.
     */
    std::function<void(std::uint16_t channel, bool active)>      onChannelOVL;

    /**
     * Fired when the gain-reduction headroom value of a channel changes.
     * @param headroomDb  Available headroom in dB before gain reduction activates.
     */
    std::function<void(std::uint16_t channel, float headroomDb)> onChannelHeadroom;

private:
    void rebuildTrackedObjects();

    AmpType        m_ampType{AmpType::Dx};
    std::uint16_t  m_channelCount{4};
};


} // namespace NanoOcp1
