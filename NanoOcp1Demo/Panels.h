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

// ── Panel renderers, canvas management, and the background redraw thread ───────
// resetCanvas(), redrawLoop(), and printPrompt() are the module's public surface
// (used from main.cpp); everything else here is only ever called internally.

#include <algorithm>
#include <chrono>
#include <functional>
#include <iomanip>
#include <iostream>
#include <mutex>
#include <sstream>
#include <thread>
#include <vector>

#include "Ansi.h"
#include "AppState.h"
#include "FocusParams.h"
#include "Format.h"
#include "Terminal.h"

// Each panel renderer emits exactly kFixedLines (or, for Soundscape-focus,
// kFocusFixedLines + logCapacity) newline-terminated rows. Must be called with
// g_ioMutex held.

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

// Renders the "Events" header + a fixed-capacity scrolling log — always the
// very last section of a panel, immediately above the input prompt. Shared by
// every mode's renderer (Amp, Soundscape overview/routing — after any
// optional trailing block such as --speakerlvls's — and Soundscape-focus,
// which passes its own dynamically-sized logCapacity instead of kLogLines).
// Emits exactly 1 + capacity rows.
static void renderEventsSection(const std::vector<std::string>& log, int capacity)
{
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };
    row(std::string(Ansi::Dim) + " Events" + Ansi::Reset);
    for (int i = 0; i < capacity; ++i)
    {
        if (i < static_cast<int>(log.size()))
            row(std::string(" ") + Ansi::Cyan + log[i] + Ansi::Reset);
        else
            row("");
    }
}

// Renders a short, centred warning in place of the normal panel when the
// current terminal is too small for the requested cmdln interface (see
// computePanelFit() in AppState.h). Emits exactly kWarningPanelLines rows.
static void renderTooSmallWarning(DemoMode mode, SoundscapeView view,
                                   int neededRows, int neededCols,
                                   int haveRows, int haveCols)
{
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // Pads `plain` to the terminal's horizontal centre; callers wrap the
    // already-centred (and thus already correctly counted) result in ANSI
    // colour afterwards, so colour codes never throw off the padding math.
    auto centered = [haveCols](const std::string& plain) -> std::string {
        const int pad = std::max(0, (haveCols - static_cast<int>(plain.size())) / 2);
        return std::string(pad, ' ') + plain;
    };

    std::string modeName;
    bool canGoBack = false; // only Routing is a runtime sub-state 'b' can actually leave
    switch (mode)
    {
    case DemoMode::Amp:             modeName = "Amplifier mode"; break;
    case DemoMode::SoundscapeFocus: modeName = "Soundscape Focus mode"; break;
    case DemoMode::Soundscape:
        if (view == SoundscapeView::Routing) { modeName = "Soundscape Routing sub-mode"; canGoBack = true; }
        else                                   modeName = "Soundscape Overview mode";
        break;
    }

    row("");
    row(std::string(Ansi::Bold) + Ansi::Red
        + centered("Terminal too small for " + modeName) + Ansi::Reset);
    {
        std::ostringstream o;
        o << "Needs at least " << neededRows << " rows x " << neededCols << " cols"
          << " \xe2\x80\x94 have " << haveRows << " x " << haveCols; // U+2014 em dash
        row(std::string(Ansi::Dim) + centered(o.str()) + Ansi::Reset);
    }
    {
        std::string hint = canGoBack ? "Enlarge the terminal, 'b' to go back, or 'q' to quit"
                                       : "Enlarge the terminal, or 'q' to quit";
        row(std::string(Ansi::Dim) + centered(hint) + Ansi::Reset);
    }
    row("");
}

static void renderAmpPanel(const AppState& st)
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

    // The command-reference rows and the Events/log section are rendered
    // separately — see renderAmpCommands() / renderEventsSection() — always
    // last, immediately above the input prompt.
}

