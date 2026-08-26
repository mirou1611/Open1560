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

define_dummy_symbol(stream_filestream);

#include "filestream.h"

#include "core/minwin.h"

#ifdef _WIN32

static HANDLE CopyIoHandle(i32 handle)
{
    DWORD std_handle = 0;

    switch (handle)
    {
        case 0: std_handle = STD_INPUT_HANDLE; break;
        case 1: std_handle = STD_OUTPUT_HANDLE; break;
        case 2: std_handle = STD_ERROR_HANDLE; break;

        default: Quitf("Invalid IO handle %i", handle);
    }

    HANDLE result = NULL;

    return DuplicateHandle(GetCurrentProcess(), GetStdHandle(std_handle), GetCurrentProcess(), &result, 0, FALSE,
               DUPLICATE_SAME_ACCESS)
        ? result
        : INVALID_HANDLE_VALUE;
}

FileStream::FileStream(void* buffer, isize buffer_size, FileSystem* file_system)
    : Stream(buffer, buffer_size, file_system)
    , file_handle_(INVALID_HANDLE_VALUE)
    , pager_handle_(INVALID_HANDLE_VALUE)
{}

FileStream::~FileStream()
{
    Close();
}

i32 FileStream::Close()
{
    Flush();

    i32 result = -1;

    if (file_mapping_ != nullptr)
    {
        UnmapViewOfFile(file_mapping_);
        file_mapping_ = nullptr;
    }

    if (pager_handle_ != INVALID_HANDLE_VALUE)
    {
        CloseHandle(pager_handle_);
        pager_handle_ = INVALID_HANDLE_VALUE;
    }

    if (file_handle_ != INVALID_HANDLE_VALUE)
    {
        result = CloseHandle(file_handle_) ? 1 : -1;
        file_handle_ = INVALID_HANDLE_VALUE;
    }

    return result;
}

i32 FileStream::Create(const char* path)
{
    if (file_handle_ != INVALID_HANDLE_VALUE)
        return -1;

    file_handle_ = CreateFileA(path, GENERIC_READ | GENERIC_WRITE, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL,
        CREATE_ALWAYS, FILE_ATTRIBUTE_NORMAL, NULL);

    return (file_handle_ != INVALID_HANDLE_VALUE) ? 1 : -1;
}

void* FileStream::GetMapping()
{
    return file_mapping_;
}

usize FileStream::GetPagerHandle()
{
    return reinterpret_cast<usize>(pager_handle_);
}

