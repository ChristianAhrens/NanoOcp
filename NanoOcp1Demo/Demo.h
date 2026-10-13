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

// ── Demo controller ───────────────────────────────────────────────────────────
// Wraps AmpController / SoundscapeController, owns the Config for the mode
// selected on the command line, and exposes one cmd*() method per interactive
// command. All cmd*()/connect*() methods update g_state and push to the log;
// they never touch the terminal directly (see Panels.h for that).

#include <algorithm>
#include <functional>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <vector>

#include "AmpController.h"
#include "AppState.h"
#include "FocusParams.h"
#include "Format.h"
#include "SoundscapeController.h"

class Demo
{
public:
    struct Config
    {
        std::string host{"127.0.0.1"};
        int         port{50014};
        DemoMode    mode{DemoMode::Amp};
        // Amp-specific
        NanoOcp1::AmpController::AmpType ampType{NanoOcp1::AmpController::AmpType::Dy};
        std::uint16_t channelCount{4};
        // DS100-specific: sound-object range for Soundscape-overview mode
        // (soundObjectHi == soundObjectLo means a single object).
        int soundObjectLo{1};
        int soundObjectHi{1};
        // Soundscape-focus-specific (only used when mode == SoundscapeFocus);
        // addresses exactly soundObjectLo.
        SORemObjIdent focusParamId{SORemObjIdent::Invalid};
        std::string   focusParamName;
        int           addr2{0};
        // Soundscape-only: show the optional loudspeaker level-meter block
        // (MatrixOutput_LevelMeterPostMute, for outputs with an assigned
        // Positioning_SpeakerPosition) below the overview/routing panel.
        bool speakerLvls{false};
    };