// Amp mode's command-reference rows, kept separate from renderAmpPanel() so
// renderPanel() can place it after the data regardless of what else (if
// anything) comes between them — currently nothing for Amp mode, but kept
// symmetric with the Soundscape renderers for the same reason they split.
static void renderAmpCommands()
{
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };
    row(makeSep());
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    row(" 1 / 0     power on/off          g <ch> <dB>   set gain (ch: 1-4)");
    row(" m <ch> <1|0>   mute/unmute (ch: 1-4,  1=muted  0=unmuted)       q  quit");
}

// Renders Soundscape-overview mode's table: a handful of fixed header rows plus
// one compact row per sound object in the active --soundscape <lo>-<hi> range.
// Only the data — the command-reference rows and the trailing Events/log
// section are rendered separately (renderSoOverviewCommands() /
// renderEventsSection()) so renderPanel() can place --speakerlvls's optional
// block between the data and them, without this function needing to know
// whether that block is active.
static void renderSoOverviewPanel(const AppState& st)
{
    const auto  sep = makeSep();
    const auto& ds  = st.ds100;

    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // row 1: title
    {
        std::ostringstream o;
        o << Ansi::Bold << " NanoOcp1 Demo — Soundscape Overview, SO "
          << ds.soundObjectLo;
        if (ds.soundObjectHi != ds.soundObjectLo)
            o << "-" << ds.soundObjectHi;
        o << " (" << ds.objects.size() << " object" << (ds.objects.size() == 1 ? "" : "s") << ")"
          << Ansi::Reset;
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

    row(sep);  // row 5

    // Channel-strip grid: one column per sound object (an SO-number header row
    // plus one row per parameter), wrapped into bands of g_soColsPerBand
    // columns so wide ranges spill onto additional stacked bands instead of
    // off-screen. Every cell is formatted to a fixed kSoValueWidth-char
    // plain-text width *before* any ANSI colour is wrapped around it, so
    // colour codes never throw off std::setw()'s column counting.
    const int colsPerBand = std::max(1, g_soColsPerBand.load());
    const int total       = static_cast<int>(ds.objects.size());

    auto numCell = [](bool known, double value, int prec) -> std::string {
        std::ostringstream o;
        if (!known) { o << std::setw(kSoValueWidth) << "?"; return std::string(Ansi::Dim) + o.str() + Ansi::Reset; }
        o << std::fixed << std::setprecision(prec) << std::setw(kSoValueWidth) << value;
        return o.str();
    };
    // A full multi-cell meterBar(), not a single glyph, for legibility — full
    // -120..0 dB range, matching the device's actual metering floor.
    auto levelBarCell = [](bool known, float dbRaw) -> std::string {
        if (!known) return std::string(Ansi::Dim) + "[???????]" + Ansi::Reset;
        return meterBar(dbRaw, -120.0f, 0.0f, kSoLevelBarCells);
    };
    // The exact dB value shown underneath the bar.
    auto levelDbCell = [&numCell](bool known, float dbRaw) { return numCell(known, dbRaw, 1); };
    auto muteCell = [](bool known, bool muted) -> std::string {
        std::ostringstream o;
        o << std::setw(kSoValueWidth) << (!known ? "?" : (muted ? "M" : "-"));
        const auto s = o.str();
        if (!known) return std::string(Ansi::Dim) + s + Ansi::Reset;
        return muted ? (std::string(Ansi::Red) + s + Ansi::Reset) : (std::string(Ansi::Dim) + s + Ansi::Reset);
    };
    auto dmCell = [](bool known, int dm) -> std::string {
        std::ostringstream o;
        if (!known) { o << std::setw(kSoValueWidth) << "?"; return std::string(Ansi::Dim) + o.str() + Ansi::Reset; }
        o << std::setw(kSoValueWidth) << dm;
        return o.str();
    };
    auto label = [](const char* text) -> std::string {
        std::ostringstream o;
        o << std::left << std::setw(5) << text << std::right << " ";
        return o.str();
    };
    auto soHeaderCell = [](int so) -> std::string {
        std::ostringstream o;
        o << std::setw(kSoValueWidth) << so;
        return std::string(Ansi::Bold) + o.str() + Ansi::Reset;
    };

    for (int start = 0; start < total; start += colsPerBand)
    {
        const int end = std::min(total, start + colsPerBand);

        auto band = [&](const char* rowLabel, const std::function<std::string(const AppState::SoObjState&)>& cell) {
            std::ostringstream r;
            r << label(rowLabel);
            for (int i = start; i < end; ++i)
                r << cell(ds.objects[i]) << " ";
            row(r.str());
        };

        band("SO",   [&](const AppState::SoObjState& o) { return soHeaderCell(o.soundObject); });
        band("Lvl",  [&](const AppState::SoObjState& o) { return levelBarCell(o.levelKnown, o.levelDb); });
        band("dB",   [&](const AppState::SoObjState& o) { return levelDbCell(o.levelKnown, o.levelDb); });
        band("Gain", [&](const AppState::SoObjState& o) { return numCell(o.gainKnown, o.gainDb, 1); });
        band("Mt",   [&](const AppState::SoObjState& o) { return muteCell(o.muteKnown, o.muted); });
        band("X",    [&](const AppState::SoObjState& o) { return numCell(o.posKnown, o.posX, 2); });
        band("Y",    [&](const AppState::SoObjState& o) { return numCell(o.posKnown, o.posY, 2); });
        band("Z",    [&](const AppState::SoObjState& o) { return numCell(o.posKnown, o.posZ, 2); });
        band("Spr",  [&](const AppState::SoObjState& o) { return numCell(o.spreadKnown, o.spread, 2); });
        band("DM",   [&](const AppState::SoObjState& o) { return dmCell(o.dmKnown, o.delayMode); });
        band("ES",   [&](const AppState::SoObjState& o) { return numCell(o.esKnown, o.enspaceDb, 1); });

        if (end < total)
            row(sep);
    }

    // The command-reference rows and the Events/log section are rendered
    // separately — see renderSoOverviewCommands() / renderEventsSection() —
    // so --speakerlvls's block can be placed between the data and them.
}

// Soundscape-overview mode's command-reference rows, kept separate from
// renderSoOverviewPanel() (see its own comment for why) so renderPanel() can
// place it after the optional --speakerlvls block instead of right after the
// data table.
static void renderSoOverviewCommands()
{
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };
    row(makeSep());
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    row(" x/y/z [so] <m>  position   sp [so] <0-1>  spread   dm [so] <0|1|2>  delay");
    row(" ig [so] <dB>  input gain   mm [so] <1|0>  mute   es [so] <dB>  En-Space");
    row(" r <so> | r <lo>-<hi>  enter Soundobject-Routing sub-mode for one or more objects");
}

