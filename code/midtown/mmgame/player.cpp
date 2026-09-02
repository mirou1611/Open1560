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
#include "mmcar/trailer.h"
#include "mmcamcs/transitioncs.h"
#include "mmaudio/manager.h"
#include "mmcity/cullcity.h"
#include "mmcityinfo/state.h"
#include "mmgame/rainaudio.h"
#include "mminput/input.h"
#include "mmphysics/phys.h"

#include <cmath>

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

void mmPlayer::UpdateHOG()
{
    // Hog: the car has come to rest on its roof or its side, so the up axis of its
    // matrix has fallen away from vertical. Held there long enough and it is put
    // back on the road, facing the way it was pointing.
    if (Car.Sim.ICS.Matrix.m1.y < 0.1f && Car.Sim.HasCollided && Car.Sim.Speed < HogSpeedLimit)
        HogTimer += Sim()->GetUpdateDelta();
    else
        HogTimer = 0.0f;

    if (HogTimer <= HogTimerLimit)
        return;

    // Reset() puts the car back at the sim's reset position, so that position is
    // borrowed for the one call and put back afterwards. Damage and score have to
    // survive it too - this is a righting, not a restart.
    const Vector3 old_position = Car.Sim.ResetPosition;
    const f32 old_rotation = Car.Sim.ResetRotation;

    Vector3 position = Car.Sim.ICS.Matrix.m3;
    position.y += 0.5f;

    Car.Sim.SetResetPos(position);
    Car.Sim.ResetRotation = std::atan2(Car.Sim.ICS.Matrix.m2.x, Car.Sim.ICS.Matrix.m2.z);

    const f32 damage = Car.Sim.CurrentDamage;
    const i32 score = Score;

    DontResetDamage = 1;

    Reset();

    Vector3 restore = old_position;
    Car.Sim.SetResetPos(restore);
    Car.Sim.ResetRotation = old_rotation;
    Score = score;
    Car.Sim.CurrentDamage = damage;
}

