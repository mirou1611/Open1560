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

// Portable wrappers for the compiler/architecture intrinsics used by the engine.
// The x86/MSVC build keeps using the MSVC intrinsics; other targets (Android/ARM)
// use the equivalent compiler builtins.

#if defined(__i386__) || defined(__x86_64__) || defined(_M_IX86) || defined(_M_X64)
#    define ARTS_ARCH_X86 1
#endif

#if defined(__aarch64__) || defined(_M_ARM64) || defined(__arm__) || defined(_M_ARM)
#    define ARTS_ARCH_ARM 1
#endif

#if defined(_MSC_VER)
#    include <intrin.h>
#elif defined(ARTS_ARCH_X86)
#    include <x86intrin.h>
#endif

#if defined(_MSC_VER)
#    define ArReturnAddress() _ReturnAddress()
#    define ArDebugBreak() __debugbreak()
#else
#    define ArReturnAddress() __builtin_return_address(0)
#    define ArDebugBreak() __builtin_trap()
#endif

// Hint to the CPU that we are in a spin-wait loop.
inline void ArCpuRelax()
{
#if defined(_MSC_VER) && defined(ARTS_ARCH_X86)
    _mm_pause();
#elif defined(ARTS_ARCH_X86)
    __builtin_ia32_pause();
#elif defined(ARTS_ARCH_ARM)
    __asm__ __volatile__("yield" ::: "memory");
#endif
}

// Index of the highest/lowest set bit. Returns false if mask is zero.
inline bool ArBitScanReverse(unsigned long* index, unsigned long mask)
{
#if defined(_MSC_VER)
    return _BitScanReverse(index, mask) != 0;
#else
    if (mask == 0)
        return false;

    *index = 31 - static_cast<unsigned long>(__builtin_clz(static_cast<unsigned int>(mask)));

    return true;
#endif
}

inline bool ArBitScanForward(unsigned long* index, unsigned long mask)
{
#if defined(_MSC_VER)
    return _BitScanForward(index, mask) != 0;
#else
    if (mask == 0)
        return false;

    *index = static_cast<unsigned long>(__builtin_ctz(static_cast<unsigned int>(mask)));

    return true;
#endif
}
