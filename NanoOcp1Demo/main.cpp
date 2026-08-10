/* Copyright (c) 2022-2026, Christian Ahrens
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

/**
 * NanoOcp1Demo — two-mode terminal UI for the NanoOcp1 library.
 *
 * Amp mode (--amp):
 *   Connects to a d&b amplifier using AmpController.  Shows per-channel gain,
 *   mute state, and protection status (ISP/GR/OVL/headroom).
 *
 * Soundscape mode (--soundscape <N>):
 *   Connects to a d&b Soundscape signal engine (DS100 / DS110 / DS100M / vCore)
 *   using SoundscapeController.  Monitors and controls sound object N: input gain,
 *   position (XYZ), spread, delay mode, En-Space send gain.  Also shows the input
 *   level meter for that sound object.  The exact device model is identified
 *   automatically via GUID.
 *
 * Usage:
 *   NanoOcp1Demo [host [port]] [--amp | --soundscape <N>] [--type dx|dy|5d] [--ch <n>]
 *   NanoOcp1Demo -h | --help
 *   Defaults: 127.0.0.1  50014  --amp  --type dy  --ch 4
 */

// ── Platform setup ────────────────────────────────────────────────────────────
#if defined(_WIN32) || defined(_WIN64)
  #ifndef WIN32_LEAN_AND_MEAN
    #define WIN32_LEAN_AND_MEAN
  #endif
  #ifndef NOMINMAX
    #define NOMINMAX
  #endif
  #include <windows.h>
  static void setupTerminal()
  {
      // Enable UTF-8 output so box-drawing and block characters render correctly.
      SetConsoleOutputCP(CP_UTF8);
      // Enable ANSI/VT escape-sequence processing (colour, cursor movement).
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      DWORD  mode = 0;
      if (GetConsoleMode(h, &mode))
          SetConsoleMode(h, mode | ENABLE_VIRTUAL_TERMINAL_PROCESSING);
  }
#else
  static void setupTerminal() {}
#endif

#include <algorithm>
#include <atomic>
#include <cctype>
#include <chrono>
#include <cmath>
#include <cstdint>
#include <deque>
#include <iomanip>
#include <iostream>
#include <memory>
#include <mutex>
#include <sstream>
#include <string>
#include <thread>
#include <vector>

#include "AmpController.h"
#include "SoundscapeController.h"

// ── ANSI helpers ──────────────────────────────────────────────────────────────
namespace Ansi
{
    static const char* Reset  = "\033[0m";
    static const char* Bold   = "\033[1m";
    static const char* Dim    = "\033[2m";
    static const char* Red    = "\033[31m";
    static const char* Green  = "\033[32m";
    static const char* Yellow = "\033[33m";
    static const char* Cyan   = "\033[36m";
    static const char* Save   = "\033[s";   // save cursor position
    static const char* Rest   = "\033[u";   // restore cursor position
    static const char* Eol    = "\033[K";   // erase to end of current line
    static const char* Home   = "\033[H";   // move cursor to top-left (1,1)
    static const char* Clear  = "\033[2J";  // erase entire screen
}

// ── Panel dimensions ──────────────────────────────────────────────────────────
// Both modes use exactly 14 fixed rows + 5 log rows = 19 total.
// Layout (fixed rows):
//   1  title         2  sep         3  host/port
//   4  status        5  mode row 1  6-9  mode rows 2-5
//   10 sep           11-13 commands  14 Events header
static constexpr int kLogLines   = 5;
static constexpr int kFixedLines = 14;
static constexpr int kPanelLines = kFixedLines + kLogLines; // 19

// ── Shared UI state ───────────────────────────────────────────────────────────

enum class DemoMode { Amp, Soundscape };

using CtrlState = NanoOcp1::Ocp1Controller::State;

struct ChState
{
    bool  gainKnown{false};    float gainDb{0.0f};
    bool  muteKnown{false};    bool  muted{false};
    bool  ispKnown{false};     bool  isp{false};
    bool  grKnown{false};      bool  gr{false};
    bool  ovlKnown{false};     bool  ovl{false};
    bool  hrKnown{false};      float headroomDb{0.0f};
};

