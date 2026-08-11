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

// ── Platform setup ────────────────────────────────────────────────────────────
// setupTerminal() and getTerminalSize() are the only bits of the demo that need
// to differ between Windows and POSIX.

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
  // Returns the current terminal size in character rows/cols, or false if it
  // could not be determined (e.g. stdout redirected to a file).
  static bool getTerminalSize(int& rows, int& cols)
  {
      CONSOLE_SCREEN_BUFFER_INFO info;
      HANDLE h = GetStdHandle(STD_OUTPUT_HANDLE);
      if (h == INVALID_HANDLE_VALUE || !GetConsoleScreenBufferInfo(h, &info))
          return false;
      cols = info.srWindow.Right  - info.srWindow.Left + 1;
      rows = info.srWindow.Bottom - info.srWindow.Top  + 1;
      return rows > 0 && cols > 0;
  }
#else
  #include <sys/ioctl.h>
  #include <unistd.h>
  static void setupTerminal() {}
  // Returns the current terminal size in character rows/cols, or false if it
  // could not be determined (e.g. stdout redirected to a file).
  static bool getTerminalSize(int& rows, int& cols)
  {
      struct winsize ws {};
      if (ioctl(STDOUT_FILENO, TIOCGWINSZ, &ws) != 0)
          return false;
      rows = ws.ws_row;
      cols = ws.ws_col;
      return rows > 0 && cols > 0;
  }
#endif
