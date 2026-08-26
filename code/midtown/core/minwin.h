/*
    Open1560 - An Open Source Re-Implementation of Midtown Madness 1 Beta
    Copyright (C) 2020 Brick

    This program is free software: you can redistribute it and/or modify
    it under the terms of the GNU General Public License as published by
    the Free Software Foundation, either version 3 of the License, or
    (at your option) any later version.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
    GNU General Public License for more details.

    You should have received a copy of the GNU General Public License
    along with this program. If not, see <http://www.gnu.org/licenses/>.
*/

#pragma once

#ifdef _WIN32
#    define VC_EXTRALEAN
#    define WIN32_LEAN_AND_MEAN
#    define NOMINMAX
#    include <Windows.h>
#    undef GetClassName
#else

// Non-Windows targets (Android) do not have <Windows.h>. A handful of engine
// interfaces are declared in terms of window-message types (Dispatchable, the
// CD/mixer audio helpers). Declaring the types keeps those signatures - and the
// class layouts they imply - identical across platforms; the message pump
// itself is SDL on these targets.

#    include <cstdint>

using HWND = void*;
using HANDLE = void*;
using HINSTANCE = void*;
using HMODULE = void*;
using UINT = unsigned int;
using DWORD = unsigned int;
using BOOL = int;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;

#endif