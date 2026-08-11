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
 * Soundscape overview mode (--soundscape <N>):
 *   Connects to a d&b Soundscape signal engine (DS100 / DS110 / DS100M / vCore)
 *   using SoundscapeController.  Monitors and controls sound object N: input gain,
 *   position (XYZ), spread, delay mode, En-Space send gain.  Also shows the input
 *   level meter for that sound object.  The exact device model is identified
 *   automatically via GUID.
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
 *   parameters.
 *
 * Usage:
 *   NanoOcp1Demo [host [port]] [--amp | --soundscape <N> [--param <name> [--addr2 <n>]]]
 *                [--type dx|dy|5d] [--ch <n>]
 *   NanoOcp1Demo -h | --help
 *   NanoOcp1Demo --soundscape --list-params
 *   Defaults: 127.0.0.1  50014  --amp  --type dy  --ch 4
 *
 * Source layout:
 *   Terminal.h      Platform terminal setup / size query (setupTerminal, getTerminalSize)
 *   Ansi.h          ANSI escape-sequence constants
 *   AppState.h      Shared UI state (AppState, DemoMode, globals) + logging helpers
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

// ── Help ──────────────────────────────────────────────────────────────────────

static void printHelp(const char* argv0)
{
    std::cout <<
"Usage:\n"
"  " << argv0 << " [host [port]] [--amp | --soundscape <N> [--param <name> [--addr2 <n>]]]\n"
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
"  --soundscape <N>       Soundscape overview mode — connect to a d&b Soundscape\n"
"                         signal engine (DS100 / DS110 / DS100M / vCore) via\n"
"                         SoundscapeController and monitor/control sound object N.\n"
"                         The exact device model is identified automatically\n"
"                         from the device GUID on every connect.\n"
"  --soundscape <N> --param <name>\n"
"                         Soundscape focus mode — same connection, but subscribes\n"
"                         to exactly one OCP1 remote object (--param, see\n"
"                         --soundscape --list-params for valid names) addressed by\n"
"                         <N> (primary) and --addr2 (secondary, if that parameter\n"
"                         needs one), and monitors it in a full-height, timestamped,\n"
"                         scrolling log.  Use the interactive 'v' command to send a\n"
"                         new value.\n"
"  --soundscape --list-params\n"
"                         Print every focusable parameter name (for --param) and\n"
"                         exit immediately — does not connect to anything.\n"
"\n"
"Amp-mode options (ignored in Soundscape modes):\n"
"  --type dx|dy|5d        Amplifier family  (default: dy)\n"
"  --ch <n>               Number of channels to display, 1–4  (default: 4)\n"
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
            try { cfg.soundObject = std::stoi(argv[++i]); }
            catch (...) { cfg.soundObject = 1; }
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

    // Reserve the on-screen panel canvas. Amp / Soundscape-overview modes use a
    // constant size; Soundscape-focus mode sizes itself to the current terminal
    // height up front (redrawLoop() keeps it in sync as the terminal is resized).
    int initialPanelLines = kPanelLines;
    if (cfg.mode == DemoMode::SoundscapeFocus)
    {
        int rows = 24, cols = 80;
        getTerminalSize(rows, cols); // best-effort; keeps the 24x80 fallback on failure
        const int logLines = std::max(kFocusMinLogLines, rows - kFocusFixedLines - 1);
        initialPanelLines  = kFocusFixedLines + logLines;
        g_logCapacity      = logLines;
        g_panelLines       = initialPanelLines;
    }
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
