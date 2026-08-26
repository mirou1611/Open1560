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
// tally at exit is the priority list for what to reimplement next.

#include <cstdio>
#include <cstring>
#include <mutex>
#include <unordered_map>
#include <vector>

#ifdef __ANDROID__
#    include <android/log.h>
#endif

namespace
{
    std::mutex StubMutex;
    std::unordered_map<const char*, unsigned long> StubCalls;
    std::vector<const char*> StubOrder;

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

    {
        std::lock_guard<std::mutex> lock(StubMutex);

        auto [entry, inserted] = StubCalls.try_emplace(name, 0);
        ++entry->second;
        first = inserted;

        if (inserted)
            StubOrder.push_back(name);
    }

    LogStub(name, first);

    return 0;
}

// Called from the Android entry point when the game shuts down, and useful from
// a debugger at any point.
extern "C" void ArtsStubReport()
{
    std::lock_guard<std::mutex> lock(StubMutex);

#ifdef __ANDROID__
    __android_log_print(ANDROID_LOG_WARN, "Open1560", "=== %zu distinct stubs called ===", StubOrder.size());
#else
    std::fprintf(stderr, "=== %zu distinct stubs called ===\n", StubOrder.size());
#endif

    for (const char* name : StubOrder)
    {
        unsigned long count = StubCalls[name];

#ifdef __ANDROID__
        __android_log_print(ANDROID_LOG_WARN, "Open1560", "%8lu  %s", count, name);
#else
        std::fprintf(stderr, "%8lu  %s\n", count, name);
#endif
    }
}