i32 FileStream::Open(const char* path, b32 read_only)
{
    if (file_handle_ != INVALID_HANDLE_VALUE)
        return -1;

    file_handle_ = CreateFileA(path, read_only ? GENERIC_READ : (GENERIC_READ | GENERIC_WRITE),
        FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

    if (file_handle_ == INVALID_HANDLE_VALUE)
        return -1;

    if (read_only)
    {
        pager_handle_ = CreateFileA(
            path, GENERIC_READ, FILE_SHARE_READ | FILE_SHARE_WRITE, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, NULL);

        if (pager_handle_ == INVALID_HANDLE_VALUE)
            return -1;

        if (EnableBinaryFileMapping)
        {
            // TODO: Only map .ar files?
            // TODO: Only map when requested?
            HANDLE mapping = CreateFileMappingA(pager_handle_, NULL, PAGE_READONLY, 0, 0, NULL);
            file_mapping_ = MapViewOfFile(mapping, FILE_MAP_READ | FILE_MAP_COPY, 0, 0, 0);
            CloseHandle(mapping);

            if (file_mapping_ == nullptr)
                return -1;

            flags_ |= ARTS_STREAM_SUPPORTS_MAPPING;
        }
    }

    return 1;
}

isize FileStream::RawRead(void* ptr, isize size)
{
    DWORD result = 0;

    return ReadFile(file_handle_, ptr, static_cast<DWORD>(size), &result, NULL) ? result : 0;
}

i32 FileStream::RawSeek(i32 pos)
{
    return SetFilePointer(file_handle_, pos, NULL, FILE_BEGIN);
}

i32 FileStream::RawSize()
{
    return GetFileSize(file_handle_, NULL);
}

i32 FileStream::RawTell()
{
    return SetFilePointer(file_handle_, 0, NULL, FILE_CURRENT);
}

isize FileStream::RawWrite(const void* ptr, isize size)
{
    DWORD result = 0;

    return WriteFile(file_handle_, ptr, static_cast<DWORD>(size), &result, NULL) ? result : 0;
}

i32 FileStream::Stderr()
{
    if (file_handle_ != INVALID_HANDLE_VALUE)
        return -1;

    file_handle_ = CopyIoHandle(2);

    return (file_handle_ != INVALID_HANDLE_VALUE) ? 1 : -1;
}

i32 FileStream::Stdin()
{
    if (file_handle_ != INVALID_HANDLE_VALUE)
        return -1;

    file_handle_ = CopyIoHandle(0);

    return (file_handle_ != INVALID_HANDLE_VALUE) ? 1 : -1;
}

i32 FileStream::Stdout()
{
    if (file_handle_ != INVALID_HANDLE_VALUE)
        return -1;

    file_handle_ = CopyIoHandle(1);

    return (file_handle_ != INVALID_HANDLE_VALUE) ? 1 : -1;
}

#else

#    include <fcntl.h>
#    include <sys/mman.h>
#    include <sys/stat.h>
#    include <unistd.h>

// POSIX implementation. File descriptors replace HANDLEs, and the copy-on-write
// view Windows creates with FILE_MAP_COPY becomes a private mapping.

static int CopyIoHandle(i32 handle)
{
    switch (handle)
    {
        case 0:
        case 1:
        case 2: break;

        default: Quitf("Invalid IO handle %i", handle);
    }

    return dup(handle);
}

FileStream::FileStream(void* buffer, isize buffer_size, FileSystem* file_system)
    : Stream(buffer, buffer_size, file_system)
{}

FileStream::~FileStream()
{
    Close();
}

i32 FileStream::Close()
{
    Flush();

    i32 result = -1;

    if (file_mapping_ != nullptr)
    {
        munmap(file_mapping_, mapping_size_);
        file_mapping_ = nullptr;
        mapping_size_ = 0;
    }

    if (pager_handle_ != -1)
    {
        close(pager_handle_);
        pager_handle_ = -1;
    }

    if (file_handle_ != -1)
    {
        result = (close(file_handle_) == 0) ? 1 : -1;
        file_handle_ = -1;
    }

    return result;
}

i32 FileStream::Create(const char* path)
{
    if (file_handle_ != -1)
        return -1;

    file_handle_ = open(path, O_RDWR | O_CREAT | O_TRUNC, 0644);

    return (file_handle_ != -1) ? 1 : -1;
}

void* FileStream::GetMapping()
{
    return file_mapping_;
}

usize FileStream::GetPagerHandle()
{
    return static_cast<usize>(pager_handle_);
}

i32 FileStream::Open(const char* path, b32 read_only)
{
    if (file_handle_ != -1)
        return -1;

    file_handle_ = open(path, read_only ? O_RDONLY : O_RDWR);

    if (file_handle_ == -1)
        return -1;

    if (read_only)
    {
        pager_handle_ = open(path, O_RDONLY);

        if (pager_handle_ == -1)
            return -1;

        if (EnableBinaryFileMapping)
        {
            struct stat file_stat;

            if (fstat(pager_handle_, &file_stat) != 0 || file_stat.st_size == 0)
                return -1;

            mapping_size_ = static_cast<usize>(file_stat.st_size);

            // PROT_WRITE + MAP_PRIVATE gives the copy-on-write view the engine expects
            void* mapping = mmap(nullptr, mapping_size_, PROT_READ | PROT_WRITE, MAP_PRIVATE, pager_handle_, 0);

            if (mapping == MAP_FAILED)
            {
                mapping_size_ = 0;

                return -1;
            }

            file_mapping_ = mapping;
            flags_ |= ARTS_STREAM_SUPPORTS_MAPPING;
        }
    }

    return 1;
}

isize FileStream::RawRead(void* ptr, isize size)
{
    isize result = read(file_handle_, ptr, static_cast<usize>(size));

    return (result > 0) ? result : 0;
}

i32 FileStream::RawSeek(i32 pos)
{
    return static_cast<i32>(lseek(file_handle_, pos, SEEK_SET));
}

i32 FileStream::RawSize()
{
    struct stat file_stat;

    return (fstat(file_handle_, &file_stat) == 0) ? static_cast<i32>(file_stat.st_size) : 0;
}

i32 FileStream::RawTell()
{
    return static_cast<i32>(lseek(file_handle_, 0, SEEK_CUR));
}

isize FileStream::RawWrite(const void* ptr, isize size)
{
    isize result = write(file_handle_, ptr, static_cast<usize>(size));

    return (result > 0) ? result : 0;
}

i32 FileStream::Stderr()
{
    if (file_handle_ != -1)
        return -1;

    file_handle_ = CopyIoHandle(2);

    return (file_handle_ != -1) ? 1 : -1;
}

i32 FileStream::Stdin()
{
    if (file_handle_ != -1)
        return -1;

    file_handle_ = CopyIoHandle(0);

    return (file_handle_ != -1) ? 1 : -1;
}

i32 FileStream::Stdout()
{
    if (file_handle_ != -1)
        return -1;

    file_handle_ = CopyIoHandle(1);

    return (file_handle_ != -1) ? 1 : -1;
}

#endif

i32 FileStream::GetError(char* buf, isize buf_len)
{
    DWORD error = GetLastError();
    FormatMessageA(FORMAT_MESSAGE_FROM_SYSTEM, NULL, error, LANG_SYSTEM_DEFAULT, buf, buf_len, NULL);
    return static_cast<i32>(error);
}

static mem::cmd_param PARAM_mapping {"mapping"};

hook_func(INIT_main, [] { EnableBinaryFileMapping = PARAM_mapping.get_or(false); });