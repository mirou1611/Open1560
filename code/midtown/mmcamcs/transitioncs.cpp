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

define_dummy_symbol(mmcamcs_transitioncs);

#include "transitioncs.h"

#include "mmcar/car.h"

void TransitionCS::Reset()
{}

TransitionCS::TransitionCS()
{
    camera_.Identity();

    View = nullptr;

    ApproachOn = 1;
    AppAppOn = 0;
    AppApp = 0.0f;
    AppRotMin = 0.0f;
    AppPosMin = 0.0f;
    LookAt = 0.0f;

    field_118 = 0;
    field_11C = 0;
    field_120 = 0;
}

void TransitionCS::Init(mmCar* car)
{
    Car = car;
    CarMatrix = &car->Sim.LCS.World;
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
TransitionCS::~TransitionCS() = default;