// Renders the Soundobject-Routing sub-mode table: one *column* per selected
// sound object (the same column/band scheme as the overview, wrapped onto
// additional stacked bands when needed) and one row per Function Group
// (always all 32, dimmed and collapsed when FunctionGroup_Mode says the group
// is unused). Only the data — the command-reference rows and the trailing
// Events/log section are rendered separately (renderSoRoutingCommands() /
// renderEventsSection()) so renderPanel() can place --speakerlvls's optional
// block between the data and them, without this function needing to know
// whether that block is active.
static void renderSoRoutingPanel(const AppState& st)
{
    const auto  sep = makeSep();
    const auto& ds  = st.ds100;

    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // row 1: title
    {
        std::ostringstream o;
        o << Ansi::Bold << " NanoOcp1 Demo — Soundscape Routing: SO " << ds.routingSoundObjectLo;
        if (ds.routingSoundObjectHi != ds.routingSoundObjectLo)
            o << "-" << ds.routingSoundObjectHi;
        o << " -> Function Groups" << Ansi::Reset;
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

    row(sep);  // row 5

    // Channel-strip grid: one column per selected sound object, one row per
    // Function Group. As with the overview grid, every cell is formatted to a
    // fixed plain-text width *before* any ANSI colour is wrapped around it.
    const int colsPerBand = std::max(1, g_routingColsPerBand.load());
    const int total       = static_cast<int>(ds.routingObjects.size());
    const int maxFg        = static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_FUNCTION_GROUPS);

    auto gutterLabel = [](const char* text) -> std::string {
        std::ostringstream o;
        o << std::left << std::setw(kRoutingLabelGutter) << text << std::right;
        return o.str();
    };
    // "<fg#> <name, truncated>", padded to exactly kRoutingLabelGutter.
    auto fgLabel = [](int fg, bool nameKnown, const std::string& name) -> std::string {
        std::ostringstream o;
        o << std::setw(2) << fg << " ";
        std::string n = nameKnown ? name : std::string("?");
        if (n.size() > 17) n = n.substr(0, 17);
        o << std::left << std::setw(17) << n << std::right;
        return o.str();
    };
    auto soHeaderCell = [](int so) -> std::string {
        std::ostringstream o;
        o << std::setw(kRoutingValueWidth) << so;
        return std::string(Ansi::Bold) + o.str() + Ansi::Reset;
    };
    // "<gainDb:6.1f> <mute-glyph>", each sub-field coloured only after its own
    // fixed-width formatting is already done.
    auto routingCell = [](bool gainKnown, float gainDb, bool muteKnown, bool muted) -> std::string {
        std::ostringstream g;
        if (!gainKnown) g << std::setw(6) << "?";
        else            g << std::fixed << std::setprecision(1) << std::setw(6) << gainDb;
        const std::string gainStr = gainKnown ? g.str() : (std::string(Ansi::Dim) + g.str() + Ansi::Reset);

        const std::string muteChar = !muteKnown ? "?" : (muted ? "M" : "-");
        const std::string muteStr  = !muteKnown ? (std::string(Ansi::Dim) + muteChar + Ansi::Reset)
                                    : muted       ? (std::string(Ansi::Red) + muteChar + Ansi::Reset)
                                                   : (std::string(Ansi::Dim) + muteChar + Ansi::Reset);
        return gainStr + " " + muteStr;
    };

    for (int start = 0; start < total; start += colsPerBand)
    {
        const int end = std::min(total, start + colsPerBand);

        // SO-number header row
        {
            std::ostringstream r;
            r << gutterLabel("SO");
            for (int i = start; i < end; ++i)
                r << " " << soHeaderCell(ds.routingObjects[i].soundObject);
            row(r.str());
        }

        // one row per Function Group
        for (int fg = 1; fg <= maxFg; ++fg)
        {
            const auto& info  = ds.fg[fg];
            const bool  inUse = info.modeKnown && info.mode != 0;

            std::ostringstream r;
            if (!inUse)
            {
                r << Ansi::Dim << fgLabel(fg, info.nameKnown, info.name);
                for (int i = start; i < end; ++i)
                {
                    std::ostringstream c;
                    c << std::setw(kRoutingValueWidth) << "-";
                    r << " " << c.str();
                }
                r << Ansi::Reset;
            }
            else
            {
                r << fgLabel(fg, info.nameKnown, info.name);
                for (int i = start; i < end; ++i)
                {
                    const auto& cell = ds.routingObjects[i].fg[fg];
                    r << " " << routingCell(cell.gainKnown, cell.gainDb, cell.muteKnown, cell.muted);
                }
            }
            row(r.str());
        }

        if (end < total)
            row(sep);
    }

    // The command-reference rows and the Events/log section are rendered
    // separately — see renderSoRoutingCommands() / renderEventsSection() —
    // so --speakerlvls's block can be placed between the data and them.
}

