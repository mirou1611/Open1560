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

define_dummy_symbol(mmcamcs_povcamcs);

#include "povcamcs.h"

#include "mmcar/car.h"
#include "mmcar/trailer.h"

void PovCamCS::UpdateInput()
{}

PovCamCS::PovCamCS()
{
    Active = 1;

    BlendTime = 1.2f;
    BlendGoal = 1.0f;
    CameraFOV = 60.0f;
    CameraNear = 3.0f;
    CameraFar = 1600.0f;

    ApproachOn = true;
    AppAppOn = true;
    AppRot = 28.0f;
    AppYPos = 28.0f;
    AppXZPos = 28.0f;
    AppApp = 0.7f;
    AppRotMin = 0.0f;
    AppPosMin = 0.0f;
    OneShot = false;
    MaxDist = 1.8f;
    MinDist = 1.74f;
    LookAt = 0.0f;

    Car = nullptr;

    // Offset carries its own initializer; everything after it is simply zeroed
    std::memset(gap124, 0, sizeof(gap124));
}

void PovCamCS::MakeActive()
{
    // From inside the car: the dash camera wants the interior, the plain POV camera
    // wants nothing drawn at all.
    if (IsDash)
        Car->Model.DashActivated();
    else
        Car->Model.Deactivate();

    if (mmTrailer* trailer = Car->Trailer)
        trailer->Inst.Flags &= ~INST_FLAG_ACTIVE;
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
PovCamCS::~PovCamCS() = default;