struct AppState
{
    std::string address{"127.0.0.1"};
    int         port{50014};
    CtrlState   ctrlState{CtrlState::Disconnected};
    DemoMode    mode{DemoMode::Amp};

    struct AmpState
    {
        NanoOcp1::AmpController::AmpType type{NanoOcp1::AmpController::AmpType::Dy};
        int      channelCount{4};
        bool     powerKnown{false};
        bool     powerOn{false};
        ChState  ch[4];
    } amp;

    struct SoState
    {
        int         soundObject{1};
        std::string deviceModel;
        bool  levelKnown{false};   float levelDb{-60.0f};
        bool  gainKnown{false};    float gainDb{0.0f};
        bool  muteKnown{false};    bool  muted{false};
        bool  posKnown{false};
        float posX{0.0f};          float posY{0.0f};  float posZ{0.0f};
        bool  spreadKnown{false};  float spread{0.0f};
        bool  dmKnown{false};      int   delayMode{0};
        bool  esKnown{false};      float enspaceDb{-120.0f};
    } ds100;
};

static std::mutex              g_ioMutex;
static std::mutex              g_stateMutex;
static AppState                g_state;
static std::deque<std::string> g_log;
static std::atomic<bool>       g_needsRedraw{true};
static std::atomic<bool>       g_quit{false};

// ── Utility ───────────────────────────────────────────────────────────────────

static std::string fillBar(float val, float lo, float hi, int width = 16)
{
    const float clamped = std::max(lo, std::min(hi, val));
    const float frac    = (hi > lo) ? (clamped - lo) / (hi - lo) : 0.0f;
    const int   n       = static_cast<int>(std::round(frac * static_cast<float>(width)));
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
        bar += (i < n) ? "\xe2\x96\x88" : "\xe2\x96\x91"; // U+2588 full / U+2591 light
    bar += "]";
    return bar;
}

// Colour-segmented VU-meter-style bar (green/yellow/red zones), redrawn on every
// live value update from the device — used for signal-level and channel-gain
// readouts, as opposed to fillBar()'s plain single-colour rendering for
// positional/target controls (spread, position, delay mode).
static std::string meterBar(float val, float lo, float hi, int width = 16)
{
    const float clamped = std::max(lo, std::min(hi, val));
    const float frac    = (hi > lo) ? (clamped - lo) / (hi - lo) : 0.0f;
    const int   n       = static_cast<int>(std::round(frac * static_cast<float>(width)));
    std::string bar = "[";
    for (int i = 0; i < width; ++i)
    {
        if (i >= n)
        {
            bar += "\xe2\x96\x91"; // U+2591 light — unlit cell
            continue;
        }
        const float cellFrac = static_cast<float>(i + 1) / static_cast<float>(width);
        const char* color = (cellFrac > 0.9f)  ? Ansi::Red
                           : (cellFrac > 0.75f) ? Ansi::Yellow
                                                 : Ansi::Green;
        bar += std::string(color) + "\xe2\x96\x88" + Ansi::Reset; // U+2588 full, zone-coloured
    }
    bar += "]";
    return bar;
}

static std::string stateToStr(CtrlState s)
{
    switch (s)
    {
    case CtrlState::Disconnected: return "disconnected";
    case CtrlState::Connecting:   return "connecting...";
    case CtrlState::Subscribing:  return "subscribing...";
    case CtrlState::Subscribed:   return "subscribed";
    case CtrlState::GetValues:    return "reading values...";
    case CtrlState::Connected:    return "connected";
    default:                      return "unknown";
    }
}

static std::string ampTypeStr(NanoOcp1::AmpController::AmpType t)
{
    switch (t)
    {
    case NanoOcp1::AmpController::AmpType::Dx:    return "Dx";
    case NanoOcp1::AmpController::AmpType::Dy:    return "Dy";
    case NanoOcp1::AmpController::AmpType::FiveD: return "5D";
    default:                                       return "?";
    }
}

