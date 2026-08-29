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

define_dummy_symbol(mmgame_player);

#include "player.h"

#include "arts7/sim.h"
#include "mmcamcs/viewcs.h"
#include "mmcar/carmodel.h"

void mmPlayer::AfterLoad()
{}

void mmPlayer::BeforeSave()
{}

static const f32 RegenFrameRate = 30.0f;
static const f32 PlayerRegenRate = RegenFrameRate * 0.0005f;

void mmPlayer::UpdateRegen()
{
    if (Car.Sim.ICS.GetVelocity().Mag2() > 25.0f)
    {
        if (f32 damage = Car.Sim.CurrentDamage; damage > 0.0f)
        {
            Car.Sim.CurrentDamage =
                std::max<f32>(0.0f, damage - (Car.Sim.MaxDamageScaled * PlayerRegenRate * Sim()->GetUpdateDelta()));
        }
    }
}

mmPlayer::mmPlayer()
{
    ViewCS = mmViewCS::Instance(&Camera);

    CamIndex = 0;
    WantPreRaceCam = 1;
    InAutoCam = 0;
    InPreRaceCam = 0;

    HogTimer = 0.0f;
    HogTimerLimit = 1.0f;
    HogSpeedLimit = 4.5f;
    field_30 = 0;

    Score = 0;
    RegenEnabled = 0;
    InWater = 0;
    DontResetDamage = 0;
    CamPan = 0.0f;

    ScoreWeight = 1.0f;
    Steering = 0.0f;
    PeggedTimer = 0.0f;
    PeggedTheshold = 0.999f;

    field_49C8 = 0.0f;
    field_49D8 = 1.0f;
    field_49DC = 20.0f;
    field_49E0 = 2.0f;
    field_49E4 = 0;
    field_49E6 = 0;
    field_49E8 = 0;
    field_49EC = 0;

    // Steering response, one set of numbers per input device
    SpeedSensitive = 2;
    SpeedBaseLow = 5.0f;
    SpeedBaseHi = 100.0f;

    DiscreteSteeringDeltaInHi = 1.5f;
    DiscreteSteeringDeltaOutHi = 2.5f;
    DiscreteSteeringFilterHi = 1.0f;
    DiscreteSteeringDeltaInLo = 2.5f;
    DiscreteSteeringDeltaOutLo = 3.5f;
    DiscreteSteeringFilterLo = 2.0f;

    MouseSensitivityLow = 0.6f;
    MouseSteerFilterLow = 1.5f;
    MouseSensitivityHi = 1.8f;
    MouseSteerFilterHi = 4.0f;

    SteerApp = 0;

    JoySteerApproachOutHi = 2.0f;
    JoySteerApproachInHi = 4.0f;
    JoySteerAppApp = 0.0f;
    JoySteerApproachOutLo = 10.0f;
    JoySteerApproachInLo = 10.0f;
    JoySteerFilterLow = 1.0f;
    JoySensitivityLow = 0.5f;
    JoySteerFilterHi = 3.0f;
    JoySensitivityHi = 1.1f;
    field_4A5C = 1.0f;
    field_4A60 = 0;

    WheelSteerApproachOutHi = 2.0f;
    WheelSteerApproachInHi = 4.0f;
    WheelSteerAppApp = 0.0f;
    WheelSteerApproachOutLo = 10.0f;
    WheelSteerFilterLow = 1.0f;
    WheelSensitivityLow = 0.5f;
    WheelSteerFilterHi = 3.0f;
    WheelSensitivityHi = 1.1f;
    field_4A94 = 1.0f;

    // The cameras that follow the car. PolarCam1/2, AiCam and PointCam are built but
    // not parented, as in the original.
    AddChild(&PovCam);
    AddChild(&NearCam);
    AddChild(&FarCam);
    AddChild(&PreCam);
    AddChild(&PostCam);
    AddChild(&DashCam);
    AddChild(&IndCam);

    Car.Sim.AddPlayerSpecifics();
    Car.Model.CarFlags |= CAR_FLAG_HIGH_QUALITY;
}