    explicit Demo(Config cfg) : m_cfg(std::move(cfg))
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        g_state.address       = m_cfg.host;
        g_state.port          = m_cfg.port;
        g_state.mode          = m_cfg.mode;
        g_state.amp.type          = m_cfg.ampType;
        g_state.amp.channelCount  = static_cast<int>(m_cfg.channelCount);
        g_state.ds100.soundObjectLo = m_cfg.soundObjectLo;
        g_state.ds100.soundObjectHi = m_cfg.soundObjectHi;
        g_state.ds100.view          = SoundscapeView::Overview;
        g_state.ds100.objects.clear();
        for (int so = m_cfg.soundObjectLo; so <= m_cfg.soundObjectHi; ++so)
        {
            AppState::SoObjState o;
            o.soundObject = so;
            g_state.ds100.objects.push_back(o);
        }
        g_state.ds100.speakerLvlsEnabled = m_cfg.speakerLvls;
        g_state.ds100.speakerObjects.clear();
        if (m_cfg.speakerLvls)
        {
            const int maxOut = static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_OUTPUT_CHANNELS);
            for (int ch = 1; ch <= maxOut; ++ch)
            {
                AppState::SpeakerObjState s;
                s.outputChannel = ch;
                g_state.ds100.speakerObjects.push_back(s);
            }
        }
        g_state.focus.paramName   = m_cfg.focusParamName;
        g_state.focus.paramId     = m_cfg.focusParamId;
        g_state.focus.addr        = SORemObjAddr{static_cast<std::int16_t>(m_cfg.soundObjectLo),
                                                  static_cast<std::int16_t>(m_cfg.addr2)};
    }

    ~Demo() { teardown(); }

    // ── Transport ─────────────────────────────────────────────────────────────

    void connect()
    {
        teardown();

        switch (m_cfg.mode)
        {
        case DemoMode::Amp:
            connectAmp();
            break;
        case DemoMode::Soundscape:
            if (m_routingActive) connectRouting();
            else                 connectDS100();
            break;
        case DemoMode::SoundscapeFocus:
            connectFocus();
            break;
        }
    }

    void disconnect()
    {
        teardown();
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            g_state.ctrlState = CtrlState::Disconnected;
            switch (m_cfg.mode)
            {
            case DemoMode::Amp:
                resetAmpDisplay();
                break;
            case DemoMode::Soundscape:
                if (m_routingActive) resetRoutingDisplay();
                else                 resetSoDisplay();
                resetSpeakerDisplay();
                break;
            case DemoMode::SoundscapeFocus:
                resetFocusDisplay();
                break;
            }
        }
        g_needsRedraw = true;
        if (m_cfg.mode == DemoMode::SoundscapeFocus)
            pushFocusLog("Disconnected.");
        else
            pushLog("Disconnected.");
    }

    void setAddress(const std::string& addr)
    {
        m_cfg.host = addr;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            g_state.address = addr;
        }
        g_needsRedraw = true;
    }

    void setPort(int p)
    {
        m_cfg.port = p;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            g_state.port = p;
        }
        g_needsRedraw = true;
    }

    // ── Amp commands ──────────────────────────────────────────────────────────

    void cmdPower(bool on)
    {
        if (!m_amp) { pushLog("Not in amp mode or not connected."); return; }
        if (!m_amp->setPower(on))
            pushLog("setPower failed (not connected?)");
        else
            pushLog(std::string("Sent power ") + (on ? "ON" : "OFF"));
    }

    void cmdGain(std::uint16_t ch, float dB)
    {
        if (!m_amp) { pushLog("Not in amp mode or not connected."); return; }
        if (ch < 1 || ch > m_cfg.channelCount)
        {
            pushLog("Channel out of range (1-" + std::to_string(m_cfg.channelCount) + ")");
            return;
        }
        if (dB < -57.5f || dB > 6.0f)
        {
            pushLog("Gain out of range (-57.5 to +6.0 dB)");
            return;
        }
        if (!m_amp->setChannelGain(ch, dB))
            pushLog("setChannelGain failed (not connected?)");
        else
        {
            std::ostringstream oss;
            oss << "Sent Ch" << ch << " gain "
                << std::fixed << std::setprecision(1) << dB << " dB";
            pushLog(oss.str());
        }
    }

    void cmdMute(std::uint16_t ch, bool mute)
    {
        if (!m_amp) { pushLog("Not in amp mode or not connected."); return; }
        if (ch < 1 || ch > m_cfg.channelCount)
        {
            pushLog("Channel out of range (1-" + std::to_string(m_cfg.channelCount) + ")");
            return;
        }
        if (!m_amp->setChannelMute(ch, mute))
            pushLog("setChannelMute failed (not connected?)");
        else
            pushLog(std::string("Sent Ch") + std::to_string(ch)
                    + (mute ? " MUTE" : " UNMUTE"));
    }

    // ── DS100 commands ────────────────────────────────────────────────────────
    // Every command below applies to `target` (a single sound-object number) if
    // given and non-zero, or broadcasts to every object in the active overview
    // range [m_cfg.soundObjectLo, m_cfg.soundObjectHi] when target == 0.

    void cmdPositionX(float x, int target = 0)
    {
        applyToTargets("X=" + fixedStr(x, 3), target,
                        [this, x](int so) { return sendPositionAxis(so, 0, x); });
    }

    void cmdPositionY(float y, int target = 0)
    {
        applyToTargets("Y=" + fixedStr(y, 3), target,
                        [this, y](int so) { return sendPositionAxis(so, 1, y); });
    }

    void cmdPositionZ(float z, int target = 0)
    {
        applyToTargets("Z=" + fixedStr(z, 3), target,
                        [this, z](int so) { return sendPositionAxis(so, 2, z); });
    }

    void cmdSpread(float s, int target = 0)
    {
        if (s < 0.0f || s > 1.0f) { pushLog("Spread out of range (0.0-1.0)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        applyToTargets("spread " + fixedStr(s, 3), target, [this, s](int so) {
            return m_ds100->setObjectValue(RO{ROI::Positioning_SourceSpread, ROA{static_cast<std::int16_t>(so), 0},
                                              NanoOcp1::Variant{static_cast<std::float_t>(s)}});
        });
    }

    void cmdDelayMode(int dm, int target = 0)
    {
        if (dm < 0 || dm > 2) { pushLog("Delay mode out of range (0, 1, or 2)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        applyToTargets("delay mode " + std::to_string(dm), target, [this, dm](int so) {
            return m_ds100->setObjectValue(RO{ROI::Positioning_SourceDelayMode, ROA{static_cast<std::int16_t>(so), 0},
                                              NanoOcp1::Variant{static_cast<std::uint8_t>(dm)}});
        });
    }

    void cmdMatrixInputGain(float dB, int target = 0)
    {
        if (dB < -120.0f || dB > 24.0f) { pushLog("Gain out of range (-120.0 to +24.0 dB)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        applyToTargets("input gain " + fixedStr(dB, 1) + " dB", target, [this, dB](int so) {
            return m_ds100->setObjectValue(RO{ROI::MatrixInput_Gain, ROA{static_cast<std::int16_t>(so), 0},
                                              NanoOcp1::Variant{static_cast<std::float_t>(dB)}});
        });
    }

    void cmdMatrixInputMute(bool mute, int target = 0)
    {
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        // d&b convention: 1 = muted, 2 = unmuted (not the boolean 0/1)
        applyToTargets(std::string("input ") + (mute ? "MUTE" : "UNMUTE"), target, [this, mute](int so) {
            return m_ds100->setObjectValue(RO{ROI::MatrixInput_Mute, ROA{static_cast<std::int16_t>(so), 0},
                                              NanoOcp1::Variant{static_cast<std::uint8_t>(mute ? 1 : 2)}});
        });
    }

    void cmdEnSpace(float dB, int target = 0)
    {
        if (dB < -120.0f || dB > 24.0f) { pushLog("Gain out of range (-120.0 to +24.0 dB)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        applyToTargets("EnSpace " + fixedStr(dB, 1) + " dB", target, [this, dB](int so) {
            return m_ds100->setObjectValue(RO{ROI::MatrixInput_ReverbSendGain, ROA{static_cast<std::int16_t>(so), 0},
                                              NanoOcp1::Variant{static_cast<std::float_t>(dB)}});
        });
    }

    // ── Soundobject-Routing sub-mode ──────────────────────────────────────────
    // Entered (for any sound-object range addressable via --soundscape, single
    // or a sub-range of it) to control SoundObjectRouting_Gain/Mute for every
    // selected sound object across all 32 Function Groups. Switching into or
    // out of this sub-mode disconnects and re-subscribes, since
    // SoundscapeController's active remote-object list can only be changed
    // while disconnected.

    void enterRouting(int lo, int hi)
    {
        if (m_cfg.mode != DemoMode::Soundscape)
        {
            pushLog("Routing sub-mode is only available in Soundscape mode.");
            return;
        }
        if (lo > hi) std::swap(lo, hi);
        if (lo < m_cfg.soundObjectLo || hi > m_cfg.soundObjectHi)
        {
            pushLog("Sound object range out of bounds of the active range ("
                    + std::to_string(m_cfg.soundObjectLo) + "-" + std::to_string(m_cfg.soundObjectHi) + ")");
            return;
        }
        teardown();
        m_routingActive = true;
        m_routingLo     = lo;
        m_routingHi     = hi;
        connectRouting();
    }

    void exitRouting()
    {
        if (!m_routingActive) { pushLog("Not in routing sub-mode."); return; }
        teardown();
        m_routingActive = false;
        connectDS100();
    }

    // `target`, like the overview commands above, is a single sound-object
    // number to target within the active routing range, or 0 to broadcast to
    // every sound object in that range.
    void cmdRoutingGain(int fg, float dB, int target = 0)
    {
        if (!m_ds100 || !m_routingActive) { pushLog("Not in routing sub-mode or not connected."); return; }
        if (fg < 1 || fg > static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS))
        {
            pushLog("Function group out of range (1-32)");
            return;
        }
        if (dB < -120.0f || dB > 24.0f) { pushLog("Gain out of range (-120.0 to +24.0 dB)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto fgNo = static_cast<std::int16_t>(fg);
        applyToTargets("FG" + std::to_string(fg) + " routing gain " + fixedStr(dB, 1) + " dB",
                        target, m_routingLo, m_routingHi, [this, dB, fgNo](int so) {
            return m_ds100->setObjectValue(RO{ROI::SoundObjectRouting_Gain, ROA{static_cast<std::int16_t>(so), fgNo},
                                              NanoOcp1::Variant{static_cast<std::float_t>(dB)}});
        });
    }

    void cmdRoutingMute(int fg, bool mute, int target = 0)
    {
        if (!m_ds100 || !m_routingActive) { pushLog("Not in routing sub-mode or not connected."); return; }
        if (fg < 1 || fg > static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS))
        {
            pushLog("Function group out of range (1-32)");
            return;
        }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto fgNo = static_cast<std::int16_t>(fg);
        // d&b convention: 1 = muted, 2 = unmuted (not the boolean 0/1)
        applyToTargets(std::string("FG") + std::to_string(fg) + " routing " + (mute ? "MUTE" : "UNMUTE"),
                        target, m_routingLo, m_routingHi, [this, mute, fgNo](int so) {
            return m_ds100->setObjectValue(RO{ROI::SoundObjectRouting_Mute, ROA{static_cast<std::int16_t>(so), fgNo},
                                              NanoOcp1::Variant{static_cast<std::uint8_t>(mute ? 1 : 2)}});
        });
    }

    // ── Soundscape-focus command ──────────────────────────────────────────────

    // Parses `rawValue` according to the focused parameter's last-known data type
    // and sends it as a SetValue. Requires at least one value to have already
    // been received, since that is how the demo learns the parameter's type.
    void cmdSetFocusValue(const std::string& rawValue)
    {
        if (!m_ds100) { pushFocusLog("Not connected."); return; }

        NanoOcp1::Ocp1DataType hint = NanoOcp1::OCP1DATATYPE_NONE;
        SORemObjAddr addr;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            if (g_state.focus.valueKnown)
                hint = g_state.focus.lastValue.GetDataType();
            addr = g_state.focus.addr;
        }

        NanoOcp1::Variant value;
        std::string err;
        if (!parseVariantInput(rawValue, hint, value, err))
        {
            pushFocusLog("Invalid value: " + err);
            return;
        }

        using RO = NanoOcp1::SoundscapeController::RemoteObject;
        if (!m_ds100->setObjectValue(RO{m_cfg.focusParamId, addr, value}))
            pushFocusLog("setObjectValue(" + m_cfg.focusParamName + ") failed (not connected?)");
        else
            pushFocusLog("Sent " + m_cfg.focusParamName + " -> " + formatVariant(value));
    }

