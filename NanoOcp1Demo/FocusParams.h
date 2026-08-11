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

// ── Soundscape-focus mode: generic parameter table, formatting, parsing ────────
// Everything needed to let Soundscape-focus mode address, describe, display, and
// parse-a-new-value-for *any* RemoteObject::RemObjIdent generically, instead of
// hardcoding per-parameter logic the way the overview panel (Panels.h) does.

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <iostream>
#include <sstream>
#include <string>

#include "SoundscapeController.h"

using SORemObjIdent = NanoOcp1::SoundscapeController::RemoteObject::RemObjIdent;
using SORemObjAddr  = NanoOcp1::SoundscapeController::RemObjAddr;

struct FocusParamEntry
{
    SORemObjIdent id;
    const char*   name;      ///< Matched case-insensitively against --param.
    const char*   addrHint;  ///< See kAddrLegend below.
};

// Every RemObjIdent that has a real, directly gettable/settable OCA counterpart —
// i.e. every entry SoundscapeController::createKnownONosMap() populates. Omitted:
// HeartbeatPing/Pong/Invalid/InvalidMAX (housekeeping), Device_Clear (no OCA
// definition at all), the X/Y/XY position split-views, and Scene_Previous/Next/
// Recall (all six are SetValue-only convenience remaps with no GetValue/notify
// counterpart of their own — see SoundscapeController.h's setActiveRemoteObjects()
// docs).
static const FocusParamEntry kFocusableParams[] = {
    { SORemObjIdent::Fixed_GUID,                          "Fixed_GUID",                          "-" },
    { SORemObjIdent::Settings_DeviceName,                 "Settings_DeviceName",                 "-" },
    { SORemObjIdent::Status_StatusText,                   "Status_StatusText",                   "-" },
    { SORemObjIdent::Status_AudioNetworkSampleStatus,     "Status_AudioNetworkSampleStatus",     "-" },
    { SORemObjIdent::Error_GnrlErr,                       "Error_GnrlErr",                       "-" },
    { SORemObjIdent::Error_ErrorText,                     "Error_ErrorText",                     "-" },
    { SORemObjIdent::MatrixInput_Mute,                    "MatrixInput_Mute",                    "ch" },
    { SORemObjIdent::MatrixInput_Gain,                    "MatrixInput_Gain",                    "ch" },
    { SORemObjIdent::MatrixInput_Delay,                   "MatrixInput_Delay",                   "ch" },
    { SORemObjIdent::MatrixInput_DelayEnable,             "MatrixInput_DelayEnable",             "ch" },
    { SORemObjIdent::MatrixInput_EqEnable,                "MatrixInput_EqEnable",                "ch" },
    { SORemObjIdent::MatrixInput_Polarity,                "MatrixInput_Polarity",                "ch" },
    { SORemObjIdent::MatrixInput_ChannelName,             "MatrixInput_ChannelName",             "ch" },
    { SORemObjIdent::MatrixInput_LevelMeterPreMute,       "MatrixInput_LevelMeterPreMute",       "ch" },
    { SORemObjIdent::MatrixInput_LevelMeterPostMute,      "MatrixInput_LevelMeterPostMute",      "ch" },
    { SORemObjIdent::MatrixInput_ReverbSendGain,          "MatrixInput_ReverbSendGain",          "ch" },
    { SORemObjIdent::MatrixNode_Enable,                   "MatrixNode_Enable",                   "ch,out" },
    { SORemObjIdent::MatrixNode_Gain,                     "MatrixNode_Gain",                     "ch,out" },
    { SORemObjIdent::MatrixNode_DelayEnable,              "MatrixNode_DelayEnable",              "ch,out" },
    { SORemObjIdent::MatrixNode_Delay,                    "MatrixNode_Delay",                    "ch,out" },
    { SORemObjIdent::MatrixOutput_Mute,                   "MatrixOutput_Mute",                   "out" },
    { SORemObjIdent::MatrixOutput_Gain,                   "MatrixOutput_Gain",                   "out" },
    { SORemObjIdent::MatrixOutput_Delay,                  "MatrixOutput_Delay",                  "out" },
    { SORemObjIdent::MatrixOutput_DelayEnable,            "MatrixOutput_DelayEnable",            "out" },
    { SORemObjIdent::MatrixOutput_EqEnable,               "MatrixOutput_EqEnable",               "out" },
    { SORemObjIdent::MatrixOutput_Polarity,               "MatrixOutput_Polarity",               "out" },
    { SORemObjIdent::MatrixOutput_ChannelName,            "MatrixOutput_ChannelName",            "out" },
    { SORemObjIdent::MatrixOutput_LevelMeterPreMute,      "MatrixOutput_LevelMeterPreMute",      "out" },
    { SORemObjIdent::MatrixOutput_LevelMeterPostMute,     "MatrixOutput_LevelMeterPostMute",     "out" },
    { SORemObjIdent::Positioning_SourceSpread,            "Positioning_SourceSpread",            "ch" },
    { SORemObjIdent::Positioning_SourceDelayMode,         "Positioning_SourceDelayMode",         "ch" },
    { SORemObjIdent::Positioning_SourceEnable,            "Positioning_SourceEnable",            "ch" },
    { SORemObjIdent::Positioning_SourcePosition,          "Positioning_SourcePosition",          "ch" },
    { SORemObjIdent::CoordinateMapping_SourcePosition,    "CoordinateMapping_SourcePosition",    "ch,area" },
    { SORemObjIdent::MatrixSettings_ReverbRoomId,         "MatrixSettings_ReverbRoomId",         "-" },
    { SORemObjIdent::MatrixSettings_ReverbPredelayFactor, "MatrixSettings_ReverbPredelayFactor", "-" },
    { SORemObjIdent::MatrixSettings_ReverbRearLevel,      "MatrixSettings_ReverbRearLevel",      "-" },
    { SORemObjIdent::FunctionGroup_Name,                  "FunctionGroup_Name",                  "fg" },
    { SORemObjIdent::FunctionGroup_Delay,                 "FunctionGroup_Delay",                 "fg" },
    { SORemObjIdent::FunctionGroup_Mode,                  "FunctionGroup_Mode",                  "fg" },
    { SORemObjIdent::FunctionGroup_SpreadFactor,          "FunctionGroup_SpreadFactor",          "fg" },
    { SORemObjIdent::ReverbInput_Gain,                    "ReverbInput_Gain",                    "zone,ch" },
    { SORemObjIdent::ReverbInputProcessing_Mute,          "ReverbInputProcessing_Mute",          "zone" },
    { SORemObjIdent::ReverbInputProcessing_Gain,          "ReverbInputProcessing_Gain",          "zone" },
    { SORemObjIdent::ReverbInputProcessing_EqEnable,      "ReverbInputProcessing_EqEnable",      "zone" },
    { SORemObjIdent::ReverbInputProcessing_LevelMeter,    "ReverbInputProcessing_LevelMeter",    "zone" },
    { SORemObjIdent::Scene_SceneIndex,                    "Scene_SceneIndex",                    "-" },
    { SORemObjIdent::Scene_SceneName,                     "Scene_SceneName",                     "-" },
    { SORemObjIdent::Scene_SceneComment,                  "Scene_SceneComment",                  "-" },
    { SORemObjIdent::CoordinateMappingSettings_P1real,    "CoordinateMappingSettings_P1real",    "area" },
    { SORemObjIdent::CoordinateMappingSettings_P2real,    "CoordinateMappingSettings_P2real",    "area" },
    { SORemObjIdent::CoordinateMappingSettings_P3real,    "CoordinateMappingSettings_P3real",    "area" },
    { SORemObjIdent::CoordinateMappingSettings_P4real,    "CoordinateMappingSettings_P4real",    "area" },
    { SORemObjIdent::CoordinateMappingSettings_P1virtual, "CoordinateMappingSettings_P1virtual", "area" },
    { SORemObjIdent::CoordinateMappingSettings_P3virtual, "CoordinateMappingSettings_P3virtual", "area" },
    { SORemObjIdent::CoordinateMappingSettings_Flip,      "CoordinateMappingSettings_Flip",      "area" },
    { SORemObjIdent::CoordinateMappingSettings_Name,      "CoordinateMappingSettings_Name",      "area" },
    { SORemObjIdent::Positioning_SpeakerPosition,         "Positioning_SpeakerPosition",         "out" },
    { SORemObjIdent::Positioning_SpeakerGroup,            "Positioning_SpeakerGroup",            "out" },
    { SORemObjIdent::SoundObjectRouting_Mute,             "SoundObjectRouting_Mute",             "ch,fg" },
    { SORemObjIdent::SoundObjectRouting_Gain,             "SoundObjectRouting_Gain",             "ch,fg" },
};

