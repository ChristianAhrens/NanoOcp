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
#include <vector>

#include "AmpController.h"
#include "SoundscapeController.h"

// ── Panel dimensions ──────────────────────────────────────────────────────────
// Amp mode uses exactly 14 fixed rows + 5 log rows = 19 total, never resized.
// Layout (fixed rows):
//   1  title         2  sep         3  host/port
//   4  status        5  mode row 1  6-9  mode rows 2-5
//   10 sep           11-13 commands  14 Events header
static constexpr int kLogLines   = 5;
static constexpr int kFixedLines = 14;
static constexpr int kPanelLines = kFixedLines + kLogLines; // 19

// Soundscape-overview mode lays out one channel-strip *column* per sound
// object (parameter rows stacked top-to-bottom within each column), grouped
// into left-to-right "bands" that wrap onto additional stacked bands when the
// active range doesn't fit the terminal width in one band. Each band is
// kSoBandRows tall (a header row of SO numbers, a multi-cell level-meter bar
// row, then one row per remaining parameter); bands are separated by a thin
// rule. See soColumnsPerBand() / soOverviewPanelLines().
//   1 title  2 sep  3 host/port  4 status  5 sep
//   (band 1: kSoBandRows rows) [sep (band 2: kSoBandRows rows)]...
//   sep  4 command rows  Events header
static constexpr int kSoHeaderLines   = 5;
static constexpr int kSoFooterLines   = 6; // sep + 4 cmd rows + Events header
static constexpr int kSoLabelGutter   = 6; // left-hand row-label column width
static constexpr int kSoValueWidth    = 9; // per-SO value field width (bar: "[" + 7 cells + "]")
static constexpr int kSoLevelBarCells = 7; // meterBar() cell count, sized to fit kSoValueWidth
static constexpr int kSoColumnStride  = kSoValueWidth + 1; // + 1-space separator
static constexpr int kSoBandRows      = 11; // SO header + Lvl(bar) + dB + 8 more parameter rows

// How many sound-object columns fit side by side in one band, given the
// current terminal width.
static inline int soColumnsPerBand(int termWidth)
{
    return std::max(1, (termWidth - kSoLabelGutter) / kSoColumnStride);
}

static inline int soOverviewPanelLines(int rangeCount, int colsPerBand)
{
    const int numBands = std::max(1, (std::max(1, rangeCount) + colsPerBand - 1) / colsPerBand);
    return kSoHeaderLines + numBands * kSoBandRows + (numBands - 1) + kSoFooterLines + kLogLines;
}

// Soundscape-routing sub-mode lays out one column per *selected* sound object
// (a sub-range of the active overview range) — the same column/band scheme as
// Soundscape-overview, just with function groups as rows instead of a fixed
// parameter list: a header row of SO numbers, then one row per function group
// (always all 32; dimmed and collapsed when FunctionGroup_Mode says the group
// is unused) showing that group's routing gain + mute per SO column, combined
// into a single cell. See soRoutingColumnsPerBand() / soRoutingPanelLines().
//   1 title  2 sep  3 host/port  4 status  5 sep
//   (band 1: kRoutingBandRows rows) [sep (band 2: kRoutingBandRows rows)]...
//   sep  2 command rows  Events header
static constexpr int kRoutingHeaderLines  = 5;
static constexpr int kRoutingFooterLines  = 4; // sep + 2 cmd rows + Events header
static constexpr int kRoutingLabelGutter  = 20; // "<fg#> <name, truncated>"
static constexpr int kRoutingValueWidth   = 8;  // "<gainDb:6.1f> <mute-glyph>"
static constexpr int kRoutingColumnStride = kRoutingValueWidth + 1; // + 1-space separator
static constexpr int kRoutingBandRows     = 1 + static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS); // SO header + 32 FG rows