private:
    // ── Amp connection ────────────────────────────────────────────────────────

    void connectAmp()
    {
        m_amp = std::make_unique<NanoOcp1::AmpController>(m_scheduler);
        m_amp->setAmpType(m_cfg.ampType, m_cfg.channelCount);

        m_amp->onStateChanged = [this](CtrlState s) {
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.ctrlState = s;
                if (s == CtrlState::Disconnected)
                    resetAmpDisplay();
            }
            g_needsRedraw = true;
            pushLog("State: " + stateToStr(s));
        };

        m_amp->onPower = [](bool on) {
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.powerKnown = true;
                g_state.amp.powerOn    = on;
            }
            g_needsRedraw = true;
            pushLog(std::string("Power -> ") + (on ? "ON" : "OFF"));
        };

        m_amp->onChannelGain = [](std::uint16_t ch, float dB) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].gainKnown = true;
                g_state.amp.ch[ch - 1].gainDb    = dB;
            }
            g_needsRedraw = true;
            std::ostringstream oss;
            oss << "Ch" << ch << " gain -> "
                << std::fixed << std::setprecision(1) << dB << " dB";
            pushLog(oss.str());
        };

        m_amp->onChannelMute = [](std::uint16_t ch, bool muted) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].muteKnown = true;
                g_state.amp.ch[ch - 1].muted     = muted;
            }
            g_needsRedraw = true;
            pushLog(std::string("Ch") + std::to_string(ch)
                    + " mute -> " + (muted ? "ON" : "off"));
        };

        m_amp->onChannelISP = [](std::uint16_t ch, bool active) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].ispKnown = true;
                g_state.amp.ch[ch - 1].isp      = active;
            }
            g_needsRedraw = true;
            if (active)
                pushLog(std::string("Ch") + std::to_string(ch) + " ISP active");
        };

        m_amp->onChannelGR = [](std::uint16_t ch, bool active) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].grKnown = true;
                g_state.amp.ch[ch - 1].gr      = active;
            }
            g_needsRedraw = true;
            if (active)
                pushLog(std::string("Ch") + std::to_string(ch) + " GR active");
        };

        m_amp->onChannelOVL = [](std::uint16_t ch, bool active) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].ovlKnown = true;
                g_state.amp.ch[ch - 1].ovl      = active;
            }
            g_needsRedraw = true;
            if (active)
                pushLog(std::string("Ch") + std::to_string(ch) + " OVL!");
        };

        m_amp->onChannelHeadroom = [](std::uint16_t ch, float hr) {
            if (ch < 1 || ch > 4) return;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.amp.ch[ch - 1].hrKnown    = true;
                g_state.amp.ch[ch - 1].headroomDb = hr;
            }
            g_needsRedraw = true;
        };

        pushLog("Connecting to " + m_cfg.host + ":" + std::to_string(m_cfg.port) + " (amp)...");
        m_amp->connect(m_cfg.host, m_cfg.port);
    }

    // ── DS100 connection (Soundscape-overview, ranged) ────────────────────────

    void connectDS100()
    {
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;

        m_ds100 = std::make_unique<NanoOcp1::SoundscapeController>(m_scheduler);

        std::vector<RO> active;
        for (int so = m_cfg.soundObjectLo; so <= m_cfg.soundObjectHi; ++so)
        {
            const auto a = static_cast<std::int16_t>(so);
            active.push_back(RO{ ROI::MatrixInput_LevelMeterIn,      ROA{a, 0} });
            active.push_back(RO{ ROI::MatrixInput_Gain,              ROA{a, 0} });
            active.push_back(RO{ ROI::MatrixInput_Mute,              ROA{a, 0} });
            active.push_back(RO{ ROI::Positioning_SourcePosition,    ROA{a, 0} });
            active.push_back(RO{ ROI::Positioning_SourceSpread,      ROA{a, 0} });
            active.push_back(RO{ ROI::Positioning_SourceDelayMode,   ROA{a, 0} });
            active.push_back(RO{ ROI::MatrixInput_ReverbSendGain,    ROA{a, 0} });
        }
        addSpeakerLvlsSubscriptions(active);
        m_ds100->setActiveRemoteObjects(active);

        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            g_state.ds100.view = SoundscapeView::Overview;
        }
        g_needsRedraw = true;

        m_ds100->onStateChanged = [this](CtrlState s) {
            std::string devModel;
            if (s == CtrlState::Connected)
            {
                devModel = modelStr(m_ds100->getConnectedDeviceModel());
                const int stack = m_ds100->getOcaStackIdent();
                if (stack >= 0)
                    devModel += " stack " + std::to_string(stack);
            }
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.ctrlState         = s;
                g_state.ds100.deviceModel = devModel;
                if (s == CtrlState::Disconnected)
                {
                    resetSoDisplay();
                    resetSpeakerDisplay();
                }
            }
            g_needsRedraw = true;
            pushLog("State: " + stateToStr(s));
        };

        m_ds100->onRemoteObjectReceived = [this](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool {
            using Id = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
            bool ok = false;
            bool logPosition = false;
            std::ostringstream logMsg;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                auto& ds = g_state.ds100;

                // Speaker-level-meter objects are addressed by output channel,
                // not sound object, so they're handled up front — before the
                // sound-object range bounds check below, which would otherwise
                // incorrectly reject any output channel outside that range.
                if (m_cfg.speakerLvls
                    && (ro.Id == Id::Positioning_SpeakerPosition || ro.Id == Id::MatrixOutput_LevelMeterPostMute))
                {
                    ok = updateSpeakerObject(ro);
                    if (ok) g_needsRedraw = true;
                    return ok;
                }

                const int idx = ro.Addr.pri - ds.soundObjectLo;
                if (idx < 0 || idx >= static_cast<int>(ds.objects.size()))
                    return false;
                auto& o = ds.objects[idx];

                switch (ro.Id)
                {
                case Id::MatrixInput_LevelMeterIn:
                {
                    const float dB = ro.Var.ToFloat(&ok);
                    if (ok) { o.levelKnown = true; o.levelDb = dB; }
                    break;
                }
                case Id::MatrixInput_Gain:
                {
                    const float dB = ro.Var.ToFloat(&ok);
                    if (ok) { o.gainKnown = true; o.gainDb = dB; }
                    break;
                }
                case Id::MatrixInput_Mute:
                {
                    const std::uint8_t v = ro.Var.ToUInt8(&ok);
                    if (ok) { o.muteKnown = true; o.muted = (v == 1); }
                    break;
                }
                case Id::Positioning_SourcePosition:
                {
                    const auto xyz = ro.Var.ToPosition(&ok);
                    if (ok)
                    {
                        o.posKnown = true; o.posX = xyz[0]; o.posY = xyz[1]; o.posZ = xyz[2];
                        logPosition = true;
                        logMsg << "SO" << ro.Addr.pri << " Pos X=" << std::fixed << std::setprecision(3)
                               << xyz[0] << " Y=" << xyz[1] << " Z=" << xyz[2];
                    }
                    break;
                }
                case Id::Positioning_SourceSpread:
                {
                    const float s = ro.Var.ToFloat(&ok);
                    if (ok) { o.spreadKnown = true; o.spread = s; }
                    break;
                }
                case Id::Positioning_SourceDelayMode:
                {
                    const std::uint8_t dm = ro.Var.ToUInt8(&ok);
                    if (ok) { o.dmKnown = true; o.delayMode = static_cast<int>(dm); }
                    break;
                }
                case Id::MatrixInput_ReverbSendGain:
                {
                    const float dB = ro.Var.ToFloat(&ok);
                    if (ok) { o.esKnown = true; o.enspaceDb = dB; }
                    break;
                }
                default:
                    return false;
                }
            }
            if (ok) g_needsRedraw = true;
            if (logPosition) pushLog(logMsg.str());
            return ok;
        };

        pushLog("Connecting to " + m_cfg.host + ":" + std::to_string(m_cfg.port)
                + " (Soundscape, SO#" + std::to_string(m_cfg.soundObjectLo)
                + (m_cfg.soundObjectHi != m_cfg.soundObjectLo
                       ? "-" + std::to_string(m_cfg.soundObjectHi) : std::string())
                + ")...");
        m_ds100->connect(m_cfg.host, m_cfg.port);
    }

    // ── DS100 connection (Soundscape-routing sub-mode) ────────────────────────

    // Subscribes to all 32 Function Groups' Mode (is it in use?) and Name
    // (display label) — independent of which sound objects are selected —
    // plus SoundObjectRouting_Gain/Mute for every sound object in
    // [m_routingLo, m_routingHi] against all 32 groups.
    void connectRouting()
    {
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;

        m_ds100 = std::make_unique<NanoOcp1::SoundscapeController>(m_scheduler);

        const auto numGroups = static_cast<std::int16_t>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS);

        std::vector<RO> active;
        for (std::int16_t fg = 1; fg <= numGroups; ++fg)
        {
            active.push_back(RO{ ROI::FunctionGroup_Mode, ROA{fg, 0} });
            active.push_back(RO{ ROI::FunctionGroup_Name, ROA{fg, 0} });
        }
        for (int so = m_routingLo; so <= m_routingHi; ++so)
        {
            const auto soAddr = static_cast<std::int16_t>(so);
            for (std::int16_t fg = 1; fg <= numGroups; ++fg)
            {
                active.push_back(RO{ ROI::SoundObjectRouting_Gain, ROA{soAddr, fg} });
                active.push_back(RO{ ROI::SoundObjectRouting_Mute, ROA{soAddr, fg} });
            }
        }
        addSpeakerLvlsSubscriptions(active);
        m_ds100->setActiveRemoteObjects(active);

        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            auto& ds = g_state.ds100;
            ds.view                 = SoundscapeView::Routing;
            ds.routingSoundObjectLo = m_routingLo;
            ds.routingSoundObjectHi = m_routingHi;
            ds.routingObjects.clear();
            for (int so = m_routingLo; so <= m_routingHi; ++so)
            {
                AppState::RoutingObjState o;
                o.soundObject = so;
                ds.routingObjects.push_back(o);
            }
            resetRoutingDisplay();
        }
        g_needsRedraw = true;

        m_ds100->onStateChanged = [this](CtrlState s) {
            std::string devModel;
            if (s == CtrlState::Connected)
            {
                devModel = modelStr(m_ds100->getConnectedDeviceModel());
                const int stack = m_ds100->getOcaStackIdent();
                if (stack >= 0)
                    devModel += " stack " + std::to_string(stack);
            }
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.ctrlState         = s;
                g_state.ds100.deviceModel = devModel;
                if (s == CtrlState::Disconnected)
                {
                    resetRoutingDisplay();
                    resetSpeakerDisplay();
                }
            }
            g_needsRedraw = true;
            pushLog("State: " + stateToStr(s));
        };

        m_ds100->onRemoteObjectReceived = [this](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool {
            using Id = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
            const int maxFg = static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS);
            bool ok = false;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                auto& ds = g_state.ds100;
                switch (ro.Id)
                {
                case Id::Positioning_SpeakerPosition:
                case Id::MatrixOutput_LevelMeterPostMute:
                    if (!m_cfg.speakerLvls) return false;
                    ok = updateSpeakerObject(ro);
                    break;
                case Id::FunctionGroup_Mode:
                {
                    if (ro.Addr.pri < 1 || ro.Addr.pri > maxFg)
                        return false;
                    const std::uint16_t mode = ro.Var.ToUInt16(&ok);
                    if (ok)
                    {
                        ds.fg[ro.Addr.pri].modeKnown = true;
                        ds.fg[ro.Addr.pri].mode      = static_cast<int>(mode);
                    }
                    break;
                }
                case Id::FunctionGroup_Name:
                {
                    if (ro.Addr.pri < 1 || ro.Addr.pri > maxFg)
                        return false;
                    const std::string name = ro.Var.ToString(&ok);
                    if (ok)
                    {
                        ds.fg[ro.Addr.pri].nameKnown = true;
                        ds.fg[ro.Addr.pri].name      = name;
                    }
                    break;
                }
                case Id::SoundObjectRouting_Gain:
                case Id::SoundObjectRouting_Mute:
                {
                    if (ro.Addr.sec < 1 || ro.Addr.sec > maxFg)
                        return false;
                    const int idx = ro.Addr.pri - ds.routingSoundObjectLo;
                    if (idx < 0 || idx >= static_cast<int>(ds.routingObjects.size()))
                        return false;
                    auto& cell = ds.routingObjects[idx].fg[ro.Addr.sec];
                    if (ro.Id == Id::SoundObjectRouting_Gain)
                    {
                        const float dB = ro.Var.ToFloat(&ok);
                        if (ok) { cell.gainKnown = true; cell.gainDb = dB; }
                    }
                    else
                    {
                        const std::uint8_t v = ro.Var.ToUInt8(&ok);
                        if (ok) { cell.muteKnown = true; cell.muted = (v == 1); }
                    }
                    break;
                }
                default:
                    return false;
                }
            }
            if (ok) g_needsRedraw = true;
            return ok;
        };

        std::string soRangeStr = std::to_string(m_routingLo);
        if (m_routingHi != m_routingLo)
            soRangeStr += "-" + std::to_string(m_routingHi);
        pushLog("Connecting to " + m_cfg.host + ":" + std::to_string(m_cfg.port)
                + " (Soundscape routing, SO#" + soRangeStr + ")...");
        m_ds100->connect(m_cfg.host, m_cfg.port);
    }

    // ── Soundscape-focus connection ───────────────────────────────────────────

    // Subscribes to exactly one remote object and streams every GetValue response
    // / change notification for it into the timestamped focus log.
    void connectFocus()
    {
        using RO = NanoOcp1::SoundscapeController::RemoteObject;

        m_ds100 = std::make_unique<NanoOcp1::SoundscapeController>(m_scheduler);

        const SORemObjAddr addr{static_cast<std::int16_t>(m_cfg.soundObjectLo),
                                 static_cast<std::int16_t>(m_cfg.addr2)};

        m_ds100->setActiveRemoteObjects({ RO{ m_cfg.focusParamId, addr } });

        m_ds100->onStateChanged = [this](CtrlState s) {
            std::string devModel;
            if (s == CtrlState::Connected)
            {
                devModel = modelStr(m_ds100->getConnectedDeviceModel());
                const int stack = m_ds100->getOcaStackIdent();
                if (stack >= 0)
                    devModel += " stack " + std::to_string(stack);
            }
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.ctrlState         = s;
                g_state.ds100.deviceModel = devModel;
                if (s == CtrlState::Disconnected)
                    resetFocusDisplay();
            }
            g_needsRedraw = true;
            pushFocusLog("State: " + stateToStr(s));
        };

        m_ds100->onRemoteObjectReceived = [this](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool {
            if (ro.Id != m_cfg.focusParamId)
                return false;
            {
                std::lock_guard<std::mutex> lk(g_stateMutex);
                g_state.focus.valueKnown = true;
                g_state.focus.lastValue  = ro.Var;
                ++g_state.focus.updateCount;
                g_needsRedraw = true;
            }
            pushFocusLog(m_cfg.focusParamName + " -> " + formatVariant(ro.Var));
            return true;
        };

        std::string addrStr = std::to_string(addr.pri);
        if (addr.sec != 0)
            addrStr += "," + std::to_string(addr.sec);
        pushFocusLog("Connecting to " + m_cfg.host + ":" + std::to_string(m_cfg.port)
                + " (Soundscape focus, " + m_cfg.focusParamName + " @ " + addrStr + ")...");
        m_ds100->connect(m_cfg.host, m_cfg.port);
    }

    // ── Helpers ───────────────────────────────────────────────────────────────

    void teardown()
    {
        m_amp.reset();
        m_ds100.reset();
    }

    static void resetAmpDisplay()
    {
        // called with g_stateMutex held
        auto& a = g_state.amp;
        a.powerKnown = false;
        for (auto& ch : a.ch)
            ch = ChState{};
    }

    static void resetSoDisplay()
    {
        // called with g_stateMutex held
        auto& ds = g_state.ds100;
        ds.deviceModel = "";
        for (auto& o : ds.objects)
        {
            const int so = o.soundObject;
            o = AppState::SoObjState{};
            o.soundObject = so;
        }
    }

    static void resetRoutingDisplay()
    {
        // called with g_stateMutex held
        for (auto& fg : g_state.ds100.fg)
            fg = AppState::FunctionGroupInfo{};
        for (auto& o : g_state.ds100.routingObjects)
        {
            const int so = o.soundObject;
            o = AppState::RoutingObjState{};
            o.soundObject = so;
        }
    }

    static void resetSpeakerDisplay()
    {
        // called with g_stateMutex held
        for (auto& s : g_state.ds100.speakerObjects)
        {
            const int ch = s.outputChannel;
            s = AppState::SpeakerObjState{};
            s.outputChannel = ch;
        }
    }

    // Appends the --speakerlvls subscriptions (every possible output
    // channel's Positioning_SpeakerPosition + MatrixOutput_LevelMeterPostMute)
    // to `active` — shared by connectDS100() and connectRouting() since the
    // optional loudspeaker block can be shown under either view. No-op unless
    // m_cfg.speakerLvls is set.
    void addSpeakerLvlsSubscriptions(std::vector<NanoOcp1::SoundscapeController::RemoteObject>& active) const
    {
        if (!m_cfg.speakerLvls) return;
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto maxOut = static_cast<std::int16_t>(NanoOcp1::SoundscapeController::sc_MAX_OUTPUT_CHANNELS);
        for (std::int16_t out = 1; out <= maxOut; ++out)
        {
            active.push_back(RO{ ROI::Positioning_SpeakerPosition,     ROA{out, 0} });
            active.push_back(RO{ ROI::MatrixOutput_LevelMeterPostMute, ROA{out, 0} });
        }
    }

    // Updates g_state.ds100.speakerObjects for a Positioning_SpeakerPosition
    // or MatrixOutput_LevelMeterPostMute notification. Called with
    // g_stateMutex held by connectDS100()/connectRouting()'s
    // onRemoteObjectReceived. Returns whether the value was decoded.
    static bool updateSpeakerObject(const NanoOcp1::SoundscapeController::RemoteObject& ro)
    {
        using Id = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        auto& spk = g_state.ds100.speakerObjects;
        const int idx = ro.Addr.pri - 1; // output channels are 1-based, speakerObjects is 0-based 1..N
        if (idx < 0 || idx >= static_cast<int>(spk.size()))
            return false;
        bool ok = false;
        if (ro.Id == Id::Positioning_SpeakerPosition)
        {
            const auto sixDof = ro.Var.ToAimingAndPosition(&ok); // [hor, vert, rot, x, y, z]
            if (ok)
            {
                spk[idx].posKnown   = true;
                spk[idx].hasSpeaker = std::any_of(sixDof.begin(), sixDof.end(),
                                                   [](float v) { return v != 0.0f; });
            }
        }
        else if (ro.Id == Id::MatrixOutput_LevelMeterPostMute)
        {
            const float dB = ro.Var.ToFloat(&ok);
            if (ok) { spk[idx].levelKnown = true; spk[idx].levelDb = dB; }
        }
        return ok;
    }

    static void resetFocusDisplay()
    {
        // called with g_stateMutex held
        g_state.focus.valueKnown = false;
    }

    // Returns the sound-object numbers a command should apply to: just
    // `target` if non-zero (provided it lies within [lo, hi]), or every
    // object in [lo, hi] when `target == 0` (broadcast).
    static std::vector<int> resolveTargets(int target, int lo, int hi)
    {
        std::vector<int> result;
        if (target != 0)
        {
            if (target >= lo && target <= hi)
                result.push_back(target);
        }
        else
        {
            for (int so = lo; so <= hi; ++so)
                result.push_back(so);
        }
        return result;
    }

    // Overview commands resolve against the active --soundscape range.
    std::vector<int> resolveTargets(int target) const
    {
        return resolveTargets(target, m_cfg.soundObjectLo, m_cfg.soundObjectHi);
    }

    // Resolves `target` (within [lo, hi]) via resolveTargets(), calls
    // `sendOne(so)` for each resulting sound object, and logs one summary line
    // ("Sent <label> to <sent>/<total> object(s)") rather than one line per
    // object.
    void applyToTargets(const std::string& label, int target, int lo, int hi,
                         const std::function<bool(int)>& sendOne)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        const auto targets = resolveTargets(target, lo, hi);
        if (targets.empty())
        {
            pushLog("Sound object out of range (" + std::to_string(lo) + "-" + std::to_string(hi) + ")");
            return;
        }
        int sent = 0;
        for (int so : targets)
            if (sendOne(so)) ++sent;
        std::ostringstream oss;
        oss << "Sent " << label << " to " << sent << "/" << targets.size() << " object(s)";
        pushLog(oss.str());
    }

    // Overview commands resolve against the active --soundscape range.
    void applyToTargets(const std::string& label, int target, const std::function<bool(int)>& sendOne)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        const auto targets = resolveTargets(target);
        if (targets.empty())
        {
            pushLog("Sound object out of range (" + std::to_string(m_cfg.soundObjectLo)
                    + "-" + std::to_string(m_cfg.soundObjectHi) + ")");
            return;
        }
        int sent = 0;
        for (int so : targets)
            if (sendOne(so)) ++sent;
        std::ostringstream oss;
        oss << "Sent " << label << " to " << sent << "/" << targets.size() << " object(s)";
        pushLog(oss.str());
    }

    static std::string fixedStr(float v, int prec)
    {
        std::ostringstream oss;
        oss << std::fixed << std::setprecision(prec) << v;
        return oss.str();
    }

    // Sets one axis of sound-object `so`'s position, keeping its other two
    // axes at their last-known value (Positioning_SourcePosition is a single
    // XYZ blob, so a single-axis change still needs all three coordinates).
    bool sendPositionAxis(int so, int axis, float v)
    {
        float cx, cy, cz;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            const auto& o = g_state.ds100.objects[so - g_state.ds100.soundObjectLo];
            cx = o.posX; cy = o.posY; cz = o.posZ;
        }
        if      (axis == 0) cx = v;
        else if (axis == 1) cy = v;
        else                cz = v;

        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        return m_ds100->setObjectValue(RO{ROI::Positioning_SourcePosition, ROA{static_cast<std::int16_t>(so), 0},
                                           NanoOcp1::Variant{static_cast<std::float_t>(cx),
                                                             static_cast<std::float_t>(cy),
                                                             static_cast<std::float_t>(cz)}});
    }

    // ── Data members ──────────────────────────────────────────────────────────

    Config m_cfg;
    // One scheduler shared by this demo's controllers (higher layer owns it; NanoOcp has no singleton).
    std::shared_ptr<NanoOcp1::NanoTimerScheduler> m_scheduler{ std::make_shared<NanoOcp1::NanoTimerScheduler>() };
    std::unique_ptr<NanoOcp1::AmpController>   m_amp;
    std::unique_ptr<NanoOcp1::SoundscapeController> m_ds100;

    // Soundobject-Routing sub-mode state (only meaningful while mode == Soundscape).
    bool m_routingActive{false};
    int  m_routingLo{0};
    int  m_routingHi{0};
};
