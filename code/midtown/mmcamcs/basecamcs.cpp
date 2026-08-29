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

define_dummy_symbol(mmcamcs_basecamcs);

#include "basecamcs.h"

void BaseCamCS::MakeActive()
{}

void BaseCamCS::UpdateInput()
{}

void BaseCamCS::SetST(f32* /*arg1*/)
{}

BaseCamCS::BaseCamCS()
{
    ClearNodeFlag(NODE_FLAG_ACTIVE);

    camera_.Identity();
    matrix_.Identity();

    View = nullptr;

    BlendTime = 1.2f;
    BlendGoal = 1.0f;
    CameraFOV = 50.0f;
    CameraNear = 3.0f;
    CameraFar = 1600.0f;

    Active = 0;
}
