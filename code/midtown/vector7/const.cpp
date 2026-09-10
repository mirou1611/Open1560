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

define_dummy_symbol(vector7_const);

#include "const.h"

#include "vector3.h"

// These are the standard basis, and they were data stubs - zero filled, so YAXIS and
// ZAXIS both read as (0, 0, 0). Anything rotating about YAXIS was rotating about
// nothing at all, silently. ORIGIN was only ever right by accident.
Vector3 ORIGIN {0.0f, 0.0f, 0.0f};
Vector3 XAXIS {1.0f, 0.0f, 0.0f};
Vector3 YAXIS {0.0f, 1.0f, 0.0f};
Vector3 ZAXIS {0.0f, 0.0f, 1.0f};
