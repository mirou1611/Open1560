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

define_dummy_symbol(mmdyna_isect);

#include "isect.h"

mmIntersection::mmIntersection()
{
    Reset();
}

void mmIntersection::InitSegment(
    const Vector3& from, const Vector3& to, mmBoundTemplate* bound, i32 type, i32 arg5)
{
    Type = type;
    field_4 = arg5;
    BoundTemplate = bound;

    field_6C = 0.0f;
    field_70 = 0;
    field_74 = 0.0f;
    field_7C = 0.0f;

    // Min and Max are the two ends of the segment rather than a sorted box - callers pass
    // them in whichever order the ray runs, and nothing here reorders them.
    Min = from;
    Max = to;
    Size = to - from;
    MagnitudeSqr = Size.Mag2();

    LocalMin = Min;
    LocalMax = Max;
}
