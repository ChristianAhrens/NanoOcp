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
 * NanoOcp1Demo — terminal UI for the NanoOcp1 library, with three modes.
 *
 * Amp mode (--amp):
 *   Connects to a d&b amplifier using AmpController.  Shows per-channel gain,
 *   mute state, and protection status (ISP/GR/OVL/headroom).
 *
 * Soundscape overview mode (--soundscape <N> or --soundscape <lo>-<hi>):
 *   Connects to a d&b Soundscape signal engine (DS100 / DS110 / DS100M / vCore)
 *   using SoundscapeController.  Monitors and controls every sound object in the
 *   given range (a single number means a range of one): input gain, position
 *   (XYZ), spread, delay mode, En-Space send gain, plus a flickering input
 *   level meter (MatrixInput_LevelMeterIn) — shown as one channel-strip
 *   *column* per sound object (wrapping onto additional stacked bands when
 *   the range is wider than the terminal), with the level meter rendered as a
 *   full-width bar over the device's full -120..0 dB metering range.
 *   Per-parameter commands broadcast to the whole range by default, or can
 *   target one object within it (see --help).
 *   The exact device model is identified automatically via GUID.
 *
 * Soundobject-Routing sub-mode ('r <so>' or 'r <lo>-<hi>' from Soundscape-
 * overview mode):
 *   Switches to controlling SoundObjectRouting_Gain / SoundObjectRouting_Mute
 *   for one or more sound objects — any single object or sub-range of the
 *   active --soundscape range — against all 32 Function Groups.  Laid out the
 *   same way as the overview (one column per selected sound object, wrapping
 *   onto further bands if needed), with one row per Function Group.
 *   Subscribes to FunctionGroup_Mode (is the group in use?) and
 *   FunctionGroup_Name (display label) for all 32 groups so unused groups can
 *   be shown dimmed.  'rg'/'rm' broadcast to every selected sound object by
 *   default, or can target one within the routing range.  'b' / 'back'
 *   returns to the overview.
 *
 * Loudspeaker level-meter block (--speakerlvls, Soundscape overview/routing
 * only):
 *   Appends an extra block below the overview/routing panel showing a
 *   vertical level meter (MatrixOutput_LevelMeterPostMute, 3-segment bar)
 *   for every output channel that has an assigned loudspeaker position —
 *   i.e. whose Positioning_SpeakerPosition 6-DOF blob isn't all-zero.  The
 *   bar is clipped to a -30..0 dB window for sensitivity; the numeric dB
 *   value beneath it still shows the true value down to the full -120 dB
 *   floor.  Laid out the same way as the overview/routing grids (one column
 *   per output, wrapping onto further bands).  Counts toward the terminal-
 *   too-small check below, same as everything else.
 *
 * Soundscape focus mode (--soundscape <N> --param <name>):
 *   Connects the same way, but subscribes to exactly one OCP1 remote object
 *   (given by --param, addressed via --soundscape <N> as its primary address and
 *   optionally --addr2 <n> as its secondary address) and monitors it in a
 *   scrolling, timestamped log that fills the whole terminal height and adapts
 *   as the terminal is resized.  The interactive 'v' command lets the user type
 *   in a new value and send it to the engine.  Run with --soundscape --list-params
 *   for a single-shot enumeration of every focusable parameter name — this flag
 *   is exclusive to Soundscape mode, since it enumerates SoundscapeController
 *   parameters.  A sound-object range cannot be combined with --param, since
 *   focus mode addresses exactly one object.
 *
 * Terminal-too-small handling (all modes):
 *   Every mode computes the minimum terminal size its panel needs (fixed for
 *   Amp/focus; derived from the active range and current column/band layout
 *   for Soundscape overview/routing) and checks it against the live terminal
 *   size on every redraw tick.  If the terminal is too small — at startup, or
 *   because it was resized smaller mid-session — the panel is replaced with a
 *   short centred warning instead of an overflowing/garbled layout; 'q'/'b'
 *   keep working so the user can quit or back out without resizing.
 *
 * Usage:
 *   NanoOcp1Demo [host [port]] [--amp | --soundscape <N>[-<hi>] [--param <name> [--addr2 <n>]] [--speakerlvls]]
 *                [--type dx|dy|5d] [--ch <n>]
 *   NanoOcp1Demo -h | --help
 *   NanoOcp1Demo -v | --version
 *   NanoOcp1Demo --soundscape --list-params
 *   Defaults: 127.0.0.1  50014  --amp  --type dy  --ch 4
 *
 * Source layout:
 *   Terminal.h      Platform terminal setup / size query (setupTerminal, getTerminalSize)
 *   Ansi.h          ANSI escape-sequence constants
 *   AppState.h      Shared UI state (AppState, DemoMode, globals), logging helpers,
 *                   and the per-mode panel-size / terminal-fit calculations
 *   Format.h        Small text-formatting helpers (bars, state/type/model strings)
 *   FocusParams.h   Soundscape-focus parameter table, lookup/listing, Variant format/parse
 *   Panels.h        Panel renderers, canvas management, background redraw thread
 *   Demo.h          The Demo controller class (wraps AmpController / SoundscapeController)
 *   main.cpp        CLI help/argument parsing and the interactive command loop
 */

