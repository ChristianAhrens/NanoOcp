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

static void renderPanel()
{
    AppState state;
    std::vector<std::string> log;
    {
        std::lock_guard<std::mutex> lk(g_stateMutex);
        state = g_state;
        log.assign(g_log.begin(), g_log.end());
    }

    switch (state.mode)
    {
    case DemoMode::Amp:             renderAmpPanel(state, log);   break;
    case DemoMode::Soundscape:      renderDS100Panel(state, log); break;
    case DemoMode::SoundscapeFocus: renderFocusPanel(state, log, g_logCapacity.load()); break;
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
        DemoMode mode;
        {
            std::lock_guard<std::mutex> lk(g_stateMutex);
            mode = g_state.mode;
        }

        // Soundscape-focus mode fills the whole terminal height and must react to
        // resizes; the other modes use a constant panel size fixed at startup.
        if (mode == DemoMode::SoundscapeFocus)
        {
            int rows, cols;
            if (getTerminalSize(rows, cols))
            {
                (void)cols; // only the row count affects this panel's layout
                const int logLines   = std::max(kFocusMinLogLines, rows - kFocusFixedLines - 1);
                const int panelLines = kFocusFixedLines + logLines;
                if (panelLines != g_panelLines.load())
                {
                    g_logCapacity = logLines;
                    resetCanvas(panelLines);
                    g_panelLines  = panelLines;
                    g_needsRedraw = true;
                }
            }
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
