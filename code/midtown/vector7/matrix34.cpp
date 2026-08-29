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

define_dummy_symbol(vector7_matrix34);

#include "matrix34.h"

#include "data7/metadefine.h"

META_DEFINE("Matrix34", Matrix34)
{
    META_FIELD("a", m0);
    META_FIELD("b", m1);
    META_FIELD("c", m2);
    META_FIELD("d", m3);
}
// Row-vector convention: `v * Dot(a, b)` is `v * a * b`, so the rows of `arg1`
// are re-expressed in the basis `arg2`. Results go through locals because the
// destination is allowed to alias either source.
void Matrix34::Dot(const Matrix34& arg1, const Matrix34& arg2)
{
    Vector3 r0 = arg2.m0 * arg1.m0.x + arg2.m1 * arg1.m0.y + arg2.m2 * arg1.m0.z;
    Vector3 r1 = arg2.m0 * arg1.m1.x + arg2.m1 * arg1.m1.y + arg2.m2 * arg1.m1.z;
    Vector3 r2 = arg2.m0 * arg1.m2.x + arg2.m1 * arg1.m2.y + arg2.m2 * arg1.m2.z;
    Vector3 r3 = arg2.m0 * arg1.m3.x + arg2.m1 * arg1.m3.y + arg2.m2 * arg1.m3.z + arg2.m3;

    m0 = r0;
    m1 = r1;
    m2 = r2;
    m3 = r3;
}

// As Dot, but the translation row is left alone
void Matrix34::Dot3x3(const Matrix34& arg1, const Matrix34& arg2)
{
    Vector3 r0 = arg2.m0 * arg1.m0.x + arg2.m1 * arg1.m0.y + arg2.m2 * arg1.m0.z;
    Vector3 r1 = arg2.m0 * arg1.m1.x + arg2.m1 * arg1.m1.y + arg2.m2 * arg1.m1.z;
    Vector3 r2 = arg2.m0 * arg1.m2.x + arg2.m1 * arg1.m2.y + arg2.m2 * arg1.m2.z;

    m0 = r0;
    m1 = r1;
    m2 = r2;
}

// Inverse of a rigid transform: transpose the rotation, then undo the
// translation through it. Wrong for a matrix carrying scale or shear.
void Matrix34::FastInverse(const Matrix34& arg1)
{
    Vector3 r0 {arg1.m0.x, arg1.m1.x, arg1.m2.x};
    Vector3 r1 {arg1.m0.y, arg1.m1.y, arg1.m2.y};
    Vector3 r2 {arg1.m0.z, arg1.m1.z, arg1.m2.z};

    Vector3 r3 {
        -(arg1.m3.x * arg1.m0.x + arg1.m3.y * arg1.m0.y + arg1.m3.z * arg1.m0.z),
        -(arg1.m3.x * arg1.m1.x + arg1.m3.y * arg1.m1.y + arg1.m3.z * arg1.m1.z),
        -(arg1.m3.x * arg1.m2.x + arg1.m3.y * arg1.m2.y + arg1.m3.z * arg1.m2.z),
    };

    m0 = r0;
    m1 = r1;
    m2 = r2;
    m3 = r3;
}
