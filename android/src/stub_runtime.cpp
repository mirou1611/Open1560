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

// Runtime half of the generated link stubs (see android/gen_stubs.py).
//
// Every function that still only exists as x86 assembly in game.asm lands here
// when the game calls it. The first call to each is logged and counted; the
// tally is the priority list for what to reimplement next.
//
// This file must not allocate. The engine replaces global operator new with its
// own allocator, and stubs are called from static constructors - long before
// that allocator has a heap, and again after asSafeHeap swaps the heap out from
// under it. So the table below is fixed size and the lock is a raw pthread
// mutex. Symbol names are string literals in the generated stubs, which makes
// the pointer itself the key.

#include <cstdio>
#include <dlfcn.h>
#include <pthread.h>

#ifdef __ANDROID__
#    include <android/log.h>
#endif

namespace
{
    constexpr unsigned StubTableSize = 2048; // Power of two; ~764 stubs today

    pthread_mutex_t StubMutex = PTHREAD_MUTEX_INITIALIZER;

    const char* StubNames[StubTableSize];
    unsigned long StubCounts[StubTableSize];
    unsigned StubDistinct;

    unsigned StubSlot(const char* name)
    {
        // Fibonacci hash of the pointer, then linear probing.
        auto value = reinterpret_cast<unsigned long long>(name);
        unsigned slot = static_cast<unsigned>((value * 11400714819323198485ull) >> 53) & (StubTableSize - 1);

        for (unsigned probe = 0; probe < StubTableSize; ++probe)
        {
            unsigned index = (slot + probe) & (StubTableSize - 1);

            if (StubNames[index] == nullptr || StubNames[index] == name)
                return index;
        }

        return StubTableSize; // Full - should not happen
    }

    void LogStub(const char* name, bool first)
    {
#ifdef __ANDROID__
        __android_log_print(
            first ? ANDROID_LOG_WARN : ANDROID_LOG_VERBOSE, "Open1560", "[stub] %s", name ? name : "?");
#else
        if (first)
            std::fprintf(stderr, "[stub] %s\n", name ? name : "?");
#endif
    }
} // namespace

extern "C" long ArtsStubCalled(const char* name)
{
    bool first = false;

    pthread_mutex_lock(&StubMutex);

    if (unsigned index = StubSlot(name); index < StubTableSize)
    {
        first = StubNames[index] == nullptr;

        if (first)
        {
            StubNames[index] = name;
            ++StubDistinct;
        }

        ++StubCounts[index];
    }

    pthread_mutex_unlock(&StubMutex);

    LogStub(name, first);

    return 0;
}

// Every slot of a synthesized vtable points here (see gen_stubs.py): a virtual
// call on a class whose implementation is still in game.asm. There is no way to
// know which method was wanted, but the caller is worth logging.
extern "C" long ArtsVirtualStub()
{
    void* caller = __builtin_return_address(0);

#ifdef __ANDROID__
    Dl_info info {};

    __android_log_print(ANDROID_LOG_WARN, "Open1560", "[vstub] virtual call from %s+%p",
        (dladdr(caller, &info) && info.dli_sname) ? info.dli_sname : "?", caller);
#else
    std::fprintf(stderr, "[vstub] virtual call from %p\n", caller);
#endif

    return 0;
}

// Callable from a debugger at any point, and worth wiring to shutdown.
extern "C" void ArtsStubReport()
{
    pthread_mutex_lock(&StubMutex);

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_WARN, "Open1560", "=== %u distinct stubs called ===", StubDistinct);
#else
    std::fprintf(stderr, "=== %u distinct stubs called ===\n", StubDistinct);
#endif

    for (unsigned i = 0; i < StubTableSize; ++i)
    {
        if (StubNames[i] == nullptr)
            continue;

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_WARN, "Open1560", "%8lu  %s", StubCounts[i], StubNames[i]);
#else
        std::fprintf(stderr, "%8lu  %s\n", StubCounts[i], StubNames[i]);
#endif
    }

    pthread_mutex_unlock(&StubMutex);
}
