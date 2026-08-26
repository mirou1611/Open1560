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

define_dummy_symbol(memory_valloc);

#include "valloc.h"

#include "core/minwin.h"

#include "allocator.h"

#ifndef _WIN32
#    include <sys/mman.h>
#    include <unistd.h>

// Win32 reserve/commit semantics on POSIX: reserve is an inaccessible private
// mapping, commit turns pages read/write, decommit drops them again.
static void* ArVirtualReserve(usize size)
{
    void* result = mmap(nullptr, size, PROT_NONE, MAP_PRIVATE | MAP_ANONYMOUS | MAP_NORESERVE, -1, 0);

    return (result != MAP_FAILED) ? result : nullptr;
}

static void* ArVirtualAlloc(usize size)
{
    void* result = mmap(nullptr, size, PROT_READ | PROT_WRITE, MAP_PRIVATE | MAP_ANONYMOUS, -1, 0);

    return (result != MAP_FAILED) ? result : nullptr;
}

static void ArVirtualCommit(void* ptr, usize size)
{
    mprotect(ptr, size, PROT_READ | PROT_WRITE);
}

static void ArVirtualDecommit(void* ptr, usize size)
{
    madvise(ptr, size, MADV_DONTNEED);
    mprotect(ptr, size, PROT_NONE);
}

static void ArVirtualRelease(void* ptr, usize size)
{
    munmap(ptr, size);
}
#endif

asSafeHeap SAFEHEAP {};

asSafeHeap::~asSafeHeap()
{
    Kill();
}

void asSafeHeap::Init(isize heap_size, i32 num_heaps)
{
#ifdef _WIN32
    SYSTEM_INFO sys_info;
    GetSystemInfo(&sys_info);

    isize granularity = sys_info.dwAllocationGranularity;
#else
    isize granularity = sysconf(_SC_PAGESIZE);
#endif

    heap_size = (heap_size + granularity - 1) & -granularity;

    heap_size_ = heap_size;
    num_heaps_ = (num_heaps > 1) ? num_heaps : 0;

    usize total_size = static_cast<usize>(heap_size * (num_heaps_ ? num_heaps_ : 1));

#ifdef _WIN32
    heap_ = static_cast<u8*>(VirtualAlloc(nullptr, total_size, num_heaps_ ? MEM_RESERVE : MEM_COMMIT,
        num_heaps_ ? PAGE_NOACCESS : PAGE_READWRITE));
#else
    heap_ = static_cast<u8*>(num_heaps_ ? ArVirtualReserve(total_size) : ArVirtualAlloc(total_size));
#endif

    Activate();
}

void asSafeHeap::Kill()
{
    if (heap_)
    {
        Deactivate();

        if (num_heaps_)
        {
#ifdef _WIN32
            VirtualFree(heap_, 0, MEM_RELEASE);
#else
            ArVirtualRelease(heap_, static_cast<usize>(heap_size_ * num_heaps_));
#endif
        }

        heap_ = nullptr;
    }
}

static mem::cmd_param PARAM_safeheap {"safeheap"};

void asSafeHeap::Restart()
{
    if (!PARAM_safeheap.get_or(true))
    {
        ALLOCATOR.DumpStats();
        return;
    }

    Deactivate();

    if (num_heaps_)
    {
        heap_index_ = (heap_index_ + 1) % num_heaps_;
    }

    Activate();
}

void asSafeHeap::Activate()
{
    current_heap_ = heap_ + (heap_size_ * heap_index_);

    if (num_heaps_)
    {
#ifdef _WIN32
        VirtualAlloc(current_heap_, heap_size_, MEM_COMMIT, PAGE_READWRITE);
#else
        ArVirtualCommit(current_heap_, static_cast<usize>(heap_size_));
#endif
    }

    ALLOCATOR.Init(current_heap_, heap_size_);
}

void asSafeHeap::Deactivate()
{
    ALLOCATOR.Kill();

    if (num_heaps_)
    {
#ifdef _WIN32
        // VirtualFree without the MEM_RELEASE flag may free memory but not address descriptors
        ARTS_MSVC_DIAGNOSTIC_IGNORED(6250);
        VirtualFree(current_heap_, heap_size_, MEM_DECOMMIT);
#else
        ArVirtualDecommit(current_heap_, static_cast<usize>(heap_size_));
#endif
    }

    current_heap_ = 0;
}
