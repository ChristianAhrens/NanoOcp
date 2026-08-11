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