#include <algorithm>
#include <cctype>
#include <iostream>
#include <sstream>
#include <string>
#include <thread>

#include "AppState.h"
#include "Demo.h"
#include "FocusParams.h"
#include "Panels.h"
#include "Terminal.h"

#include "NanoOcp1Version.h"

// ── Help ──────────────────────────────────────────────────────────────────────

static void printHelp(const char* argv0)
{
    std::cout <<
"Usage:\n"
"  " << argv0 << " [host [port]] [--amp | --soundscape <N>[-<hi>] [--param <name> [--addr2 <n>]] [--speakerlvls]]\n"
"               [options]\n"
"  " << argv0 << " --soundscape --list-params\n"
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
"  --soundscape <N>\n"
"  --soundscape <lo>-<hi> Soundscape overview mode — connect to a d&b Soundscape\n"
"                         signal engine (DS100 / DS110 / DS100M / vCore) via\n"
"                         SoundscapeController and monitor/control every sound\n"
"                         object in the range (a single number is a range of\n"
"                         one), e.g. '8-32'.  Shown as one channel-strip column\n"
"                         per object, wrapping onto further bands if the range\n"
"                         doesn't fit the terminal width.  The level meter bar\n"
"                         (MatrixInput_LevelMeterIn) spans the full -120..0 dB\n"
"                         metering range.  The exact device model is identified\n"
"                         automatically from the device GUID on every connect.\n"
"  --soundscape <N> --param <name>\n"
"                         Soundscape focus mode — same connection, but subscribes\n"
"                         to exactly one OCP1 remote object (--param, see\n"
"                         --soundscape --list-params for valid names) addressed by\n"
"                         <N> (primary) and --addr2 (secondary, if that parameter\n"
"                         needs one), and monitors it in a full-height, timestamped,\n"
"                         scrolling log.  Use the interactive 'v' command to send a\n"
"                         new value.  A range (<lo>-<hi>) cannot be combined with\n"
"                         --param, since focus mode addresses exactly one object.\n"
"  --soundscape --list-params\n"
"                         Print every focusable parameter name (for --param) and\n"
"                         exit immediately — does not connect to anything.\n"
"\n"
"Amp-mode options (ignored in Soundscape modes):\n"
"  --type dx|dy|5d        Amplifier family  (default: dy)\n"
"  --ch <n>               Number of channels to display, 1–4  (default: 4)\n"
"\n"
"Soundscape overview/routing option:\n"
"  --speakerlvls          Show an extra loudspeaker level-meter block below the\n"
"                         panel: a vertical bar (MatrixOutput_LevelMeterPostMute)\n"
"                         for every output with an assigned loudspeaker position.\n"
"                         The bar is clipped to -30..0 dB; its dB readout still\n"
"                         shows the true value down to -120 dB.  Counts toward the\n"
"                         terminal-too-small check like everything else.  Ignored in\n"
"                         Amp and Soundscape-focus modes.\n"
"\n"
"Soundscape-focus option:\n"
"  --addr2 <n>            Secondary address for two-dimensional parameters\n"
"                         (e.g. MatrixNode_Gain's output channel).  Default: 0.\n"
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
"Interactive commands — Soundscape overview mode:\n"
"  Per-parameter commands below take an optional leading sound-object number\n"
"  to target just that one object within the active range; with no target they\n"
"  broadcast to every object in the range.\n"
"  a <ip>              Set host address and reconnect\n"
"  p <port>            Set port and reconnect\n"
"  c                   Connect (or reconnect)\n"
"  d                   Disconnect\n"
"  x [so] <meters>     Set sound-object absolute position X\n"
"  y [so] <meters>     Set sound-object absolute position Y\n"
"  z [so] <meters>     Set sound-object absolute position Z\n"
"  sp [so] <0-1>       Set spread factor\n"
"  dm [so] <0|1|2>     Set delay mode  (0 = off,  1 = compensate,  2 = reflect)\n"
"  ig [so] <dB>        Set matrix input gain  (-120.0 to +24.0 dB)\n"
"  mm [so] <1|0>       Mute / unmute matrix input  (1 = muted,  0 = unmuted)\n"
"  es [so] <dB>        Set En-Space send gain  (-120.0 to +24.0 dB)\n"
"  r <so>              Enter Soundobject-Routing sub-mode for sound object <so>\n"
"  r <lo>-<hi>         Enter Soundobject-Routing sub-mode for a sub-range of\n"
"                      sound objects (must lie within the active range)\n"
"  q / quit            Exit\n"
"\n"
"Interactive commands — Soundobject-Routing sub-mode (entered via 'r'):\n"
"  Controls SoundObjectRouting_Gain / SoundObjectRouting_Mute between each\n"
"  selected sound object and up to all 32 Function Groups, laid out the same\n"
"  way as the overview (one column per sound object).  Groups for which\n"
"  FunctionGroup_Mode reports 'None' are shown dimmed as not in use.  rg/rm\n"
"  below take an optional leading sound-object number to target just that one\n"
"  object within the routing range; with no target they broadcast to every\n"
"  object in it.\n"
"  a <ip>              Set host address and reconnect\n"
"  p <port>            Set port and reconnect\n"
"  c                   Connect (or reconnect)\n"
"  d                   Disconnect\n"
"  rg <fg> [so] <dB>   Set routing gain to Function Group <fg>  (-120.0 to +24.0 dB)\n"
"  rm <fg> [so] <1|0>  Mute / unmute routing to Function Group <fg>\n"
"  b / back            Return to Soundscape-overview mode\n"
"  q / quit            Exit\n"
"\n"
"Interactive commands — Soundscape focus mode:\n"
"  a <ip>           Set host address and reconnect\n"
"  p <port>         Set port and reconnect\n"
"  c                Connect (or reconnect)\n"
"  d                Disconnect\n"
"  v                Prompt for a new value to send; type it on the next line,\n"
"                   or 'b' / 'back' to cancel\n"
"  q / quit         Exit\n"
"\n"
"Examples:\n"
"  " << argv0 << " 192.168.1.100 50014 --amp --type dy --ch 4\n"
"  " << argv0 << " 192.168.1.100 50014 --soundscape 5\n"
"  " << argv0 << " 192.168.1.100        --soundscape 1  (port defaults to 50014)\n"
"  " << argv0 << " 192.168.1.100 --soundscape 8-32  (ranged overview, 25 objects)\n"
"  " << argv0 << " 192.168.1.100 --soundscape 8-32 --speakerlvls  (+ loudspeaker levels)\n"
"  " << argv0 << " 192.168.1.100 --soundscape 5 --param MatrixInput_Gain\n"
"  " << argv0 << " 192.168.1.100 --soundscape 3 --param MatrixNode_Gain --addr2 7\n"
"  " << argv0 << " --soundscape --list-params\n"
"\n"
"Notes:\n"
"  The controller reconnects automatically on connection loss and re-subscribes\n"
"  to all parameters on the next successful connect.  No manual subscribe step\n"
"  is required.\n"
<< std::flush;
}

