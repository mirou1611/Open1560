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

define_dummy_symbol(mmcar_engine);

#include "engine.h"

mmEngine::mmEngine()
{
    GCL = 0.1f;
    HPScale = 1.0f;

    // The golden ratio and its reciprocal, computed the same way the original does
    f32 root5 = std::sqrt(5.0f);

    PullUpModifier = (root5 + 1.0f) * 0.5f;
    BreakDownModifier = (root5 - 1.0f) * 0.5f;

    Throttle = 0.0f;
    field_60 = 1.0f;
    RotationSpeed = 0.0f;
    Horsepower = 370.0f;
    Torque = 0.0f;
}
