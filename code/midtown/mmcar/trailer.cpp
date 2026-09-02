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

define_dummy_symbol(mmcar_trailer);

#include "trailer.h"

#include "carsim.h"

#ifdef ARTS_DEV_BUILD
void mmTrailerInstance::AddWidgets(Bank* /*arg1*/)
{}
#endif

// Key functions - see the note in joint3dof.cpp.
mmTrailerInstance::~mmTrailerInstance() = default;

mmTrailer::~mmTrailer() = default;

Vector3& mmTrailerInstance::GetPos()
{
    return Trailer->ICS.Matrix.m3;
}

mmTrailer::mmTrailer()
{
    // The trailer body, its bound, its four driven wheels and the splash all hang
    // under the inertial frame, so they move with it.
    AddChild(&ICS);

    ICS.AddChild(&Bound);
    Bound.ICS = &ICS;

    ICS.AddChild(&DrivetrainFL);
    ICS.AddChild(&DrivetrainFR);
    ICS.AddChild(&DrivetrainBL);
    ICS.AddChild(&DrivetrainBR);
    ICS.AddChild(&Splash);

    // Only turned on when the trailer is actually in water.
    Splash.DeactivateNode();

    DrivetrainFL.AddChild(&WheelFL);
    DrivetrainFR.AddChild(&WheelFR);
    DrivetrainBL.AddChild(&WheelBL);
    DrivetrainBR.AddChild(&WheelBR);

    DrivetrainFL.AddWheel(&WheelFL);
    DrivetrainFR.AddWheel(&WheelFR);
    DrivetrainBL.AddWheel(&WheelBL);
    DrivetrainBR.AddWheel(&WheelBR);

    InertiaBox = {3.0f, 4.0f, 9.0f};
}

void mmTrailer::Activate()
{
    Inst.Flags |= INST_FLAG_ACTIVE;
}

void mmTrailer::Deactivate()
{
    Inst.Flags &= ~INST_FLAG_ACTIVE;
}