static const char* kAddrLegend =
    "Addr legend (primary address = --soundscape <N>, secondary = --addr2 <n>):\n"
    "  -        no address needed\n"
    "  ch       sound object / matrix input channel (1-128)\n"
    "  out      matrix output / loudspeaker channel (1-64)\n"
    "  fg       function group (1-32)\n"
    "  zone     En-Space reverb zone (1-4)\n"
    "  area     coordinate-mapping area (1-4)\n"
    "  a,b      two-dimensional: primary is 'a' via --soundscape, secondary is 'b' via --addr2\n";

// Case-insensitive name lookup. Returns nullptr if not found.
static const FocusParamEntry* findFocusParam(const std::string& name)
{
    std::string lower = name;
    std::transform(lower.begin(), lower.end(), lower.begin(),
                    [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    for (const auto& e : kFocusableParams)
    {
        std::string candidate = e.name;
        std::transform(candidate.begin(), candidate.end(), candidate.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (candidate == lower)
            return &e;
    }
    return nullptr;
}

static void printParamList()
{
    constexpr auto count = sizeof(kFocusableParams) / sizeof(kFocusableParams[0]);
    std::cout << "Focusable Soundscape parameters (" << count << "):\n\n";
    for (const auto& e : kFocusableParams)
    {
        std::cout << "  " << std::left << std::setw(38) << e.name
                   << std::setw(8) << e.addrHint
                   << NanoOcp1::SoundscapeController::RemoteObject::GetObjectDescription(e.id)
                   << "\n";
    }
    std::cout << "\n" << kAddrLegend << std::flush;
}

// Human-readable rendering of any Variant, based on its own data type.
static std::string formatVariant(const NanoOcp1::Variant& v)
{
    bool ok = false;
    std::ostringstream oss;
    switch (v.GetDataType())
    {
    case NanoOcp1::OCP1DATATYPE_BOOLEAN:
        return v.ToBool(&ok) ? "true" : "false";
    case NanoOcp1::OCP1DATATYPE_INT8:
    case NanoOcp1::OCP1DATATYPE_INT16:
    case NanoOcp1::OCP1DATATYPE_INT32:
    case NanoOcp1::OCP1DATATYPE_INT64:
        return std::to_string(v.ToInt32(&ok));
    case NanoOcp1::OCP1DATATYPE_UINT8:
    case NanoOcp1::OCP1DATATYPE_UINT16:
    case NanoOcp1::OCP1DATATYPE_UINT32:
        return std::to_string(v.ToUInt32(&ok));
    case NanoOcp1::OCP1DATATYPE_UINT64:
        return std::to_string(v.ToUInt64(&ok));
    case NanoOcp1::OCP1DATATYPE_FLOAT32:
        oss << std::fixed << std::setprecision(3) << v.ToFloat(&ok);
        return oss.str();
    case NanoOcp1::OCP1DATATYPE_FLOAT64:
        oss << std::fixed << std::setprecision(3) << v.ToDouble(&ok);
        return oss.str();
    case NanoOcp1::OCP1DATATYPE_STRING:
        return "\"" + v.ToString(&ok) + "\"";
    case NanoOcp1::OCP1DATATYPE_BLOB:
    case NanoOcp1::OCP1DATATYPE_BLOB_FIXED_LEN:
    case NanoOcp1::OCP1DATATYPE_DB_POSITION:
    {
        const auto s = v.ToPositionString(&ok);
        return ok ? s : "<binary>";
    }
    default:
        return "?";
    }
}

static const char* ocp1TypeName(NanoOcp1::Ocp1DataType t)
{
    switch (t)
    {
    case NanoOcp1::OCP1DATATYPE_BOOLEAN:        return "bool";
    case NanoOcp1::OCP1DATATYPE_INT8:           return "int8";
    case NanoOcp1::OCP1DATATYPE_INT16:          return "int16";
    case NanoOcp1::OCP1DATATYPE_INT32:          return "int32";
    case NanoOcp1::OCP1DATATYPE_INT64:          return "int64";
    case NanoOcp1::OCP1DATATYPE_UINT8:          return "uint8";
    case NanoOcp1::OCP1DATATYPE_UINT16:         return "uint16";
    case NanoOcp1::OCP1DATATYPE_UINT32:         return "uint32";
    case NanoOcp1::OCP1DATATYPE_UINT64:         return "uint64";
    case NanoOcp1::OCP1DATATYPE_FLOAT32:        return "float32";
    case NanoOcp1::OCP1DATATYPE_FLOAT64:        return "float64";
    case NanoOcp1::OCP1DATATYPE_STRING:         return "string";
    case NanoOcp1::OCP1DATATYPE_BLOB:
    case NanoOcp1::OCP1DATATYPE_BLOB_FIXED_LEN:
    case NanoOcp1::OCP1DATATYPE_DB_POSITION:    return "position (x y z)";
    default:                          return "?";
    }
}

// Parses freeform user input into a Variant of the given (previously-observed)
// data type. Returns false with `err` set on a parse failure.
static bool parseVariantInput(const std::string& raw, NanoOcp1::Ocp1DataType hint,
                               NanoOcp1::Variant& outVar, std::string& err)
{
    const auto first = raw.find_first_not_of(" \t");
    if (first == std::string::npos) { err = "Empty value."; return false; }
    const auto last = raw.find_last_not_of(" \t");
    const std::string s = raw.substr(first, last - first + 1);

    auto parseIntLike = [&](long long& v) -> bool {
        std::string low = s;
        std::transform(low.begin(), low.end(), low.begin(),
                        [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
        if (low == "on" || low == "true")  { v = 1; return true; }
        if (low == "off" || low == "false") { v = 0; return true; }
        try { std::size_t pos; v = std::stoll(s, &pos); return pos == s.size(); }
        catch (...) { return false; }
    };

    switch (hint)
    {
    case NanoOcp1::OCP1DATATYPE_BOOLEAN:
    case NanoOcp1::OCP1DATATYPE_UINT8:
    {
        long long v;
        if (!parseIntLike(v)) { err = "Expected an integer (0-255) or on/off."; return false; }
        outVar = NanoOcp1::Variant(static_cast<std::uint8_t>(std::clamp<long long>(v, 0, 255)));
        return true;
    }
    case NanoOcp1::OCP1DATATYPE_UINT16:
    {
        long long v;
        if (!parseIntLike(v)) { err = "Expected an integer or on/off."; return false; }
        outVar = NanoOcp1::Variant(static_cast<std::uint16_t>(std::clamp<long long>(v, 0, 65535)));
        return true;
    }
    case NanoOcp1::OCP1DATATYPE_UINT32:
    {
        try { std::size_t pos; outVar = NanoOcp1::Variant(static_cast<std::uint32_t>(std::stoul(s, &pos)));
              if (pos != s.size()) { err = "Expected an integer."; return false; }
              return true; }
        catch (...) { err = "Expected an integer."; return false; }
    }
    case NanoOcp1::OCP1DATATYPE_UINT64:
    {
        try { std::size_t pos; outVar = NanoOcp1::Variant(static_cast<std::uint64_t>(std::stoull(s, &pos)));
              if (pos != s.size()) { err = "Expected an integer."; return false; }
              return true; }
        catch (...) { err = "Expected an integer."; return false; }
    }
    case NanoOcp1::OCP1DATATYPE_INT8:
    case NanoOcp1::OCP1DATATYPE_INT16:
    case NanoOcp1::OCP1DATATYPE_INT32:
    case NanoOcp1::OCP1DATATYPE_INT64:
    {
        try { std::size_t pos; outVar = NanoOcp1::Variant(static_cast<std::int32_t>(std::stoi(s, &pos)));
              if (pos != s.size()) { err = "Expected an integer."; return false; }
              return true; }
        catch (...) { err = "Expected an integer."; return false; }
    }
    case NanoOcp1::OCP1DATATYPE_FLOAT32:
    {
        try { std::size_t pos; outVar = NanoOcp1::Variant(std::stof(s, &pos));
              if (pos != s.size()) { err = "Expected a decimal number."; return false; }
              return true; }
        catch (...) { err = "Expected a decimal number."; return false; }
    }
    case NanoOcp1::OCP1DATATYPE_FLOAT64:
    {
        try { std::size_t pos; outVar = NanoOcp1::Variant(std::stod(s, &pos));
              if (pos != s.size()) { err = "Expected a decimal number."; return false; }
              return true; }
        catch (...) { err = "Expected a decimal number."; return false; }
    }
    case NanoOcp1::OCP1DATATYPE_STRING:
        outVar = NanoOcp1::Variant(s);
        return true;
    case NanoOcp1::OCP1DATATYPE_BLOB:
    case NanoOcp1::OCP1DATATYPE_BLOB_FIXED_LEN:
    case NanoOcp1::OCP1DATATYPE_DB_POSITION:
    {
        std::istringstream iss(s);
        float x, y, z;
        if (!(iss >> x >> y >> z)) { err = "Expected three numbers: X Y Z."; return false; }
        outVar = NanoOcp1::Variant(x, y, z);
        return true;
    }
    default:
        err = "This parameter's value type is not yet known — wait for the first update.";
        return false;
    }
}