// Soundscape-routing sub-mode's command-reference rows — see
// renderSoOverviewCommands()'s comment for why this is kept separate from
// renderSoRoutingPanel().
static void renderSoRoutingCommands()
{
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };
    row(makeSep());
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    row(" rg <fg> [so] <dB>  routing gain   rm <fg> [so] <1|0>  routing mute   b/back overview   q quit");
}

// Renders the Soundscape-focus monitor panel: a handful of fixed rows describing
// the focused parameter plus a scrolling, timestamped log that is sized to fill
// the rest of the terminal (see redrawLoop()'s resize handling). Emits exactly
// kFocusFixedLines + logCapacity newline-terminated rows.
static void renderFocusPanel(const AppState& st, const std::vector<std::string>& log, int logCapacity)
{
    const auto  sep = makeSep();
    const auto& f   = st.focus;

    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    // row 1: title
    {
        std::ostringstream o;
        o << Ansi::Bold << " NanoOcp1 Demo — Soundscape Focus: " << f.paramName << Ansi::Reset
          << "  Addr(" << f.addr.pri;
        if (f.addr.sec != 0)
            o << ", " << f.addr.sec;
        o << ")";
        row(o.str());
    }
    row(sep);  // row 2

    // row 3: host / port / device model
    {
        std::ostringstream o;
        o << " Host       " << Ansi::Bold << st.address << Ansi::Reset
          << "   Port " << Ansi::Bold << st.port << Ansi::Reset;
        if (!st.ds100.deviceModel.empty())
            o << "   " << Ansi::Dim << st.ds100.deviceModel << Ansi::Reset;
        row(o.str());
    }

    // row 4: status
    {
        std::ostringstream o;
        renderStatusRow(o, st.ctrlState);
        row(o.str());
    }

    // row 5: current value
    {
        std::ostringstream o;
        o << " Value      ";
        if (!f.valueKnown)
            o << Ansi::Dim << "?" << Ansi::Reset;
        else
        {
            o << Ansi::Bold << formatVariant(f.lastValue) << Ansi::Reset
              << Ansi::Dim << "  (" << ocp1TypeName(f.lastValue.GetDataType()) << ")" << Ansi::Reset
              << "   updates: " << f.updateCount;
        }
        row(o.str());
    }

    row(sep);  // row 6

    // rows 7-8: command reference (row 8 is context-dependent)
    row(" a <ip>    set host     p <n>    set port    c  connect    d  disconnect");
    if (g_focusAwaitingInput.load())
    {
        std::ostringstream o;
        o << Ansi::Yellow << " Enter a new value below, or 'b' to cancel" << Ansi::Reset;
        if (f.valueKnown)
            o << Ansi::Dim << "   (expects " << ocp1TypeName(f.lastValue.GetDataType()) << ")" << Ansi::Reset;
        row(o.str());
    }
    else
    {
        row(" v          enter a new value to send                        q  quit");
    }

    row(sep);  // row 9

    // row 10: events header
    row(std::string(Ansi::Dim) + " Events" + Ansi::Reset);

    // remaining rows: scrolling, timestamped log
    for (int i = 0; i < logCapacity; ++i)
    {
        if (i < static_cast<int>(log.size()))
            row(std::string(" ") + Ansi::Cyan + log[i] + Ansi::Reset);
        else
            row("");
    }
}

