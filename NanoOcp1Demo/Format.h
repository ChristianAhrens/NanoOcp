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

// ── Small text-formatting helpers ─────────────────────────────────────────────
// Bar gauges and enum-to-string helpers shared by the panel renderers (Panels.h)
// and the Demo controller's log messages (Demo.h).

#include <algorithm>
#include <cmath>
#include <cstddef>
#include <string>
#include <vector>

#include "AmpController.h"
#include "Ansi.h"
#include "AppState.h"
#include "SoundscapeController.h"

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

// Vertical, multi-*row* counterpart to meterBar() — used by the --speakerlvls
// loudspeaker block, where each channel is a narrow column rather than a wide
// horizontal row, so the bar has to grow in height instead of width. Returns
// `segments` single-character, already zone-coloured (green/yellow/red, same
// thresholds as meterBar()) cell strings ordered top-to-bottom (index 0 is the
// topmost/loudest segment) for one column; the caller places rows[i] into row
// i of the grid for every column so the columns' bars line up into one
// multi-row meter. Unknown values are returned as dim "?" placeholders.
static std::vector<std::string> verticalBarGlyphs(bool known, float val, float lo, float hi, int segments)
{
    std::vector<std::string> rows(static_cast<std::size_t>(segments));
    if (!known)
    {
        for (auto& r : rows) r = std::string(Ansi::Dim) + "?" + Ansi::Reset;
        return rows;
    }
    const float clamped = std::max(lo, std::min(hi, val));
    const float frac    = (hi > lo) ? (clamped - lo) / (hi - lo) : 0.0f;
    for (int i = 0; i < segments; ++i)
    {
        // Row i (from the top) represents the segment spanning
        // (segments-i-1)/segments .. (segments-i)/segments of the full range.
        const float rowTopFrac    = static_cast<float>(segments - i) / static_cast<float>(segments);
        const float rowBottomFrac = static_cast<float>(segments - i - 1) / static_cast<float>(segments);
        if (frac <= rowBottomFrac)
        {
            rows[static_cast<std::size_t>(i)] = "\xe2\x96\x91"; // U+2591 light — unlit
            continue;
        }
        const char* color = (rowTopFrac > 0.9f)  ? Ansi::Red
                           : (rowTopFrac > 0.75f) ? Ansi::Yellow
                                                   : Ansi::Green;
        rows[static_cast<std::size_t>(i)] = std::string(color) + "\xe2\x96\x88" + Ansi::Reset; // U+2588 full
    }
    return rows;
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