void mmPlayer::Update()
{
#ifdef ARTS_ANDROID
    // Bring-up probe: say once whether the view node under the player is reachable
    // at all, and what it is pointing at.
    static bool probed = false;

    if (!probed)
    {
        probed = true;

        if (ViewCS)
        {
            Displayf("PROBE mmPlayer::Update viewcs=%p active=%d parent=%p current=%p camera=%p camindex=%d", ViewCS,
                static_cast<i32>(ViewCS->IsNodeActive()), ViewCS->GetParentNode(), ViewCS->CurrentCam, ViewCS->Camera,
                static_cast<i32>(CamIndex));
        }
        else
        {
            // mmViewCS::Instance is still assembly, so the stub handed back null and
            // the player has no view at all - see the note above mmPlayer::Init.
            Displayf("PROBE mmPlayer::Update viewcs=null carflags=%08x", Car.Model.CarFlags);
        }
    }
#endif

    // Echo goes on while the car is in a tunnel or under a bridge.
    if (CullCity()->GetRoomFlags(Car.Model.ChainId) & 3)
        MMSTATE.AudFlags |= AudManager::GetEchoOnMask();
    else
        MMSTATE.AudFlags &= ~AudManager::GetEchoOnMask();

    // Steering response. Every input device has a slow set of numbers and a fast
    // set; SpeedSensitive picks between them - 0 always slow, 2 blends on speed,
    // anything else always fast. Only the blend depends on the device in use; the
    // two fixed cases write every device's numbers.
    const f32 sensitivity = GameInput()->UserSteeringSensitivity;

    if (SpeedSensitive == 2)
    {
        // The blend divides the clamped speed by the width of the range rather than
        // by the distance into it, so it does not start at zero. That is the
        // original arithmetic, not a transcription slip.
        const f32 speed = Car.Sim.Speed;
        const f32 clamped =
            (speed <= SpeedBaseLow) ? SpeedBaseLow : ((speed < SpeedBaseHi) ? speed : SpeedBaseHi);
        const f32 t = clamped / (SpeedBaseHi - SpeedBaseLow);

        switch (InputConfiguration)
        {
            case 0: {
                GameInput()->MouseSensitivity =
                    (MouseSensitivityLow + (MouseSensitivityHi - MouseSensitivityLow) * t) * sensitivity;
                MouseSteerFilterLow2 = MouseSteerFilterLow + (MouseSteerFilterHi - MouseSteerFilterLow) * t;
                break;
            }

            case 2: {
                field_4A5C = (JoySensitivityLow + (JoySensitivityHi - JoySensitivityLow) * t) * sensitivity;
                field_4A58 = JoySteerFilterLow + (JoySteerFilterHi - JoySteerFilterLow) * t;
                field_4A44 = JoySteerApproachOutLo + (JoySteerApproachOutHi - JoySteerApproachOutLo) * t;
                field_4A38 = JoySteerApproachInLo + (JoySteerApproachInHi - JoySteerApproachInLo) * t;
                break;
            }

            case 1:
            case 3: {
                GameInput()->DiscreteSteeringDeltaIn =
                    DiscreteSteeringDeltaInLo + (DiscreteSteeringDeltaInHi - DiscreteSteeringDeltaInLo) * t;
                GameInput()->DiscreteSteeringDeltaOut =
                    DiscreteSteeringDeltaOutLo + (DiscreteSteeringDeltaOutHi - DiscreteSteeringDeltaOutLo) * t;
                GameInput()->SteeringExponent =
                    DiscreteSteeringFilterLo + (DiscreteSteeringFilterHi - DiscreteSteeringFilterLo) * t;
                break;
            }

            default: {
                field_4A94 = (WheelSensitivityLow + (WheelSensitivityHi - WheelSensitivityLow) * t) * sensitivity;
                field_4A90 = WheelSteerFilterLow + (WheelSteerFilterHi - WheelSteerFilterLow) * t;
                field_4A7C = WheelSteerApproachOutLo + (WheelSteerApproachOutHi - WheelSteerApproachOutLo) * t;
                field_4A70 = WheelSteerApproachInLo + (WheelSteerApproachInHi - WheelSteerApproachInLo) * t;
                break;
            }
        }
    }
    else if (SpeedSensitive == 0)
    {
        GameInput()->MouseSensitivity = MouseSensitivityLow * sensitivity;
        MouseSteerFilterLow2 = MouseSteerFilterLow;

        // The original writes the scaled joystick sensitivity back into
        // JoySensitivityLow as well as into field_4A5C, which compounds it every
        // frame this branch runs. Kept as it is - the shipping default for
        // SpeedSensitive is 2, so it never ran.
        JoySensitivityLow = JoySensitivityLow * sensitivity;
        field_4A5C = JoySensitivityLow;

        field_4A58 = JoySteerFilterLow;
        GameInput()->DiscreteSteeringDeltaIn = DiscreteSteeringDeltaInLo;
        GameInput()->DiscreteSteeringDeltaOut = DiscreteSteeringDeltaOutLo;
        GameInput()->SteeringExponent = DiscreteSteeringFilterLo;
        field_4A44 = JoySteerApproachOutLo;
        field_4A38 = JoySteerApproachInLo;
        field_4A94 = WheelSensitivityLow * sensitivity;
        field_4A90 = WheelSteerFilterLow;
        field_4A7C = WheelSteerApproachOutLo;
        field_4A70 = WheelSteerApproachInLo;
    }
    else
    {
        GameInput()->MouseSensitivity = MouseSensitivityHi * sensitivity;
        MouseSteerFilterLow2 = MouseSteerFilterHi;
        field_4A5C = JoySensitivityHi * sensitivity;
        field_4A58 = JoySteerFilterHi;
        GameInput()->DiscreteSteeringDeltaIn = DiscreteSteeringDeltaInHi;
        GameInput()->DiscreteSteeringDeltaOut = DiscreteSteeringDeltaOutHi;
        GameInput()->SteeringExponent = DiscreteSteeringFilterHi;
        field_4A44 = JoySteerApproachOutHi;
        field_4A38 = JoySteerApproachInHi;
        field_4A94 = WheelSensitivityHi * sensitivity;
        field_4A90 = WheelSteerFilterHi;
        field_4A7C = WheelSteerApproachOutHi;
        field_4A70 = WheelSteerApproachInHi;
    }

    Car.Sim.HandBrake = GameInput()->GetHandBrake();

    if (ForceStop)
    {
        Car.Sim.Engine.Throttle = 0.0f;
        Car.Sim.Brakes = 1.0f;
        Car.Sim.Steering = -1.0f;
    }

    // Past maximum damage the car stops answering the controls at all.
    if (Car.Sim.EnableDamage && Car.Sim.CurrentDamage > Car.Sim.MaxDamageScaled)
    {
        Car.Sim.Engine.Throttle = 0.0f;
        Car.Sim.Steering = 0.0f;
        Car.Sim.Brakes = 0.0f;
    }

    // PATCH: Declare car mover before trailer (TODO: Also patch mmNetObject::Update?)
    PHYS.DeclareMover(&Car.Model, 1, 11);

    if (Car.Model.CarFlags & CAR_FLAG_TRAILER)
        PHYS.DeclareMover(&Car.Trailer->Inst, 1, 10);

    if (RainAudio)
        RainAudio->Update();

    UpdateHOG();

    if (RegenEnabled && !MMSTATE.DisableDamage)
        UpdateRegen();

    // Hitting the water hands the view to a fixed point camera above where the car
    // went in. InWater latches so it only happens once per dunking.
    if (HitWaterTimer != 0.0f && !InWater)
    {
        InWater = 1;

        if (Hud.IsDashActive())
            Hud.DashView.Deactivate();

        Car.Model.DashDeactivated();
        Car.Model.Activate();

        Vector3 position = ViewCS->Matrix.m3;
        position.y += 9.0f;

        PointCam.SetPos(position);

        Vector3 velocity {0.0f, 0.0f, 0.0f};
        PointCam.SetVel(velocity);

        ViewCS->NewCam(&PointCam, 3, 0.8f, Callback {});

        InAutoCam = 1;
    }

    // Coming out of the pre-race camera the view walks back to whatever the player
    // had selected, one camera at a time - near chase, then POV, then the dash.
    if (InPreRaceCam)
    {
        CarCamCS* wanted = MMSTATE.DashView ? &DashCam : CarCams[MMSTATE.CameraIndex];
        CarCamCS* current = ViewCS->CurrentCam;

        if (current == wanted)
        {
            InPreRaceCam = 0;
        }
        else if (wanted == &DashCam)
        {
            if (current == &NearCam)
            {
                ViewCS->NewCam(&PovCam, 1, 0.3f, Callback {});
            }
            else if (current == &PovCam)
            {
                ViewCS->NewCam(&DashCam, 2, 0.3f, Callback {});

                if (HudMap.GetMode() != HUD_MAP_MEDIUM)
                    Hud.ActivateDash();
            }
        }
        else if (wanted == &PovCam && current == &NearCam)
        {
            ViewCS->NewCam(&PovCam, 3, 0.5f, Callback {});
        }
    }

    // The speedometer reads the car speed only while it travels along its own
    // matrix, and zero the other way.
    Speed = ((Car.Sim.ICS.Matrix.m2 ^ Car.Sim.ICS.LinearVelocity) <= 0.0f) ? Car.Sim.SpeedMPH : 0.0f;

    IsPlayerAutoCam = (InPreRaceCam || InAutoCam) ? 1 : 0;

    // A large vehicle indoors is put on the industrial camera, because the chase
    // cameras end up inside the ceiling. The city camera comes back on the way out.
    if (Car.Model.CarFlags & CAR_FLAG_LARGE)
    {
        if (CullCity()->GetRoomFlags(Car.Model.ChainId) & 3)
        {
            if (ViewCS->CurrentCam == &FarCam || ViewCS->CurrentCam == &NearCam)
            {
                ViewCS->NewCam(&IndCam, 3, 1.0f, Callback {});
                RestoreCityCam = 1;
            }
        }
        else if (RestoreCityCam)
        {
            ViewCS->NewCam(CarCams[CamIndex], 3, 1.0f, Callback {});
            RestoreCityCam = 0;
        }
    }

    asNode::Update();
}
