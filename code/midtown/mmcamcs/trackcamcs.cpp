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

define_dummy_symbol(mmcamcs_trackcamcs);

#include "trackcamcs.h"

#include "arts7/sim.h"
#include "mmcar/car.h"
#include "mmcar/trailer.h"

static mem::cmd_param PARAM_preapproach {"preapproach", "Enable dynamic TrackCamCS AppXZPos calculations"};

void TrackCamCS::AfterLoad()
{
    CameraNear = 0.5f;

    if (bool enabled = false; PARAM_preapproach.get(enabled))
    {
        if (!enabled)
        {
            // preapproach is explicitly disabled
            MinAppXZPos = 0.0f;
        }
    }
    else
    {
        // preapproach is not specified, disable it for vanilla cameras (these values are the same for every MM1 TrackCamCS).
        // If you want to use these values, change AppXZPos to something slightly different.
        if ((MaxAppXZPos == 30.0f) && (MinSpeed == 5.0f) && (MaxSpeed == 35.0f) && (AppXZPos == 5.0f))
        {
            if (((MinAppXZPos == 1.0f) && (AppInc == 8.0f) && (AppDec == 5.0f)) ||
                ((MinAppXZPos == 1.8f) && (AppInc == 15.0f) && (AppDec == 10.0f)))
            {
                MinAppXZPos = 0.0f;
            }
        }
    }
}

void TrackCamCS::UpdateInput()
{}

void TrackCamCS::PreApproach()
{
    // AppXZPos is used to control how quickly the camera will attempt to adjust its X/Z coordinates to match the target position.
    // MinAppXZPos/MaxAppXZPos/MinSpeed/MaxSpeed/AppInc/AppDec can used to dynamically control AppXZPos based on the vehicle's speed.

    if (!Car)
    {
        return;
    }

    // Don't touch AppXZPos at all, unless we have valid parameters.
    if ((MinAppXZPos == 0.0f) || (MinSpeed == MaxSpeed))
    {
        return;
    }

    // As speed increases from MinSpeed to MaxSpeed, we interpolate from MaxAppXZPos to MinAppXZPos
    f32 speed_scale = 0.0f;

    if (CarVelocity <= MinSpeed)
    {
        speed_scale = 0.0f;
    }
    else if (CarVelocity >= MaxSpeed)
    {
        speed_scale = 1.0f;
    }
    else
    {
        speed_scale = (CarVelocity - MinSpeed) / (MaxSpeed - MinSpeed);
    }

    f32 target_xz = (MinAppXZPos - MaxAppXZPos) * speed_scale + MaxAppXZPos;

    if (Car->Sim.Trans.IsReverse())
    {
        target_xz = 20.0f;
    }

    f32 seconds = Sim()->GetUpdateDelta();

    if (target_xz < AppXZPos)
    {
        AppXZPos = std::max(target_xz, AppXZPos - (AppDec * seconds));
    }
    else if (target_xz > AppXZPos)
    {
        AppXZPos = std::min(target_xz, AppXZPos + (AppInc * seconds));
    }
}

void TrackCamCS::UpdateHill()
{}

TrackCamCS::TrackCamCS()
{
    camera_.Identity();

    BlendTime = 1.2f;
    BlendGoal = 1.0f;
    CameraFOV = 60.0f;
    CameraNear = 1.0f;
    CameraFar = 1600.0f;

    ApproachOn = true;
    AppAppOn = true;
    AppRot = 30.0f;
    AppXRot = 10.0f;
    AppYPos = 5.0f;
    AppApp = 0.7f;
    AppRotMin = 0.01f;
    AppPosMin = 0.25f;
    OneShot = false;
    MaxDist = 11.0f;
    MinDist = 7.93f;
    LookAt = 1.0f;

    Car = nullptr;

    MatrixTouched = true;
    Offset = {0.0f, 1.9f, 7.7f};

    CollideType = 2;
    EnableMinMax = 1;
    VerticalBreak = 0;

    MinAppXZPos = 1.8f;
    MaxAppXZPos = 12.0f;
    MinSpeed = 5.0f;
    MaxSpeed = 35.0f;
    AppInc = 15.0f;
    AppDec = 10.0f;
    MinHardSteer = 0.8f;
    DriftDelay = 0.3f;
    VertOffset = 0.6f;
    FrontRate = 0.55f;
    RearRate = 0.5f;
    FlipDelay = 0.5f;

    EnableSteer = 0;
    SteerMin = 0.5f;
    SteerAmt = 3.5f;

    // Shared between the three track cameras once one of them fills it in
    SharedData = new TrackCamData {};

    InAirTime = 0.0f;
    OnGroundTime = 0.0f;
    IsOnGround = true;
    SpinningReallyFast = false;
    field_184 = 0;

    SplineState1 = 0;
    SplineState2 = 0;
    SplineState3 = 0;
    field_194 = 0;
    field_198 = 0;
    field_19C = 0;
    field_1A0 = 0.0f;
    field_1A4 = 0;

    field_228 = 0.0f;
    field_22C = 0;
    CarSteering = 0.0f;
    CarVelocity = 0.0f;
    SteerTarget = 0.0f;
    field_240 = 0;
    field_244 = 0;

    field_250 = {0.0f, 0.0f, 0.0f};
    field_25C = {0.0f, 0.0f, 0.0f};
    field_268 = {0.0f, 0.0f, 0.0f};
}

void TrackCamCS::MakeActive()
{
    // A chase camera looks at the outside of the car, so the body has to be
    // drawn and the dashboard interior put away.
    Car->Model.Activate();
    Car->Model.DashDeactivated();

    if (mmTrailer* trailer = Car->Trailer)
        trailer->Inst.Flags |= INST_FLAG_ACTIVE;
}