// Renders the optional --speakerlvls loudspeaker level-meter block, appended
// below whichever Soundscape view (overview or routing) is currently shown.
// Only outputs with a non-zero (assigned) Positioning_SpeakerPosition are
// shown, laid out as one column per output wrapped into bands exactly like
// the overview/routing grids — just with a short *vertical* bar (several
// stacked single-character rows) instead of a horizontal one, since these
// columns are narrower. Emits exactly
// soSpeakerBlockLines(assignedCount, g_spkColsPerBand) rows.
static void renderSpeakerLevelsBlock(const AppState::SoState& ds)
{
    const auto sep = makeSep();
    auto row = [](const std::string& s) {
        std::cout << s << Ansi::Eol << "\n";
    };

    row(sep);

    std::vector<const AppState::SpeakerObjState*> assigned;
    for (const auto& s : ds.speakerObjects)
        if (s.hasSpeaker) assigned.push_back(&s);

    {
        std::ostringstream o;
        o << Ansi::Bold << " Loudspeaker Levels" << Ansi::Reset;
        if (!assigned.empty())
            o << " (" << assigned.size() << " output" << (assigned.size() == 1 ? "" : "s") << ")";
        else
            o << Ansi::Dim << "  (no outputs with an assigned position yet)" << Ansi::Reset;
        row(o.str());
    }

    const int colsPerBand = std::max(1, g_spkColsPerBand.load());
    const int total       = static_cast<int>(assigned.size());

    auto label = [](const char* text) -> std::string {
        std::ostringstream o;
        o << std::left << std::setw(kSpkLabelGutter - 1) << text << std::right << " ";
        return o.str();
    };
    auto headerCell = [](int ch) -> std::string {
        std::ostringstream o;
        o << std::setw(kSpkValueWidth) << ch;
        return std::string(Ansi::Bold) + o.str() + Ansi::Reset;
    };
    auto dbCell = [](bool known, float dB) -> std::string {
        std::ostringstream o;
        if (!known) { o << std::setw(kSpkValueWidth) << "?"; return std::string(Ansi::Dim) + o.str() + Ansi::Reset; }
        o << std::fixed << std::setprecision(1) << std::setw(kSpkValueWidth) << dB;
        return o.str();
    };
    // The bar glyph is always exactly 1 visible character; pad it to
    // kSpkValueWidth with plain (uncoloured) leading spaces so the colour
    // codes inside the glyph never factor into the width.
    auto barCellPad = [](const std::string& glyph) -> std::string {
        return std::string(kSpkValueWidth - 1, ' ') + glyph;
    };

    // Runs at least once even when `total == 0`, so the structural rows
    // (header/bar/dB) still appear — with empty column tails — before any
    // Positioning_SpeakerPosition values have arrived, matching
    // soSpeakerBlockLines()'s own std::max(1, assignedCount).
    for (int start = 0; start < std::max(1, total); start += colsPerBand)
    {
        const int end = std::min(total, start + colsPerBand);

        {
            std::ostringstream r;
            r << label("Out");
            for (int i = start; i < end; ++i)
                r << headerCell(assigned[static_cast<std::size_t>(i)]->outputChannel) << " ";
            row(r.str());
        }

        // The bar is clipped to a -30..0 dB window for sensitivity near the
        // top of range; the dB row below it still shows the true value down
        // to the device's full -120 dB floor (see dbCell()).
        std::vector<std::vector<std::string>> bars;
        for (int i = start; i < end; ++i)
        {
            const auto* s = assigned[static_cast<std::size_t>(i)];
            bars.push_back(verticalBarGlyphs(s->levelKnown, s->levelDb, -30.0f, 0.0f, kSpkBarSegments));
        }

        for (int seg = 0; seg < kSpkBarSegments; ++seg)
        {
            std::ostringstream r;
            r << label("");
            for (int i = start; i < end; ++i)
                r << barCellPad(bars[static_cast<std::size_t>(i - start)][static_cast<std::size_t>(seg)]) << " ";
            row(r.str());
        }

        {
            std::ostringstream r;
            r << label("dB");
            for (int i = start; i < end; ++i)
                r << dbCell(assigned[static_cast<std::size_t>(i)]->levelKnown,
                            assigned[static_cast<std::size_t>(i)]->levelDb) << " ";
            row(r.str());
        }

        if (end < total)
            row(sep);
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

    if (!g_sizeFits.load())
    {
        renderTooSmallWarning(state.mode, state.ds100.view,
                               g_neededRows.load(), g_neededCols.load(),
                               g_termRows.load(), g_termCols.load());
        return;
    }

    // Section order (Amp / Soundscape overview / Soundscape routing): data,
    // then any optional trailing block (--speakerlvls), then the command
    // legend, then Events/log — always the very last section, immediately
    // above the input prompt. Each renderer below only ever emits its own
    // data; composing that order is renderPanel()'s job alone, so none of
    // them need to know or care whether --speakerlvls is active.
    switch (state.mode)
    {
    case DemoMode::Amp:
        renderAmpPanel(state);
        renderAmpCommands();
        renderEventsSection(log, kLogLines);
        break;
    case DemoMode::Soundscape:
        if (state.ds100.view == SoundscapeView::Routing)
            renderSoRoutingPanel(state);
        else
            renderSoOverviewPanel(state);
        if (state.ds100.speakerLvlsEnabled)
            renderSpeakerLevelsBlock(state.ds100);
        if (state.ds100.view == SoundscapeView::Routing)
            renderSoRoutingCommands();
        else
            renderSoOverviewCommands();
        renderEventsSection(log, kLogLines);
        break;
    case DemoMode::SoundscapeFocus:
        renderFocusPanel(state, log, g_logCapacity.load());
        break;
    }
}

// Clears the screen and reserves `panelLines` rows for the panel, followed by the
// input prompt line. Used at startup and whenever Soundscape-focus mode's panel
// size changes on a terminal resize.
static void resetCanvas(int panelLines)
{
    std::lock_guard<std::mutex> lk(g_ioMutex);
    std::cout << Ansi::Clear << Ansi::Home;
    for (int i = 0; i < panelLines; ++i)
        std::cout << Ansi::Eol << "\n";
    std::cout << (g_focusAwaitingInput.load() ? "value> " : "> ") << std::flush;
}

// Prints the input prompt appropriate to the current interaction state
// ("value> " while Soundscape-focus mode is awaiting a new value, "> " otherwise).
static void printPrompt()
{
    std::lock_guard<std::mutex> lk(g_ioMutex);
    std::cout << (g_focusAwaitingInput.load() ? "value> " : "> ") << std::flush;
}

// ── Background redraw thread ──────────────────────────────────────────────────
static void redrawLoop()
{
    while (!g_quit)
    {
        DemoMode       mode;
        SoundscapeView view;
        int            overviewRangeCount, routingRangeCount;
        bool           speakerLvlsEnabled;
        int            assignedSpeakerCount;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            mode               = g_state.mode;
            view               = g_state.ds100.view;
            overviewRangeCount = g_state.ds100.soundObjectHi - g_state.ds100.soundObjectLo + 1;
            routingRangeCount  = g_state.ds100.routingSoundObjectHi - g_state.ds100.routingSoundObjectLo + 1;
            speakerLvlsEnabled = g_state.ds100.speakerLvlsEnabled;
            assignedSpeakerCount = static_cast<int>(std::count_if(
                g_state.ds100.speakerObjects.begin(), g_state.ds100.speakerObjects.end(),
                [](const AppState::SpeakerObjState& s) { return s.hasSpeaker; }));
        }

        // Every mode's row count — and, for Soundscape overview/routing, how
        // many sound-object columns fit per band — depends on the current
        // terminal size, which can change at runtime (a live resize) just as
        // easily as it can start out too small. computePanelFit() also tells
        // us whether the current size is too small to use at all, in which
        // case the panel is swapped for a short warning (see renderPanel()).
        int rows, cols;
        if (getTerminalSize(rows, cols))
        {
            g_termRows = rows;
            g_termCols = cols;
        }
        else
        {
            rows = g_termRows.load();
            cols = g_termCols.load();
        }

        const PanelFit fit = computePanelFit(mode, view, overviewRangeCount, routingRangeCount,
                                              speakerLvlsEnabled, assignedSpeakerCount, rows, cols);
        g_sizeFits   = fit.fits;
        g_neededRows = fit.neededRows;
        g_neededCols = fit.neededCols;
        if (mode == DemoMode::Soundscape)
        {
            if (view == SoundscapeView::Routing) g_routingColsPerBand = fit.colsPerBand;
            else                                  g_soColsPerBand     = fit.colsPerBand;
            if (speakerLvlsEnabled) g_spkColsPerBand = fit.spkColsPerBand;
        }

        int panelLines;
        if (!fit.fits)
        {
            panelLines = kWarningPanelLines;
        }
        else if (mode == DemoMode::SoundscapeFocus)
        {
            const int logLines = std::max(kFocusMinLogLines, rows - kFocusFixedLines - 1);
            panelLines    = kFocusFixedLines + logLines;
            g_logCapacity = logLines;
        }
        else if (mode == DemoMode::Soundscape)
        {
            panelLines = fit.neededRows - 1; // neededRows includes the prompt row; panelLines doesn't
        }
        else // Amp
        {
            panelLines = kPanelLines;
        }

        if (panelLines != g_panelLines.load())
        {
            resetCanvas(panelLines);
            g_panelLines  = panelLines;
            g_needsRedraw = true;
        }

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