static std::string modelStr(NanoOcp1::SoundscapeController::DbDeviceModel m)
{
    switch (m)
    {
    case NanoOcp1::SoundscapeController::DbDeviceModel::DS100:  return "DS100";
    case NanoOcp1::SoundscapeController::DbDeviceModel::DS110:  return "DS110";
    case NanoOcp1::SoundscapeController::DbDeviceModel::DS100M: return "DS100M";
    case NanoOcp1::SoundscapeController::DbDeviceModel::vCore:  return "vCore";
    default:                                                return "unknown";
    }
}

static void pushLog(const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        g_log.push_back(msg);
        while (g_log.size() > static_cast<std::size_t>(kLogLines))
            g_log.pop_front();
    }
    g_needsRedraw = true;
}

// ── Panel renderers ───────────────────────────────────────────────────────────
// Each emits exactly kFixedLines newline-terminated rows.
// Must be called with g_ioMutex held.

static std::string makeSep()
{
    std::string s;
    s.reserve(50 * 3);
    for (int i = 0; i < 50; ++i)
        s += "\xe2\x94\x80"; // U+2500 BOX DRAWINGS LIGHT HORIZONTAL
    return s;
}

static void renderStatusRow(std::ostringstream& o, CtrlState cs)
{
    o << " Status     ";
    if (cs == CtrlState::Connected)
        o << Ansi::Green  << "\xe2\x97\x8f " << Ansi::Reset; // U+25CF filled circle
    else if (cs == CtrlState::Disconnected)
        o << Ansi::Red    << "\xe2\x97\x8b " << Ansi::Reset; // U+25CB open circle
    else
        o << Ansi::Yellow << "\xe2\x97\x8b " << Ansi::Reset;
    o << stateToStr(cs);
}