// How many sound-object columns fit side by side in one routing band, given
// the current terminal width.
static inline int soRoutingColumnsPerBand(int termWidth)
{
    return std::max(1, (termWidth - kRoutingLabelGutter) / kRoutingColumnStride);
}

static inline int soRoutingPanelLines(int rangeCount, int colsPerBand)
{
    const int numBands = std::max(1, (std::max(1, rangeCount) + colsPerBand - 1) / colsPerBand);
    return kRoutingHeaderLines + numBands * kRoutingBandRows + (numBands - 1) + kRoutingFooterLines + kLogLines;
}

// Optional (--speakerlvls) loudspeaker level-meter block, appended below
// whichever Soundscape view (overview or routing) is currently shown — same
// column/band scheme again, one column per output channel that has an
// assigned loudspeaker position, with a short *vertical* meter bar (several
// stacked rows, tallest segment on top) instead of a horizontal one, since
// the block sits below a much wider panel and columns are narrow. See
// soSpeakerColumnsPerBand() / soSpeakerBlockLines().
//   sep  title
//   (band 1: kSpkBandRows rows) [sep (band 2: kSpkBandRows rows)]...
static constexpr int kSpkHeaderLines  = 2; // sep + title
static constexpr int kSpkLabelGutter  = 5;
static constexpr int kSpkValueWidth   = 6;
static constexpr int kSpkColumnStride = kSpkValueWidth + 1; // + 1-space separator
static constexpr int kSpkBarSegments  = 3; // "at least three blocks in height"
static constexpr int kSpkBandRows     = 1 + kSpkBarSegments + 1; // Out header + bar + dB

static inline int soSpeakerMinCols() { return kSpkLabelGutter + kSpkColumnStride; }

static inline int soSpeakerColumnsPerBand(int termWidth)
{
    return std::max(1, (termWidth - kSpkLabelGutter) / kSpkColumnStride);
}

static inline int soSpeakerBlockLines(int assignedCount, int colsPerBand)
{
    const int numBands = std::max(1, (std::max(1, assignedCount) + colsPerBand - 1) / colsPerBand);
    return kSpkHeaderLines + numBands * kSpkBandRows + (numBands - 1);
}

// Soundscape-focus mode instead fills the whole terminal height. It has its own,
// shorter, fixed-row count; the remaining terminal height becomes the log.
//   1  title  2  sep  3  host/port  4  status  5  value
//   6  sep    7-8 commands          9  sep     10 Events header
static constexpr int kFocusFixedLines  = 10;
static constexpr int kFocusMinLogLines = 3;

// ── Terminal-too-small handling ────────────────────────────────────────────────
// Every mode's layout has a minimum usable terminal size; if the current
// terminal is smaller, the renderer shows a short centred warning instead of
// an overflowing/garbled panel (see Panels.h's renderTooSmallWarning()).
// Amp and Soundscape-focus have fixed-width command-reference rows that
// assume a classic 80-column terminal; Soundscape overview/routing instead
// derive their minimum width from needing room for at least one column.
static constexpr int kMinTextCols       = 80;
static constexpr int kWarningPanelLines = 5; // always small enough to safely reserve

static inline int soOverviewMinCols() { return kSoLabelGutter + kSoColumnStride; }
static inline int soRoutingMinCols()  { return kRoutingLabelGutter + kRoutingColumnStride; }

// Number of rows the currently-reserved on-screen panel canvas occupies, and how
// many log entries currently fit in it. Both are constant for Amp mode;
// Soundscape-focus mode updates them whenever the terminal is resized, and
// Soundscape-overview mode updates g_panelLines (and g_soColsPerBand, below)
// whenever the terminal is resized (see Panels.h's redrawLoop()).
static std::atomic<int> g_panelLines{kPanelLines};
static std::atomic<int> g_logCapacity{kLogLines};

// How many sound-object columns the Soundscape-overview panel currently lays
// out per band — kept in sync with the terminal width by redrawLoop() and read
// by the renderer so what's drawn always matches the reserved canvas size.
static std::atomic<int> g_soColsPerBand{1};

