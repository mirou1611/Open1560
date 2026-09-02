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

define_dummy_symbol(mmcamcs_aicamcs);

#include "aicamcs.h"

#include "mmcar/car.h"

AICamCS::AICamCS()
{
    field_118 = 30.0f;
    field_11C = 1.5f;
}

void AICamCS::Init(mmCar* car)
{
    // The AI camera is driven from whatever it is told to look at, so it takes no
    // car matrix and no name.
    Car = car;
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
AICamCS::~AICamCS() = default;
