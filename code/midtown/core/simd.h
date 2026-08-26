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

// SSE/SSE2 intrinsics for the fast paths in the pixel and vertex code.
// On ARM they are translated to NEON by sse2neon, so those paths stay vectorized
// instead of falling back to the scalar loops.

#include "arch.h"

#if defined(ARTS_ARCH_X86)
#    include <emmintrin.h>
#elif defined(ARTS_ARCH_ARM) && defined(__ARM_NEON)
#    define SSE2NEON_SUPPRESS_WARNINGS 1
#    include <sse2neon.h>
#else
#    error No SIMD backend for this architecture
#endif
