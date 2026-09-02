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

define_dummy_symbol(mmcamcs_polarcamcs);

#include "polarcamcs.h"

#include "mmcar/car.h"

// ?EnablePolarCamCollision@@3_NA
ARTS_EXPORT bool EnablePolarCamCollision = false;
PolarCamCS::PolarCamCS()
{
    CameraNear = 0.1f;

    field_118 = 2.5f;
    field_11C = 7.0f;
    field_120 = 2.5f;
    field_124 = 0.25f;
    field_128 = 2.0f;
}

void PolarCamCS::Init(mmCar* car)
{
    Car = car;
    CarMatrix = &car->Sim.LCS.World;

    // No suffix on this one - the polar cameras take the vehicle name as it stands.
    SetName(car->Sim.GetNodeName());
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
PolarCamCS::~PolarCamCS() = default;
