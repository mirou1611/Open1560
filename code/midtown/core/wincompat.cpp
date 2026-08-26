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

define_dummy_symbol(core_wincompat);

#include "wincompat.h"

#ifndef _WIN32

#    include <cerrno>
#    include <cstring>
#    include <dirent.h>
#    include <fcntl.h>
#    include <fnmatch.h>
#    include <sys/stat.h>
#    include <unistd.h>

#    ifdef __ANDROID__
#        include <android/log.h>
#    endif

static inline int HandleToFd(HANDLE handle)
{
    return static_cast<int>(reinterpret_cast<isize>(handle)) - 1;
}

static inline HANDLE FdToHandle(int fd)
{
    return (fd >= 0) ? reinterpret_cast<HANDLE>(static_cast<isize>(fd) + 1) : INVALID_HANDLE_VALUE;
}

HANDLE CreateFileA(const char* file_name, DWORD desired_access, DWORD /*share_mode*/, void* /*security_attributes*/,
    DWORD creation_disposition, DWORD /*flags_and_attributes*/, HANDLE /*template_file*/)
{
    int flags = 0;

    if ((desired_access & GENERIC_WRITE) && (desired_access & GENERIC_READ))
        flags = O_RDWR;
    else if (desired_access & GENERIC_WRITE)
        flags = O_WRONLY;
    else
        flags = O_RDONLY;

    switch (creation_disposition)
    {
        case CREATE_NEW: flags |= O_CREAT | O_EXCL; break;
        case CREATE_ALWAYS: flags |= O_CREAT | O_TRUNC; break;
        case OPEN_EXISTING: break;
        case OPEN_ALWAYS: flags |= O_CREAT; break;
        case TRUNCATE_EXISTING: flags |= O_TRUNC; break;
    }

    return FdToHandle(open(file_name, flags, 0644));
}

BOOL CloseHandle(HANDLE handle)
{
    int fd = HandleToFd(handle);

    return (fd >= 0 && close(fd) == 0) ? TRUE : FALSE;
}

BOOL ReadFile(HANDLE handle, void* buffer, DWORD size, DWORD* read_bytes, OVERLAPPED* overlapped)
{
    int fd = HandleToFd(handle);

    isize result =
        overlapped ? pread(fd, buffer, size, static_cast<off_t>(overlapped->Offset)) : read(fd, buffer, size);

    if (read_bytes)
        *read_bytes = (result > 0) ? static_cast<DWORD>(result) : 0;

    return (result >= 0) ? TRUE : FALSE;
}

BOOL WriteFile(HANDLE handle, const void* buffer, DWORD size, DWORD* written, OVERLAPPED* /*overlapped*/)
{
    isize result = write(HandleToFd(handle), buffer, size);

    if (written)
        *written = (result > 0) ? static_cast<DWORD>(result) : 0;

    return (result >= 0) ? TRUE : FALSE;
}

DWORD SetFilePointer(HANDLE handle, std::int32_t distance, std::int32_t* /*distance_high*/, DWORD move_method)
{
    int whence = SEEK_SET;

    switch (move_method)
    {
        case FILE_BEGIN: whence = SEEK_SET; break;
        case FILE_CURRENT: whence = SEEK_CUR; break;
        case FILE_END: whence = SEEK_END; break;
    }

    return static_cast<DWORD>(lseek(HandleToFd(handle), distance, whence));
}

DWORD GetFileSize(HANDLE handle, DWORD* size_high)
{
    struct stat file_stat;

    if (size_high)
        *size_high = 0;

    return (fstat(HandleToFd(handle), &file_stat) == 0) ? static_cast<DWORD>(file_stat.st_size) : 0;
}

BOOL FlushFileBuffers(HANDLE handle)
{
    return (fsync(HandleToFd(handle)) == 0) ? TRUE : FALSE;
}

BOOL DeleteFileA(const char* file_name)
{
    return (unlink(file_name) == 0) ? TRUE : FALSE;
}

DWORD GetFileAttributesA(const char* file_name)
{
    struct stat file_stat;

    if (stat(file_name, &file_stat) != 0)
        return INVALID_FILE_ATTRIBUTES;

    return S_ISDIR(file_stat.st_mode) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
}

BOOL SetEndOfFile(HANDLE handle)
{
    int fd = HandleToFd(handle);

    return (ftruncate(fd, lseek(fd, 0, SEEK_CUR)) == 0) ? TRUE : FALSE;
}

BOOL MoveFileA(const char* from, const char* to)
{
    return (rename(from, to) == 0) ? TRUE : FALSE;
}

BOOL CreateDirectoryA(const char* path, void* /*security_attributes*/)
{
    return (mkdir(path, 0755) == 0) ? TRUE : FALSE;
}

DWORD FormatMessageA(DWORD /*flags*/, const void* /*source*/, DWORD message_id, DWORD /*language_id*/, char* buffer,
    usize size, void* /*args*/)
{
    std::snprintf(buffer, size, "%s", std::strerror(static_cast<int>(message_id)));

    return static_cast<DWORD>(std::strlen(buffer));
}

DWORD GetLastError()
{
    return static_cast<DWORD>(errno);
}

