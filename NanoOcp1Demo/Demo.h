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

#include <memory>
#include <mutex>
#include <sstream>
#include <string>

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
        // DS100-specific
        int soundObject{1};
        // Soundscape-focus-specific (only used when mode == SoundscapeFocus)
        SORemObjIdent focusParamId{SORemObjIdent::Invalid};
        std::string   focusParamName;
        int           addr2{0};
    };

    explicit Demo(Config cfg) : m_cfg(std::move(cfg))
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        g_state.address       = m_cfg.host;
        g_state.port          = m_cfg.port;
        g_state.mode          = m_cfg.mode;
        g_state.amp.type          = m_cfg.ampType;
        g_state.amp.channelCount  = static_cast<int>(m_cfg.channelCount);
        g_state.ds100.soundObject = m_cfg.soundObject;
        g_state.focus.paramName   = m_cfg.focusParamName;
        g_state.focus.paramId     = m_cfg.focusParamId;
        g_state.focus.addr        = SORemObjAddr{static_cast<std::int16_t>(m_cfg.soundObject),
                                                  static_cast<std::int16_t>(m_cfg.addr2)};
    }

    ~Demo() { teardown(); }

    // ── Transport ─────────────────────────────────────────────────────────────

    void connect()
    {
        teardown();

        switch (m_cfg.mode)
        {
        case DemoMode::Amp:             connectAmp();   break;
        case DemoMode::Soundscape:      connectDS100(); break;
        case DemoMode::SoundscapeFocus: connectFocus(); break;
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
            case DemoMode::Amp:             resetAmpDisplay();   break;
            case DemoMode::Soundscape:      resetSoDisplay();    break;
            case DemoMode::SoundscapeFocus: resetFocusDisplay(); break;
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

    void cmdPositionX(float x)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        float cx, cy, cz;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            cx = x;
            cy = g_state.ds100.posY;
            cz = g_state.ds100.posZ;
        }
        sendPosition(cx, cy, cz);
    }

    void cmdPositionY(float y)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        float cx, cy, cz;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            cx = g_state.ds100.posX;
            cy = y;
            cz = g_state.ds100.posZ;
        }
        sendPosition(cx, cy, cz);
    }

    void cmdPositionZ(float z)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        float cx, cy, cz;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            cx = g_state.ds100.posX;
            cy = g_state.ds100.posY;
            cz = z;
        }
        sendPosition(cx, cy, cz);
    }

    void cmdSpread(float s)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        if (s < 0.0f || s > 1.0f) { pushLog("Spread out of range (0.0-1.0)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        if (!m_ds100->setObjectValue(RO{ROI::Positioning_SourceSpread, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::float_t>(s)}}))
            pushLog("setObjectValue(spread) failed (not connected?)");
        else
        {
            std::ostringstream oss;
            oss << "Sent spread " << std::fixed << std::setprecision(3) << s;
            pushLog(oss.str());
        }
    }

    void cmdDelayMode(int dm)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        if (dm < 0 || dm > 2) { pushLog("Delay mode out of range (0, 1, or 2)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        if (!m_ds100->setObjectValue(RO{ROI::Positioning_SourceDelayMode, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::uint8_t>(dm)}}))
            pushLog("setObjectValue(delayMode) failed (not connected?)");
        else
            pushLog("Sent delay mode " + std::to_string(dm));
    }

    void cmdMatrixInputGain(float dB)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        if (dB < -120.0f || dB > 24.0f) { pushLog("Gain out of range (-120.0 to +24.0 dB)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        if (!m_ds100->setObjectValue(RO{ROI::MatrixInput_Gain, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::float_t>(dB)}}))
            pushLog("setObjectValue(input gain) failed (not connected?)");
        else
        {
            std::ostringstream oss;
            oss << "Sent input gain " << std::fixed << std::setprecision(1) << dB << " dB";
            pushLog(oss.str());
        }
    }

    void cmdMatrixInputMute(bool mute)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        // d&b convention: 1 = muted, 2 = unmuted (not the boolean 0/1)
        if (!m_ds100->setObjectValue(RO{ROI::MatrixInput_Mute, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::uint8_t>(mute ? 1 : 2)}}))
            pushLog("setObjectValue(input mute) failed (not connected?)");
        else
            pushLog(std::string("Sent input ") + (mute ? "MUTE" : "UNMUTE"));
    }

    void cmdEnSpace(float dB)
    {
        if (!m_ds100) { pushLog("Not in Soundscape mode or not connected."); return; }
        if (dB < -120.0f || dB > 24.0f) { pushLog("Gain out of range (-120.0 to +24.0 dB)"); return; }
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        if (!m_ds100->setObjectValue(RO{ROI::MatrixInput_ReverbSendGain, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::float_t>(dB)}}))
            pushLog("setObjectValue(enspace) failed (not connected?)");
        else
        {
            std::ostringstream oss;
            oss << "Sent EnSpace " << std::fixed << std::setprecision(1) << dB << " dB";
            pushLog(oss.str());
        }
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
        m_amp = std::make_unique<NanoOcp1::AmpController>();
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

    // ── DS100 connection ──────────────────────────────────────────────────────

    void connectDS100()
    {
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;

        m_ds100 = std::make_unique<NanoOcp1::SoundscapeController>();

        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);

        m_ds100->setActiveRemoteObjects({
            RO{ ROI::MatrixInput_LevelMeterPreMute, ROA{so, 0}  },
            RO{ ROI::MatrixInput_Gain,              ROA{so, 0}  },
            RO{ ROI::MatrixInput_Mute,              ROA{so, 0}  },
            RO{ ROI::Positioning_SourcePosition,    ROA{so, 0}  },
            RO{ ROI::Positioning_SourceSpread,      ROA{so, 0}  },
            RO{ ROI::Positioning_SourceDelayMode,   ROA{so, 0}  },
            RO{ ROI::MatrixInput_ReverbSendGain,     ROA{so, 0}  },
        });

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
                    resetSoDisplay();
            }
            g_needsRedraw = true;
            pushLog("State: " + stateToStr(s));
        };

        m_ds100->onRemoteObjectReceived = [this](const NanoOcp1::SoundscapeController::RemoteObject& ro) -> bool {
            using Id = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
            bool ok = false;

            switch (ro.Id)
            {
            case Id::MatrixInput_LevelMeterPreMute:
            {
                const float dB = ro.Var.ToFloat(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.levelKnown = true;
                    g_state.ds100.levelDb    = dB;
                    g_needsRedraw = true;
                }
                break;
            }
            case Id::MatrixInput_Gain:
            {
                const float dB = ro.Var.ToFloat(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.gainKnown = true;
                    g_state.ds100.gainDb    = dB;
                    g_needsRedraw = true;
                }
                break;
            }
            case Id::MatrixInput_Mute:
            {
                const std::uint8_t v = ro.Var.ToUInt8(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.muteKnown = true;
                    g_state.ds100.muted     = (v == 1);
                    g_needsRedraw = true;
                }
                break;
            }
            case Id::Positioning_SourcePosition:
            {
                const auto xyz = ro.Var.ToPosition(&ok);
                if (ok)
                {
                    {
                        std::lock_guard<std::mutex> lk(g_stateMutex);
                        g_state.ds100.posKnown = true;
                        g_state.ds100.posX     = xyz[0];
                        g_state.ds100.posY     = xyz[1];
                        g_state.ds100.posZ     = xyz[2];
                        g_needsRedraw = true;
                    }
                    std::ostringstream oss;
                    oss << "Pos X=" << std::fixed << std::setprecision(3) << xyz[0]
                        << " Y=" << xyz[1] << " Z=" << xyz[2];
                    pushLog(oss.str());
                }
                break;
            }
            case Id::Positioning_SourceSpread:
            {
                const float s = ro.Var.ToFloat(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.spreadKnown = true;
                    g_state.ds100.spread      = s;
                    g_needsRedraw = true;
                }
                break;
            }
            case Id::Positioning_SourceDelayMode:
            {
                const std::uint8_t dm = ro.Var.ToUInt8(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.dmKnown    = true;
                    g_state.ds100.delayMode  = static_cast<int>(dm);
                    g_needsRedraw = true;
                }
                break;
            }
            case Id::MatrixInput_ReverbSendGain:
            {
                const float dB = ro.Var.ToFloat(&ok);
                if (ok)
                {
                    std::lock_guard<std::mutex> lk(g_stateMutex);
                    g_state.ds100.esKnown    = true;
                    g_state.ds100.enspaceDb  = dB;
                    g_needsRedraw = true;
                }
                break;
            }
            default:
                return false;
            }
            return ok;
        };

        pushLog("Connecting to " + m_cfg.host + ":" + std::to_string(m_cfg.port)
                + " (Soundscape, SO#" + std::to_string(m_cfg.soundObject) + ")...");
        m_ds100->connect(m_cfg.host, m_cfg.port);
    }

    // ── Soundscape-focus connection ───────────────────────────────────────────

    // Subscribes to exactly one remote object and streams every GetValue response
    // / change notification for it into the timestamped focus log.
    void connectFocus()
    {
        using RO = NanoOcp1::SoundscapeController::RemoteObject;

        m_ds100 = std::make_unique<NanoOcp1::SoundscapeController>();

        const SORemObjAddr addr{static_cast<std::int16_t>(m_cfg.soundObject),
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
        ds.levelKnown  = false;
        ds.gainKnown   = false;
        ds.muteKnown   = false;
        ds.posKnown    = false;
        ds.spreadKnown = false;
        ds.dmKnown     = false;
        ds.esKnown     = false;
    }

    static void resetFocusDisplay()
    {
        // called with g_stateMutex held
        g_state.focus.valueKnown = false;
    }

    void sendPosition(float x, float y, float z)
    {
        using ROI = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
        using ROA = NanoOcp1::SoundscapeController::RemObjAddr;
        using RO  = NanoOcp1::SoundscapeController::RemoteObject;
        const auto so = static_cast<std::int16_t>(m_cfg.soundObject);
        if (!m_ds100->setObjectValue(RO{ROI::Positioning_SourcePosition, ROA{so, 0},
                                        NanoOcp1::Variant{static_cast<std::float_t>(x),
                                                          static_cast<std::float_t>(y),
                                                          static_cast<std::float_t>(z)}}))
            pushLog("setObjectValue(position) failed (not connected?)");
        else
        {
            std::ostringstream oss;
            oss << "Sent pos X=" << std::fixed << std::setprecision(3)
                << x << " Y=" << y << " Z=" << z;
            pushLog(oss.str());
        }
    }

    // ── Data members ──────────────────────────────────────────────────────────

    Config m_cfg;
    std::unique_ptr<NanoOcp1::AmpController>   m_amp;
    std::unique_ptr<NanoOcp1::SoundscapeController> m_ds100;
};
