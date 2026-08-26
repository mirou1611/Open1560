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

// A small POSIX implementation of the Win32 types and file, directory and console
// calls the engine makes directly (logging, host file system enumeration,
// screenshots), for targets without <Windows.h>.
//
// Keeping the call sites unchanged keeps this port's diff against upstream small
// and reviewable; anything with no meaning on this platform (message boxes,
// console colour attributes) degrades to the nearest sensible behaviour.

#ifndef _WIN32

#    include <cstdint>
#    include <cstdio>
#    include <ctime>

using HWND = void*;
using HANDLE = void*;
using HGLOBAL = void*;
using HINSTANCE = void*;
using HMODULE = void*;
using UINT = unsigned int;
using DWORD = unsigned int;
using BOOL = int;
using WPARAM = std::uintptr_t;
using LPARAM = std::intptr_t;
using LRESULT = std::intptr_t;
using PSTR = char*;

struct GUID
{
    std::uint32_t Data1;
    std::uint16_t Data2;
    std::uint16_t Data3;
    std::uint8_t Data4[8];
};

// HANDLEs wrap a file descriptor as (fd + 1), so that fd 0 is not INVALID_HANDLE_VALUE.
#    define INVALID_HANDLE_VALUE (reinterpret_cast<void*>(-1))

#    define GENERIC_READ 0x80000000u
#    define GENERIC_WRITE 0x40000000u

#    define FILE_SHARE_READ 0x1u
#    define FILE_SHARE_WRITE 0x2u

#    define CREATE_NEW 1u
#    define CREATE_ALWAYS 2u
#    define OPEN_EXISTING 3u
#    define OPEN_ALWAYS 4u
#    define TRUNCATE_EXISTING 5u

#    define FILE_ATTRIBUTE_NORMAL 0x80u
#    define FILE_ATTRIBUTE_DIRECTORY 0x10u
#    define INVALID_FILE_ATTRIBUTES 0xFFFFFFFFu

#    define FILE_BEGIN 0u
#    define FILE_CURRENT 1u
#    define FILE_END 2u

#    define STD_INPUT_HANDLE (-10)
#    define STD_OUTPUT_HANDLE (-11)
#    define STD_ERROR_HANDLE (-12)

#    define FOREGROUND_BLUE 0x1u
#    define FOREGROUND_GREEN 0x2u
#    define FOREGROUND_RED 0x4u
#    define FOREGROUND_INTENSITY 0x8u

#    define MB_OK 0x0u
#    define MB_ICONERROR 0x10u
#    define MB_SETFOREGROUND 0x10000u
#    define MB_TOPMOST 0x40000u

#    define FORMAT_MESSAGE_FROM_SYSTEM 0x1000u
#    define LANG_SYSTEM_DEFAULT 0u

#    define MAX_PATH 260

#    ifndef TRUE
#        define TRUE 1
#        define FALSE 0
#    endif

struct WIN32_FIND_DATAA
{
    DWORD dwFileAttributes;
    DWORD nFileSizeLow;
    char cFileName[MAX_PATH];
};

struct OVERLAPPED
{
    DWORD Offset;
    DWORD OffsetHigh;
    HANDLE hEvent;
};

HANDLE CreateFileA(const char* file_name, DWORD desired_access, DWORD share_mode, void* security_attributes,
    DWORD creation_disposition, DWORD flags_and_attributes, HANDLE template_file);
BOOL CloseHandle(HANDLE handle);
BOOL ReadFile(HANDLE handle, void* buffer, DWORD size, DWORD* read_bytes, OVERLAPPED* overlapped);
BOOL WriteFile(HANDLE handle, const void* buffer, DWORD size, DWORD* written, OVERLAPPED* overlapped);
DWORD SetFilePointer(HANDLE handle, std::int32_t distance, std::int32_t* distance_high, DWORD move_method);
DWORD GetFileSize(HANDLE handle, DWORD* size_high);
BOOL SetEndOfFile(HANDLE handle);
BOOL FlushFileBuffers(HANDLE handle);
BOOL DeleteFileA(const char* file_name);
BOOL MoveFileA(const char* from, const char* to);
BOOL CreateDirectoryA(const char* path, void* security_attributes);
DWORD GetFileAttributesA(const char* file_name);
DWORD GetLastError();
DWORD FormatMessageA(
    DWORD flags, const void* source, DWORD message_id, DWORD language_id, char* buffer, std::size_t size, void* args);

BOOL SetCurrentDirectoryA(const char* path);
DWORD GetCurrentDirectoryA(DWORD length, char* buffer);

HANDLE FindFirstFileA(const char* file_name, WIN32_FIND_DATAA* find_data);
BOOL FindNextFileA(HANDLE handle, WIN32_FIND_DATAA* find_data);
BOOL FindClose(HANDLE handle);

HANDLE GetStdHandle(DWORD std_handle);
BOOL WriteConsoleA(HANDLE handle, const void* buffer, DWORD length, DWORD* written, void* reserved);
BOOL SetConsoleTextAttribute(HANDLE handle, std::uint16_t attributes);
void OutputDebugStringA(const char* text);
BOOL IsDebuggerPresent();
std::int32_t MessageBoxA(HWND owner, const char* text, const char* caption, DWORD type);

inline int localtime_s(std::tm* tm, const std::time_t* time)
{
    return localtime_r(time, tm) ? 0 : -1;
}

#endif