BOOL SetCurrentDirectoryA(const char* path)
{
    return (chdir(path) == 0) ? TRUE : FALSE;
}

DWORD GetCurrentDirectoryA(DWORD length, char* buffer)
{
    if (getcwd(buffer, length) == nullptr)
        return 0;

    return static_cast<DWORD>(std::strlen(buffer));
}

// FindFirstFileA takes a path whose final component is a wildcard. The directory is
// opened up front and entries are matched with fnmatch as they are requested.
struct ArFindHandle
{
    DIR* Dir {};
    char Pattern[MAX_PATH] {};
    char Directory[MAX_PATH] {};
};

static bool FindNextEntry(ArFindHandle* find, WIN32_FIND_DATAA* find_data)
{
    while (dirent* entry = readdir(find->Dir))
    {
        if (fnmatch(find->Pattern, entry->d_name, FNM_CASEFOLD) != 0)
            continue;

        char path[MAX_PATH * 2];
        std::snprintf(path, sizeof(path), "%s/%s", find->Directory, entry->d_name);

        struct stat file_stat;
        bool valid = stat(path, &file_stat) == 0;

        find_data->dwFileAttributes =
            (valid && S_ISDIR(file_stat.st_mode)) ? FILE_ATTRIBUTE_DIRECTORY : FILE_ATTRIBUTE_NORMAL;
        find_data->nFileSizeLow = valid ? static_cast<DWORD>(file_stat.st_size) : 0;

        std::snprintf(find_data->cFileName, sizeof(find_data->cFileName), "%s", entry->d_name);

        return true;
    }

    return false;
}

HANDLE FindFirstFileA(const char* file_name, WIN32_FIND_DATAA* find_data)
{
    ArFindHandle* find = new ArFindHandle();

    const char* separator = std::strrchr(file_name, '/');

    if (separator)
    {
        usize dir_len = static_cast<usize>(separator - file_name);

        if (dir_len >= sizeof(find->Directory))
            dir_len = sizeof(find->Directory) - 1;

        std::memcpy(find->Directory, file_name, dir_len);
        find->Directory[dir_len] = 0;

        std::snprintf(find->Pattern, sizeof(find->Pattern), "%s", separator + 1);
    }
    else
    {
        std::snprintf(find->Directory, sizeof(find->Directory), ".");
        std::snprintf(find->Pattern, sizeof(find->Pattern), "%s", file_name);
    }

    find->Dir = opendir(find->Directory);

    if (find->Dir == nullptr || !FindNextEntry(find, find_data))
    {
        if (find->Dir)
            closedir(find->Dir);

        delete find;

        return INVALID_HANDLE_VALUE;
    }

    return find;
}

BOOL FindNextFileA(HANDLE handle, WIN32_FIND_DATAA* find_data)
{
    return FindNextEntry(static_cast<ArFindHandle*>(handle), find_data) ? TRUE : FALSE;
}

BOOL FindClose(HANDLE handle)
{
    if (handle == nullptr || handle == INVALID_HANDLE_VALUE)
        return FALSE;

    ArFindHandle* find = static_cast<ArFindHandle*>(handle);

    closedir(find->Dir);
    delete find;

    return TRUE;
}

HANDLE GetStdHandle(DWORD std_handle)
{
    switch (static_cast<i32>(std_handle))
    {
        case STD_INPUT_HANDLE: return FdToHandle(0);
        case STD_OUTPUT_HANDLE: return FdToHandle(1);
        case STD_ERROR_HANDLE: return FdToHandle(2);
    }

    return INVALID_HANDLE_VALUE;
}

BOOL WriteConsoleA(HANDLE handle, const void* buffer, DWORD length, DWORD* written, void* /*reserved*/)
{
    return WriteFile(handle, buffer, length, written, nullptr);
}

// Console attributes become ANSI SGR sequences.
BOOL SetConsoleTextAttribute(HANDLE handle, std::uint16_t attributes)
{
    const char* sequence = "\033[0m";

    if ((attributes & (FOREGROUND_RED | FOREGROUND_GREEN | FOREGROUND_BLUE)) == FOREGROUND_RED)
        sequence = (attributes & FOREGROUND_INTENSITY) ? "\033[1;31m" : "\033[31m";
    else if ((attributes & (FOREGROUND_RED | FOREGROUND_GREEN)) == (FOREGROUND_RED | FOREGROUND_GREEN))
        sequence = (attributes & FOREGROUND_INTENSITY) ? "\033[1;33m" : "\033[33m";

    return WriteFile(handle, sequence, static_cast<DWORD>(std::strlen(sequence)), nullptr, nullptr);
}

void OutputDebugStringA(const char* text)
{
#    ifdef __ANDROID__
    __android_log_write(ANDROID_LOG_INFO, "Open1560", text);
#    else
    std::fputs(text, stderr);
#    endif
}

BOOL IsDebuggerPresent()
{
    return FALSE;
}

std::int32_t MessageBoxA(HWND /*owner*/, const char* text, const char* caption, DWORD /*type*/)
{
#    ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_ERROR, "Open1560", "%s: %s", caption, text);
#    else
    std::fprintf(stderr, "%s: %s\n", caption, text);
#    endif

    return 0;
}

#endif