// Same as g_soColsPerBand, but for the Soundscape-routing sub-mode's (narrower,
// since function-group names need more gutter width) column bands.
static std::atomic<int> g_routingColsPerBand{1};

// Same as g_soColsPerBand, but for the optional --speakerlvls loudspeaker
// level-meter block's column bands.
static std::atomic<int> g_spkColsPerBand{1};

// Whether the current terminal size fits the current mode's panel (see
// computePanelFit(), below) — kept in sync by redrawLoop() every tick, and
// checked by renderPanel() to decide whether to draw the normal panel or a
// "terminal too small" warning instead. g_neededRows/Cols and g_termRows/Cols
// are the numbers shown in that warning.
static std::atomic<bool> g_sizeFits{true};
static std::atomic<int>  g_neededRows{0};
static std::atomic<int>  g_neededCols{0};
static std::atomic<int>  g_termRows{24};
static std::atomic<int>  g_termCols{80};

enum class DemoMode { Amp, Soundscape, SoundscapeFocus };

// Soundscape overview mode's two runtime sub-views, layered on top of
// DemoMode::Soundscape the same way g_focusAwaitingInput layers an input
// sub-state on top of SoundscapeFocus — no separate DemoMode is needed.
enum class SoundscapeView { Overview, Routing };

// Result of checking whether a terminal of the given size can accommodate the
// requested cmdln interface for the current mode/state.
struct PanelFit
{
    bool fits{true};
    int  neededRows{0};      // includes the one row the input prompt itself needs
    int  neededCols{0};
    int  colsPerBand{1};     // meaningful for Soundscape overview/routing only
    int  spkColsPerBand{1};  // meaningful only when the --speakerlvls block is shown
};