// ── Argument parsing ──────────────────────────────────────────────────────────

// Parses a --soundscape argument of either "<N>" or "<lo>-<hi>" form. Returns
// false (leaving lo/hi unchanged) if the token isn't a valid integer or range.
static bool parseSoundObjectRange(const std::string& tok, int& lo, int& hi)
{
    const auto dash = tok.find('-');
    if (dash == std::string::npos || dash == 0)
    {
        try { lo = hi = std::stoi(tok); }
        catch (...) { return false; }
        return true;
    }
    try
    {
        lo = std::stoi(tok.substr(0, dash));
        hi = std::stoi(tok.substr(dash + 1));
    }
    catch (...) { return false; }
    return true;
}

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
            // Preserve an already-parsed --param (order-independent parsing);
            // otherwise this is plain overview mode.
            cfg.mode = (cfg.mode == DemoMode::SoundscapeFocus) ? DemoMode::SoundscapeFocus
                                                                : DemoMode::Soundscape;
            int lo = 1, hi = 1;
            if (parseSoundObjectRange(argv[++i], lo, hi))
            {
                cfg.soundObjectLo = lo;
                cfg.soundObjectHi = hi;
            }
            else
            {
                cfg.soundObjectLo = cfg.soundObjectHi = 1;
            }
        }
        else if (arg == "--param" && i + 1 < argc)
        {
            cfg.mode           = DemoMode::SoundscapeFocus;
            cfg.focusParamName = argv[++i];
            if (const auto* entry = findFocusParam(cfg.focusParamName))
                cfg.focusParamId = entry->id;
        }
        else if (arg == "--addr2" && i + 1 < argc)
        {
            try { cfg.addr2 = std::stoi(argv[++i]); }
            catch (...) { cfg.addr2 = 0; }
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
        else if (arg == "--speakerlvls")
        {
            cfg.speakerLvls = true;
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
        if (a == "-v" || a == "--version")
        {
            std::cout << "NanoOcp1Demo " << NANOOCP1_VERSION_STRING
                       << " (NanoOcp1 " << NanoOcp1::GetVersionString() << ")\n";
            return 0;
        }
        if (a == "--list-params")
        {
            // --list-params is exclusive to Soundscape mode (it enumerates
            // SoundscapeController parameters), so it must be written as
            // "--soundscape --list-params".
            if (i == 0 || std::string(argv[i - 1]) != "--soundscape")
            {
                std::cerr << "--list-params must directly follow --soundscape, e.g.\n"
                             "  " << argv[0] << " --soundscape --list-params\n";
                return 1;
            }
            printParamList();
            return 0;
        }
    }

    setupTerminal();

    Demo::Config cfg = parseArgs(argc, argv);

    if (cfg.mode == DemoMode::SoundscapeFocus && cfg.focusParamId == SORemObjIdent::Invalid)
    {
        std::cerr << "Unknown --param name"
                   << (cfg.focusParamName.empty() ? "" : (": '" + cfg.focusParamName + "'"))
                   << ". Run '" << argv[0] << " --soundscape --list-params' to see valid names.\n";
        return 1;
    }

    if (cfg.mode == DemoMode::SoundscapeFocus && cfg.soundObjectHi != cfg.soundObjectLo)
    {
        std::cerr << "A sound-object range (<lo>-<hi>) cannot be combined with --param; "
                     "Soundscape-focus mode addresses exactly one object.\n";
        return 1;
    }

    if ((cfg.mode == DemoMode::Soundscape || cfg.mode == DemoMode::SoundscapeFocus)
        && (cfg.soundObjectLo < 1
            || cfg.soundObjectHi > static_cast<int>(NanoOcp1::SoundscapeController::sc_MAX_INPUT_CHANNELS)
            || cfg.soundObjectLo > cfg.soundObjectHi))
    {
        std::cerr << "Invalid --soundscape range (expected 1-"
                   << NanoOcp1::SoundscapeController::sc_MAX_INPUT_CHANNELS << ", lo <= hi).\n";
        return 1;
    }

    // Reserve the on-screen panel canvas, sized for the current terminal.
    // computePanelFit() also tells us up front whether this terminal is too
    // small for the requested mode, in which case a short warning panel is
    // reserved instead (redrawLoop() keeps all of this in sync thereafter, as
    // the terminal is resized or the user enters/leaves the routing sub-mode).
    int rows = 24, cols = 80;
    getTerminalSize(rows, cols); // best-effort; keeps the 24x80 fallback on failure
    g_termRows = rows;
    g_termCols = cols;

    const int overviewRangeCount = cfg.soundObjectHi - cfg.soundObjectLo + 1;
    // Nothing is known about which outputs have an assigned loudspeaker
    // position yet at startup (that arrives only after connecting), so the
    // speaker block's column count starts from 0 and self-corrects within the
    // first few redraw ticks once Positioning_SpeakerPosition values arrive.
    const PanelFit fit = computePanelFit(cfg.mode, SoundscapeView::Overview, overviewRangeCount, 1,
                                          cfg.speakerLvls, 0, rows, cols);
    g_sizeFits   = fit.fits;
    g_neededRows = fit.neededRows;
    g_neededCols = fit.neededCols;
    if (cfg.mode == DemoMode::Soundscape)
    {
        g_soColsPerBand = fit.colsPerBand;
        if (cfg.speakerLvls) g_spkColsPerBand = fit.spkColsPerBand;
    }

    int initialPanelLines;
    if (!fit.fits)
    {
        initialPanelLines = kWarningPanelLines;
    }
    else if (cfg.mode == DemoMode::SoundscapeFocus)
    {
        const int logLines = std::max(kFocusMinLogLines, rows - kFocusFixedLines - 1);
        initialPanelLines  = kFocusFixedLines + logLines;
        g_logCapacity      = logLines;
    }
    else if (cfg.mode == DemoMode::Soundscape)
    {
        initialPanelLines = fit.neededRows - 1; // neededRows includes the prompt row; panelLines doesn't
    }
    else // Amp
    {
        initialPanelLines = kPanelLines;
    }
    g_panelLines = initialPanelLines;
    resetCanvas(initialPanelLines);

    Demo demo(cfg);
    const bool isFocusMode = (cfg.mode == DemoMode::SoundscapeFocus);

    std::thread redrawThread(redrawLoop);

    demo.connect();

    // ── Command loop ──────────────────────────────────────────────────────────
    std::string line;
    while (std::getline(std::cin, line))
    {
        // trim whitespace
        auto first = line.find_first_not_of(" \t");
        const std::string trimmed = (first == std::string::npos) ? std::string()
            : line.substr(first, line.find_last_not_of(" \t") - first + 1);

        // In Soundscape-focus mode, a pending 'v' command turns the *next* line
        // into either the new value to send, or a cancel ('b' / 'back').
        if (isFocusMode && g_focusAwaitingInput.load())
        {
            g_focusAwaitingInput = false;
            if (trimmed.empty() || trimmed == "b" || trimmed == "back")
                pushFocusLog("Cancelled.");
            else
                demo.cmdSetFocusValue(trimmed);
            printPrompt();
            continue;
        }

        if (trimmed.empty())
        {
            printPrompt();
            continue;
        }

        if (trimmed == "q" || trimmed == "quit")
            break;

        std::istringstream iss(trimmed);
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
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdPositionX(b, static_cast<int>(a)); else demo.cmdPositionX(a); }
            else pushLog("Usage: x [so] <meters>");
        }
        else if (cmd == "y")
        {
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdPositionY(b, static_cast<int>(a)); else demo.cmdPositionY(a); }
            else pushLog("Usage: y [so] <meters>");
        }
        else if (cmd == "z")
        {
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdPositionZ(b, static_cast<int>(a)); else demo.cmdPositionZ(a); }
            else pushLog("Usage: z [so] <meters>");
        }
        else if (cmd == "sp")
        {
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdSpread(b, static_cast<int>(a)); else demo.cmdSpread(a); }
            else pushLog("Usage: sp [so] <0-1>");
        }
        else if (cmd == "dm")
        {
            int a;
            if (iss >> a) { int b; if (iss >> b) demo.cmdDelayMode(b, a); else demo.cmdDelayMode(a); }
            else pushLog("Usage: dm [so] <0|1|2>  (0=off 1=compensate 2=reflect)");
        }
        else if (cmd == "ig")
        {
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdMatrixInputGain(b, static_cast<int>(a)); else demo.cmdMatrixInputGain(a); }
            else pushLog("Usage: ig [so] <dB>  (-120.0 to +24.0)");
        }
        else if (cmd == "mm")
        {
            int a;
            if (iss >> a) { int b; if (iss >> b) demo.cmdMatrixInputMute(b != 0, a); else demo.cmdMatrixInputMute(a != 0); }
            else pushLog("Usage: mm [so] <1|0>");
        }
        else if (cmd == "es")
        {
            float a;
            if (iss >> a) { float b; if (iss >> b) demo.cmdEnSpace(b, static_cast<int>(a)); else demo.cmdEnSpace(a); }
            else pushLog("Usage: es [so] <dB>  (-120.0 to +24.0)");
        }
        else if (cmd == "r")
        {
            std::string tok;
            int lo, hi;
            if (iss >> tok && parseSoundObjectRange(tok, lo, hi)) demo.enterRouting(lo, hi);
            else pushLog("Usage: r <so> | r <lo>-<hi>");
        }
        else if (cmd == "rg")
        {
            int fg;
            if (iss >> fg)
            {
                float a;
                if (iss >> a)
                {
                    float b;
                    if (iss >> b) demo.cmdRoutingGain(fg, b, static_cast<int>(a));
                    else           demo.cmdRoutingGain(fg, a);
                }
                else pushLog("Usage: rg <fg> [so] <dB>  (-120.0 to +24.0)");
            }
            else pushLog("Usage: rg <fg> [so] <dB>  (-120.0 to +24.0)");
        }
        else if (cmd == "rm")
        {
            int fg;
            if (iss >> fg)
            {
                int a;
                if (iss >> a)
                {
                    int b;
                    if (iss >> b) demo.cmdRoutingMute(fg, b != 0, a);
                    else           demo.cmdRoutingMute(fg, a != 0);
                }
                else pushLog("Usage: rm <fg> [so] <1|0>");
            }
            else pushLog("Usage: rm <fg> [so] <1|0>");
        }
        else if (cmd == "b" || cmd == "back")
        {
            demo.exitRouting();
        }
        else if (isFocusMode && cmd == "v")
        {
            g_focusAwaitingInput = true;
            pushFocusLog("Enter new value (or 'b'/'back' to cancel):");
        }
        else
        {
            pushLog("Unknown command: " + trimmed);
        }

        printPrompt();
    }

    g_quit = true;
    redrawThread.join();

    std::cout << "\n";
    return 0;
}
