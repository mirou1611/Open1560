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

define_dummy_symbol(mmcamcs_carcamcs);

#include "carcamcs.h"

#include "mmcar/car.h"

CarCamCS::CarCamCS()
{
    Car = nullptr;
}

void CarCamCS::Init(mmCar* car, aconst char* name)
{
    Car = car;

    // Every camera that follows a car tracks the same matrix: the world transform of
    // the car body, not the physics matrix.
    CarMatrix = &car->Sim.LCS.World;

    SetName(name);

    Load();
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
CarCamCS::~CarCamCS() = default;