// Determines whether a termRows x termCols terminal can fit the current
// mode's panel — plus, when `speakerLvlsEnabled`, the optional loudspeaker
// level-meter block appended below it (Soundscape overview/routing only) —
// and how many columns per band each of those implies. Used both at startup
// (main.cpp) and on every redraw tick (Panels.h's redrawLoop()) so a live
// terminal resize, or the speaker block's column count changing as
// Positioning_SpeakerPosition values arrive, is caught the same way a
// too-small starting size is.
static inline PanelFit computePanelFit(DemoMode mode, SoundscapeView view,
                                        int overviewRangeCount, int routingRangeCount,
                                        bool speakerLvlsEnabled, int assignedSpeakerCount,
                                        int termRows, int termCols)
{
    PanelFit f;
    int mainRows = 0, mainCols = 0;
    switch (mode)
    {
    case DemoMode::Amp:
        mainCols = kMinTextCols;
        mainRows = kPanelLines;
        break;

    case DemoMode::SoundscapeFocus:
        mainCols = kMinTextCols;
        mainRows = kFocusFixedLines + kFocusMinLogLines;
        break;

    case DemoMode::Soundscape:
        if (view == SoundscapeView::Routing)
        {
            mainCols      = soRoutingMinCols();
            f.colsPerBand = (termCols >= mainCols) ? soRoutingColumnsPerBand(termCols) : 1;
            mainRows      = soRoutingPanelLines(routingRangeCount, f.colsPerBand);
        }
        else
        {
            mainCols      = soOverviewMinCols();
            f.colsPerBand = (termCols >= mainCols) ? soColumnsPerBand(termCols) : 1;
            mainRows      = soOverviewPanelLines(overviewRangeCount, f.colsPerBand);
        }
        break;
    }

    int spkRows = 0, spkCols = 0;
    if (mode == DemoMode::Soundscape && speakerLvlsEnabled)
    {
        spkCols          = soSpeakerMinCols();
        f.spkColsPerBand = (termCols >= spkCols) ? soSpeakerColumnsPerBand(termCols) : 1;
        spkRows          = soSpeakerBlockLines(assignedSpeakerCount, f.spkColsPerBand);
    }

    f.neededCols = std::max(mainCols, spkCols);
    f.neededRows = mainRows + spkRows + 1; // +1 for the input prompt row
    f.fits = (termRows >= f.neededRows) && (termCols >= f.neededCols);
    return f;
}

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

    // Per-sound-object fields shown in Soundscape-overview mode's table — one
    // instance per object in the active --soundscape <lo>-<hi> range.
    struct SoObjState
    {
        int   soundObject{1};
        bool  levelKnown{false};   float levelDb{-60.0f};
        bool  gainKnown{false};    float gainDb{0.0f};
        bool  muteKnown{false};    bool  muted{false};
        bool  posKnown{false};
        float posX{0.0f};          float posY{0.0f};  float posZ{0.0f};
        bool  spreadKnown{false};  float spread{0.0f};
        bool  dmKnown{false};      int   delayMode{0};
        bool  esKnown{false};      float enspaceDb{-120.0f};
    };

    // Per-function-group fields shown in the Soundscape-routing sub-mode table —
    // always all 32 groups (independent of which/how many sound objects are
    // selected for routing), so "is this group in use" (FunctionGroup_Mode) and
    // its display name (FunctionGroup_Name) can be learned up front.
    struct FunctionGroupInfo
    {
        bool        modeKnown{false}; int         mode{0}; // 0 = None (unused)
        bool        nameKnown{false}; std::string name;
    };

    // Per-output-channel fields for the optional --speakerlvls block — always
    // every possible output channel is tracked (independent of the active
    // overview range), so "does this output have a loudspeaker assigned"
    // (a non-zero Positioning_SpeakerPosition) can be learned up front; only
    // the ones where hasSpeaker is true are actually shown.
    struct SpeakerObjState
    {
        int   outputChannel{1};
        bool  posKnown{false};    bool  hasSpeaker{false}; // Positioning_SpeakerPosition != all-zero
        bool  levelKnown{false};  float levelDb{-120.0f};  // MatrixOutput_LevelMeterPostMute
    };

    // One (sound object, function group) routing cell's gain/mute.
    struct RoutingCell
    {
        bool  gainKnown{false}; float gainDb{0.0f}; // SoundObjectRouting_Gain
        bool  muteKnown{false}; bool  muted{false};  // SoundObjectRouting_Mute
    };

    // Per-sound-object routing state shown in the Soundscape-routing sub-mode
    // table — one instance per object in the active routing sub-range.
    struct RoutingObjState
    {
        int soundObject{0};
        RoutingCell fg[NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS + 1]; // index 1..32 used
    };

    struct SoState
    {
        int         soundObjectLo{1};
        int         soundObjectHi{1};
        std::string deviceModel;
        SoundscapeView view{SoundscapeView::Overview};
        std::vector<SoObjState> objects; // size == soundObjectHi - soundObjectLo + 1

        // Routing sub-mode state (only meaningful while view == Routing). The
        // routing sub-range [routingSoundObjectLo, routingSoundObjectHi] is a
        // sub-range of [soundObjectLo, soundObjectHi] above.
        int routingSoundObjectLo{0};
        int routingSoundObjectHi{0};
        std::vector<RoutingObjState> routingObjects; // size == routingSoundObjectHi - routingSoundObjectLo + 1
        FunctionGroupInfo fg[NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS + 1]; // index 1..32 used

        // Optional (--speakerlvls) loudspeaker level-meter block state, shown
        // below whichever view (overview or routing) is active. Always sized
        // to every possible output channel; see SpeakerObjState::hasSpeaker
        // for which ones are actually displayed.
        bool speakerLvlsEnabled{false};
        std::vector<SpeakerObjState> speakerObjects; // size == sc_MAX_OUTPUT_CHANNELS when enabled
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
