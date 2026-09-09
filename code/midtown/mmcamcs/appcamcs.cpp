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

define_dummy_symbol(mmcamcs_appcamcs);

#include "appcamcs.h"

AppCamCS::AppCamCS()
{
    CarMatrix = nullptr;

    field_FC = ORIGIN;
    field_108 = ORIGIN;

    ApproachOn = false;
    AppAppOn = false;
    AppRot = 0.0f;
    AppXRot = 0.0f;
    AppYPos = 0.0f;
    AppXZPos = 0.0f;
    AppApp = 0.0f;
    AppRotMin = 0.0f;
    AppPosMin = 0.0f;
    LookAbove = false;
    OneShot = false;
    MaxDist = 0.0f;
    MinDist = 0.0f;
    LookAt = 0.0f;
    field_E4 = 0.0f;
    field_E8 = 0.0f;
    field_EC = 0.0f;

    TrackTo = {0.0f, 0.8f, 0.0f};
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
AppCamCS::~AppCamCS() = default;

void AppCamCS::ApproachIt()
{
    // UpdatePOV and friends build matrix_ - where the camera wants to be. This is
    // what turns that into camera_, either easing towards it or, when the camera is
    // not approaching or is being placed for one frame only, snapping to it.
    // PORT SHIM: UpdateApproach is still assembly - it and the 1500 lines under it
    // (DApproach, UpdateMaxDist, Matrix34::GetEulers) are the easing. Its stub leaves
    // camera_ as the zero matrix BaseCamCS value-initialised, which makes the whole
    // view degenerate and is why nothing drew. Snap until it is written; restore the
    // condition, not the branch.
    //
    // if (ApproachOn && !OneShot)
    //     UpdateApproach();
    // else
    camera_ = matrix_;
}
