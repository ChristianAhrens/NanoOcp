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

// ── Shared UI state ───────────────────────────────────────────────────────────
// The panel/redraw thread and the command-input thread both touch this state, so
// every mutable global here is guarded by g_stateMutex (state) or g_ioMutex
// (terminal I/O) — see the members below for which applies.

#include <algorithm>
#include <atomic>
#include <chrono>
#include <cstdint>
#include <ctime>
#include <deque>
#include <iomanip>
#include <mutex>
#include <sstream>
#include <string>

#include "AmpController.h"
#include "SoundscapeController.h"

// ── Panel dimensions ──────────────────────────────────────────────────────────
// Amp and Soundscape-overview modes use exactly 14 fixed rows + 5 log rows = 19
// total, never resized.
// Layout (fixed rows):
//   1  title         2  sep         3  host/port
//   4  status        5  mode row 1  6-9  mode rows 2-5
//   10 sep           11-13 commands  14 Events header
static constexpr int kLogLines   = 5;
static constexpr int kFixedLines = 14;
static constexpr int kPanelLines = kFixedLines + kLogLines; // 19

// Soundscape-focus mode instead fills the whole terminal height. It has its own,
// shorter, fixed-row count; the remaining terminal height becomes the log.
//   1  title  2  sep  3  host/port  4  status  5  value
//   6  sep    7-8 commands          9  sep     10 Events header
static constexpr int kFocusFixedLines  = 10;
static constexpr int kFocusMinLogLines = 3;

// Number of rows the currently-reserved on-screen panel canvas occupies, and how
// many log entries currently fit in it. Both are constant for Amp/Soundscape-
// overview modes; Soundscape-focus mode updates them whenever the terminal is
// resized (see Panels.h's redrawLoop()).
static std::atomic<int> g_panelLines{kPanelLines};
static std::atomic<int> g_logCapacity{kLogLines};

enum class DemoMode { Amp, Soundscape, SoundscapeFocus };

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

    struct FocusState
    {
        std::string paramName;
        NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent paramId
            { NanoOcp1::SoundscapeController::RemoteObject::Invalid };
        NanoOcp1::SoundscapeController::RemObjAddr addr;
        bool               valueKnown{false};
        NanoOcp1::Variant  lastValue;
        std::uint64_t      updateCount{0};
    } focus;
};

static std::mutex              g_ioMutex;
static std::mutex              g_stateMutex;
static AppState                g_state;
static std::deque<std::string> g_log;
static std::atomic<bool>       g_needsRedraw{true};
static std::atomic<bool>       g_quit{false};
// True while the interactive loop is prompting the user for a new value to send
// in Soundscape-focus mode (set/cleared only by the main input thread; read by
// the redraw thread to choose footer text and by main() to choose the prompt).
static std::atomic<bool>       g_focusAwaitingInput{false};

// ── Logging ───────────────────────────────────────────────────────────────────

static void pushLog(const std::string& msg)
{
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        g_log.push_back(msg);
        const auto cap = static_cast<std::size_t>(std::max(1, g_logCapacity.load()));
        while (g_log.size() > cap)
            g_log.pop_front();
    }
    g_needsRedraw = true;
}

// Returns "HH:MM:SS.mmm" for the current local time — used to timestamp entries
// in Soundscape-focus mode's monitor log.
static std::string timestampNow()
{
    using namespace std::chrono;
    const auto now = system_clock::now();
    const auto t   = system_clock::to_time_t(now);
    const auto ms  = duration_cast<milliseconds>(now.time_since_epoch()) % 1000;
    std::tm tmBuf{};
#if defined(_WIN32) || defined(_WIN64)
    localtime_s(&tmBuf, &t);
#else
    localtime_r(&t, &tmBuf);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tmBuf, "%H:%M:%S") << '.' << std::setw(3) << std::setfill('0') << ms.count();
    return oss.str();
}

// Like pushLog(), but prefixes a timestamp — used for Soundscape-focus mode's
// monitor log so every value update/command is traceable in time.
static void pushFocusLog(const std::string& msg)
{
    pushLog(timestampNow() + "  " + msg);
}