static void renderAmpPanel(const AppState& st, const std::vector<std::string>& log)
{
    const auto  sep = makeSep();
    const auto& a   = st.amp;

    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // row 1: title
    {
        std::ostringstream o;
        o << Ansi::Bold << " NanoOcp1 Demo — Amplifier ("
          << ampTypeStr(a.type) << " " << a.channelCount << "ch)" << Ansi::Reset;
        row(o.str());
    }
    row(sep);  // row 2

    // row 3: host / port
    {
        std::ostringstream o;
        o << " Host       " << Ansi::Bold << st.address << Ansi::Reset
          << "   Port " << Ansi::Bold << st.port << Ansi::Reset;
        row(o.str());
    }

    // row 4: status
    {
        std::ostringstream o;
        renderStatusRow(o, st.ctrlState);
        row(o.str());
    }

    // row 5: power
    {
        std::ostringstream o;
        o << " Power      ";
        if (!a.powerKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else if (a.powerOn)
            o << Ansi::Green << "\xe2\x96\xa0 ON"  << Ansi::Reset; // U+25A0 filled square
        else
            o << Ansi::Red   << "\xe2\x96\xa1 OFF" << Ansi::Reset; // U+25A1 open square
        row(o.str());
    }

    // rows 6-9: channels (always 4 rows, greyed out if beyond channelCount)
    for (int i = 0; i < 4; ++i)
    {
        std::ostringstream o;
        const bool active = (i < a.channelCount);
        if (!active)
        {
            o << Ansi::Dim << " Ch" << (i + 1) << "  —" << Ansi::Reset;
            row(o.str());
            continue;
        }

        const ChState& ch = a.ch[i];
        o << " Ch" << (i + 1) << "  ";

        // gain
        o << "Gain ";
        if (!ch.gainKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
            o << std::fixed << std::setprecision(1) << std::setw(6) << ch.gainDb << " dB";

        // mute
        o << "  ";
        if (!ch.muteKnown)
            o << Ansi::Dim << "mut:?" << Ansi::Reset;
        else if (ch.muted)
            o << Ansi::Red << "MUT" << Ansi::Reset;
        else
            o << Ansi::Dim << "mut" << Ansi::Reset;

        // ISP GR OVL
        auto boolLed = [&](bool known, bool active, const char* label) {
            o << "  ";
            if (!known)
                o << Ansi::Dim << label << ":?" << Ansi::Reset;
            else if (active)
                o << Ansi::Yellow << label << Ansi::Reset;
            else
                o << Ansi::Dim << label << Ansi::Reset;
        };
        boolLed(ch.ispKnown, ch.isp, "ISP");
        boolLed(ch.grKnown,  ch.gr,  "GR");
        boolLed(ch.ovlKnown, ch.ovl, "OVL");

        // headroom
        o << "  ";
        if (!ch.hrKnown)
            o << Ansi::Dim << "Hd:?" << Ansi::Reset;
        else
        {
            o << "Hd:";
            if (ch.headroomDb < -99.0f)
                o << Ansi::Dim << "  -\xe2\x88\x9e" << Ansi::Reset; // -∞
            else
                o << std::fixed << std::setprecision(1) << ch.headroomDb;
        }

        row(o.str());
    }

    row(sep);  // row 10

    // rows 11-13: command reference
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    row(" 1 / 0     power on/off          g <ch> <dB>   set gain (ch: 1-4)");
    row(" m <ch> <1|0>   mute/unmute (ch: 1-4,  1=muted  0=unmuted)       q  quit");

    // row 14: events header
    row(std::string(Ansi::Dim) + " Events" + Ansi::Reset);

    // rows 15-19: log
    for (int i = 0; i < kLogLines; ++i)
    {
        if (i < static_cast<int>(log.size()))
            row(std::string(" ") + Ansi::Cyan + log[i] + Ansi::Reset);
        else
            row("");
    }
}

static void renderDS100Panel(const AppState& st, const std::vector<std::string>& log)
{
    const auto  sep   = makeSep();
    const auto& ds    = st.ds100;

    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // row 1: title
    {
        std::ostringstream o;
        o << Ansi::Bold << " NanoOcp1 Demo — Soundscape Sound Object #"
          << ds.soundObject << Ansi::Reset;
        row(o.str());
    }
    row(sep);  // row 2

    // row 3: host / port / device model
    {
        std::ostringstream o;
        o << " Host       " << Ansi::Bold << st.address << Ansi::Reset
          << "   Port " << Ansi::Bold << st.port << Ansi::Reset;
        if (!ds.deviceModel.empty())
            o << "   " << Ansi::Dim << ds.deviceModel << Ansi::Reset;
        row(o.str());
    }

    // row 4: status
    {
        std::ostringstream o;
        renderStatusRow(o, st.ctrlState);
        row(o.str());
    }

    // row 5: input gain + level meter
    {
        std::ostringstream o;
        o << " Level      ";
        if (!ds.levelKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
        {
            o << meterBar(ds.levelDb, -60.0f, 0.0f, 14);
            o << " " << std::fixed << std::setprecision(1) << std::setw(6) << ds.levelDb << " dBFS";
        }
        o << "   Gain ";
        if (!ds.gainKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
            o << std::fixed << std::setprecision(1) << std::setw(6) << ds.gainDb << " dB";
        o << "  ";
        if (!ds.muteKnown)
            o << Ansi::Dim << "mut:?" << Ansi::Reset;
        else if (ds.muted)
            o << Ansi::Red << "MUT" << Ansi::Reset;
        else
            o << Ansi::Dim << "mut" << Ansi::Reset;
        row(o.str());
    }

    // row 6: position
    {
        std::ostringstream o;
        o << " Position   ";
        if (!ds.posKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
        {
            o << std::fixed << std::setprecision(3);
            o << "X: " << Ansi::Bold << ds.posX << Ansi::Reset << " m"
              << "  Y: " << Ansi::Bold << ds.posY << Ansi::Reset << " m"
              << "  Z: " << Ansi::Bold << ds.posZ << Ansi::Reset << " m";
        }
        row(o.str());
    }

    // row 7: spread
    {
        std::ostringstream o;
        o << " Spread     ";
        if (!ds.spreadKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
        {
            o << fillBar(ds.spread, 0.0f, 1.0f, 16);
            o << "  " << std::fixed << std::setprecision(3) << ds.spread;
        }
        row(o.str());
    }

    // row 8: delay mode
    {
        std::ostringstream o;
        o << " Delay Mode ";
        if (!ds.dmKnown)
        {
            o << Ansi::Dim << "?" << Ansi::Reset;
        }
        else
        {
            const char* label = (ds.delayMode == 0) ? "off"
                              : (ds.delayMode == 1) ? "tight"
                              : (ds.delayMode == 2) ? "full" : "?";
            o << Ansi::Bold << ds.delayMode << Ansi::Reset
              << " (" << label << ")";
        }
        row(o.str());
    }

    // row 9: En-Space send gain
    {
        std::ostringstream o;
        o << " EnSpace    ";
        if (!ds.esKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
        {
            o << fillBar(ds.enspaceDb, -120.0f, 24.0f, 16);
            o << "  " << std::fixed << std::setprecision(1) << std::setw(6) << ds.enspaceDb << " dB";
        }
        row(o.str());
    }

    row(sep);  // row 10

    // rows 11-13: command reference
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    row(" x/y/z <m>   set position XYZ (meters)   sp <0-1>  spread   dm <0|1|2>  delay");
    row(" ig <dB>  input gain   mm <1|0>  mute   es <dB>  En-Space gain   q  quit");

    // row 14: events header
    row(std::string(Ansi::Dim) + " Events" + Ansi::Reset);

    // rows 15-19: log
    for (int i = 0; i < kLogLines; ++i)
    {
        if (i < static_cast<int>(log.size()))
            row(std::string(" ") + Ansi::Cyan + log[i] + Ansi::Reset);
        else
            row("");
    }
}

static void renderPanel()
{
    AppState state;
    std::vector<std::string> log;
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        state = g_state;
        log.assign(g_log.begin(), g_log.end());
    }

    if (state.mode == DemoMode::Amp)
        renderAmpPanel(state, log);
    else
        renderDS100Panel(state, log);
}

// ── Background redraw thread ──────────────────────────────────────────────────
static void redrawLoop()
{
    while (!g_quit)
    {
        if (g_needsRedraw.exchange(false))
        {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            std::cout << Ansi::Save << Ansi::Home;
            renderPanel();
            std::cout << Ansi::Rest;
            std::cout.flush();
        }
        std::this_thread::sleep_for(std::chrono::milliseconds(120));
    }
}

// ── Demo controller ───────────────────────────────────────────────────────────

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
    }

    ~Demo() { teardown(); }

    // ── Transport ─────────────────────────────────────────────────────────────

    void connect()
    {
        teardown();

        if (m_cfg.mode == DemoMode::Amp)
            connectAmp();
        else
            connectDS100();
    }

    void disconnect()
    {
        teardown();
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            g_state.ctrlState = CtrlState::Disconnected;
            resetAmpDisplay();
        }
        g_needsRedraw = true;
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

// ── Help ──────────────────────────────────────────────────────────────────────

static void printHelp(const char* argv0)
{
    std::cout <<
"Usage:\n"
"  " << argv0 << " [host [port]] [--amp | --soundscape <N>] [options]\n"
"\n"
"  Connects to a d&b OCA device and presents a live terminal panel.\n"
"  Defaults: host=127.0.0.1  port=50014  mode=--amp  --type dy  --ch 4\n"
"\n"
"Positional arguments:\n"
"  host        Device IP address or hostname  (default: 127.0.0.1)\n"
"  port        OCP.1 TCP port                 (default: 50014)\n"
"\n"
"Mode (mutually exclusive):\n"
"  --amp                  Amplifier mode — connect to a d&b Dx, Dy, or 5D\n"
"                         amplifier via AmpController.\n"
"  --soundscape <N>       Soundscape mode — connect to a d&b Soundscape signal\n"
"                         engine (DS100 / DS110 / DS100M / vCore) via\n"
"                         SoundscapeController and monitor/control sound object N.\n"
"                         The exact device model is identified automatically\n"
"                         from the device GUID on every connect.\n"
"\n"
"Amp-mode options (ignored in Soundscape mode):\n"
"  --type dx|dy|5d        Amplifier family  (default: dy)\n"
"  --ch <n>               Number of channels to display, 1–4  (default: 4)\n"
"\n"
"Interactive commands — Amp mode:\n"
"  a <ip>           Set host address and reconnect\n"
"  p <port>         Set port and reconnect\n"
"  c                Connect (or reconnect)\n"
"  d                Disconnect\n"
"  1                Power ON\n"
"  0                Power OFF\n"
"  g <ch> <dB>      Set channel gain   (ch: 1–4,  dB: -57.5 to +6.0)\n"
"  m <ch> <1|0>     Mute / unmute channel  (1 = muted,  0 = unmuted)\n"
"  q / quit         Exit\n"
"\n"
"Interactive commands — Soundscape mode:\n"
"  a <ip>           Set host address and reconnect\n"
"  p <port>         Set port and reconnect\n"
"  c                Connect (or reconnect)\n"
"  d                Disconnect\n"
"  x <meters>       Set sound-object absolute position X\n"
"  y <meters>       Set sound-object absolute position Y\n"
"  z <meters>       Set sound-object absolute position Z\n"
"  sp <0-1>         Set spread factor\n"
"  dm <0|1|2>       Set delay mode  (0 = off,  1 = compensate,  2 = reflect)\n"
"  ig <dB>          Set matrix input gain  (-120.0 to +24.0 dB)\n"
"  mm <1|0>         Mute / unmute matrix input  (1 = muted,  0 = unmuted)\n"
"  es <dB>          Set En-Space send gain  (-120.0 to +24.0 dB)\n"
"  q / quit         Exit\n"
"\n"
"Examples:\n"
"  " << argv0 << " 192.168.1.100 50014 --amp --type dy --ch 4\n"
"  " << argv0 << " 192.168.1.100 50014 --soundscape 5\n"
"  " << argv0 << " 192.168.1.100        --soundscape 1  (port defaults to 50014)\n"
"\n"
"Notes:\n"
"  The controller reconnects automatically on connection loss and re-subscribes\n"
"  to all parameters on the next successful connect.  No manual subscribe step\n"
"  is required.\n"
<< std::flush;
}

// ── Argument parsing ──────────────────────────────────────────────────────────

static Demo::Config parseArgs(int argc, char** argv)
{
    Demo::Config cfg;
    for (int i = 1; i < argc; ++i)
    {
        std::string arg = argv[i];

        if (arg == "--amp")
        {
            cfg.mode = DemoMode::Amp;
        }
        else if (arg == "--soundscape" && i + 1 < argc)
        {
            cfg.mode = DemoMode::Soundscape;
            try { cfg.soundObject = std::stoi(argv[++i]); }
            catch (...) { cfg.soundObject = 1; }
        }
        else if (arg == "--type" && i + 1 < argc)
        {
            ++i;
            std::string t = argv[i];
            if      (t == "dx") cfg.ampType = NanoOcp1::AmpController::AmpType::Dx;
            else if (t == "dy") cfg.ampType = NanoOcp1::AmpController::AmpType::Dy;
            else if (t == "5d") cfg.ampType = NanoOcp1::AmpController::AmpType::FiveD;
        }
        else if (arg == "--ch" && i + 1 < argc)
        {
            try { cfg.channelCount = static_cast<std::uint16_t>(std::stoi(argv[++i])); }
            catch (...) { cfg.channelCount = 4; }
        }
        else if (arg[0] != '-')
        {
            // positional: first is host, second is port
            if (cfg.host == "127.0.0.1" || cfg.host.empty())
            {
                // treat as host unless it's a pure number
                bool isNum = !arg.empty() && std::all_of(arg.begin(), arg.end(), ::isdigit);
                if (!isNum)
                    cfg.host = arg;
                else
                {
                    try { cfg.port = std::stoi(arg); }
                    catch (...) {}
                }
            }
            else
            {
                try { cfg.port = std::stoi(arg); }
                catch (...) {}
            }
        }
    }
    return cfg;
}

// ── main ──────────────────────────────────────────────────────────────────────

int main(int argc, char** argv)
{
    for (int i = 1; i < argc; ++i)
    {
        std::string a = argv[i];
        if (a == "-h" || a == "--help")
        {
            printHelp(argv[0]);
            return 0;
        }
    }

    setupTerminal();

    Demo::Config cfg = parseArgs(argc, argv);

    // Clear screen and reserve kPanelLines rows.
    {
        std::cout << Ansi::Clear << Ansi::Home;
        for (int i = 0; i < kPanelLines; ++i)
            std::cout << Ansi::Eol << "\n";
        std::cout << "> " << std::flush;
    }

    Demo demo(cfg);

    std::thread redrawThread(redrawLoop);

    demo.connect();

    // ── Command loop ──────────────────────────────────────────────────────────
    std::string line;
    while (std::getline(std::cin, line))
    {
        // trim whitespace
        auto first = line.find_first_not_of(" \t");
        if (first == std::string::npos)
        {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            std::cout << "> " << std::flush;
            continue;
        }
        auto last = line.find_last_not_of(" \t");
        line = line.substr(first, last - first + 1);

        if (line == "q" || line == "quit")
            break;

        std::istringstream iss(line);
        std::string cmd;
        iss >> cmd;

        if      (cmd == "c") demo.connect();
        else if (cmd == "d") demo.disconnect();
        else if (cmd == "a")
        {
            std::string addr;
            if (iss >> addr) demo.setAddress(addr);
            else pushLog("Usage: a <ip>");
        }
        else if (cmd == "p")
        {
            int p;
            if (iss >> p) demo.setPort(p);
            else pushLog("Usage: p <port>");
        }
        else if (cmd == "1") demo.cmdPower(true);
        else if (cmd == "0") demo.cmdPower(false);
        else if (cmd == "g")
        {
            std::uint16_t ch; float dB;
            if (iss >> ch >> dB) demo.cmdGain(ch, dB);
            else pushLog("Usage: g <ch> <dB>  (ch: 1-4, dB: -57.5 to +6.0)");
        }
        else if (cmd == "m")
        {
            std::uint16_t ch; int mute;
            if (iss >> ch >> mute) demo.cmdMute(ch, mute != 0);
            else pushLog("Usage: m <ch> <1|0>  (ch: 1-4)");
        }
        else if (cmd == "x")
        {
            float v;
            if (iss >> v) demo.cmdPositionX(v);
            else pushLog("Usage: x <meters>");
        }
        else if (cmd == "y")
        {
            float v;
            if (iss >> v) demo.cmdPositionY(v);
            else pushLog("Usage: y <meters>");
        }
        else if (cmd == "z")
        {
            float v;
            if (iss >> v) demo.cmdPositionZ(v);
            else pushLog("Usage: z <meters>");
        }
        else if (cmd == "sp")
        {
            float v;
            if (iss >> v) demo.cmdSpread(v);
            else pushLog("Usage: sp <0-1>");
        }
        else if (cmd == "dm")
        {
            int v;
            if (iss >> v) demo.cmdDelayMode(v);
            else pushLog("Usage: dm <0|1|2>  (0=off 1=compensate 2=reflect)");
        }
        else if (cmd == "ig")
        {
            float v;
            if (iss >> v) demo.cmdMatrixInputGain(v);
            else pushLog("Usage: ig <dB>  (-120.0 to +24.0)");
        }
        else if (cmd == "mm")
        {
            int v;
            if (iss >> v) demo.cmdMatrixInputMute(v != 0);
            else pushLog("Usage: mm <1|0>");
        }
        else if (cmd == "es")
        {
            float v;
            if (iss >> v) demo.cmdEnSpace(v);
            else pushLog("Usage: es <dB>  (-120.0 to +24.0)");
        }
        else
        {
            pushLog("Unknown command: " + line);
        }

        {
            std::lock_guard<std::mutex> lk(g_ioMutex);
            std::cout << "> " << std::flush;
        }
    }

    g_quit = true;
    redrawThread.join();

    std::cout << "\n";
    return 0;
}
