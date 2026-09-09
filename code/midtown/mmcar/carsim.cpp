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

define_dummy_symbol(mmcar_carsim);

#include "carsim.h"

#include "agi/texdef.h"
#include "agiworld/meshset.h"
#include "agiworld/texsort.h"
#include "agi/dlptmpl.h"
#include "agi/getdlp.h"
#include "mmcity/inst.h"
#include "mmcityinfo/state.h"
#include "mminput/input.h"
#include "mmphysics/phys.h"
#include "midtown.h"

#include "car.h"
#include "roadff.h"

b32 EnableSmoke = true;
b32 ForceSmoke = false;

void mmCarSim::RestoreImpactParams()
{
    ICS.Elasticity = BoundElasticity;
    ICS.Friction = BoundFriction;
}

void mmCarSim::SetHackedImpactParams()
{
    ICS.Elasticity = 0.0f;
    ICS.Friction = 2.0f;
    Brakes = 1.0f;
}

void mmCarSim::SetResetPos(Vector3& pos)
{
    ResetPosition = pos;

    ResetPosition.y += (FrontRight.Radius - FrontRight.Center.y) - LCS.Matrix.m3.y;
}

b32 mmCarSim::ShouldSkid()
{
    return (Speed > 7.0f) || (Engine.Throttle > 0.5f) || (Brakes > 0.7f);
}

void mmCarSim::SetGlobalTuning(f32 /*arg1*/, f32 /*arg2*/)
{}

void mmCarSim::InitPtx()
{
    AsphaltRule.SetName("Asphalt");
    ExplosionRule.SetName("Explosion");
    OffroadRule.SetName("OffRoad");
    SlushRule.SetName("Slush");
    SnowRule.SetName("Snow");
    WaterRule.SetName("Water");
    SmokeRule.SetName("Smoke");

    AsphaltRule.Load();
    ExplosionRule.Load();
    OffroadRule.Load();
    SlushRule.Load();
    SnowRule.Load();
    WaterRule.Load();
    SmokeRule.Load();
}

mmCarSim::mmCarSim()
{
    field_1860 = 1.0f;
    field_1864 = 0;
    field_1868 = 0;
    field_186C = 0;
    field_1870 = 0;
    field_1874 = 0;

    HornPlaying = 0;
    PlayerCarAudio = nullptr;
    OpponentCarAudio = nullptr;
    PoliceCarAudio = nullptr;
    NetworkCarAudio = nullptr;

    CurrentDamage = 0.0f;
    MaxDamage = 1000000.0f;
    MedDamage = 500000.0f;
    MaxDamageScaled = 1000000.0f;
    MedDamageScaled = 500000.0f;
    EnableDamage = 1;

    DrivetrainType = 0;
    RealDrivetrainType = 0;
    DriverType = 0;

    Realism = nullptr;
    field_17E8 = 0;
    field_1800 = 0;
    Brakes = 0.0f;
    Steering = 0.0f;
    NumWheels = 4;

    ResetRotation = 0.0f;

    Vector3 reset_pos {0.0f, 0.0f, 0.0f};
    SetResetPos(reset_pos);

    ICS.Vel2 = 0.0f;

    AddChild(&Engine);
    AddChild(&Trans);
    AddChild(&ICS);

    AddChild(&Stuck);
    Stuck.Init(this);

    AddChild(&Splash);
    Splash.ClearNodeFlag(NODE_FLAG_ACTIVE);

    ICS.AddChild(&LCS);

    LCS.AddChild(&FrontAxle);
    LCS.AddChild(&BackAxle);

    RedistHeight = 1.0f;
    RedistLongRatio = 0.5f;

    for (mmWheel* wheel : {&FrontLeft, &FrontRight, &BackLeft, &BackRight})
    {
        wheel->StaticFric = 1.2f;
        wheel->SlidingFric = 0.9f;
    }

    LCS.AddChild(&Gyro);
    Gyro.field_2C = 6.0f;

    LCS.AddChild(&AeroCollide);
    AeroCollide.SetName("collide");

    LCS.AddChild(&Force);

    HasCollided = 0;

    // Force feedback shape
    TBWidth = 1.0f;
    TBHeight = 0.1f;
    field_1840 = 0;
    Gain = 1.0f;
    field_184C = 0.0f;
    field_1850 = 0.0f;
    CarRoadFF = nullptr;
    field_1F28 = 0.07f;
    EnableFF = 0;

    field_840 = 0.0f;
    field_844 = 1.0f;
    field_848 = 0.05f;
    field_84C = 0.05f;
    field_850 = 10.0f;
    field_854 = 80.0f;

    SmokeParticles.Init(32, 2, 2, 4, agiMeshSet::DefaultQuad);
    static char smoke_tex[] = "fxpt8";
    SmokeParticles.SetTexture(smoke_tex);

    GrassParticles.Init(32, 2, 2, 4, agiMeshSet::DefaultQuad);

    if (!GrassTex)
        GrassTex = GetPackedTexture("fxpt5"_xconst, 0).release();

    GrassParticles.SetTexture(GrassTex);

    ExplosionParticles.Init(32, 8, 8, 4, agiMeshSet::DefaultQuad);
    static char explosion_tex[] = "explosion";
    ExplosionParticles.SetTexture(explosion_tex);
    ExplosionParticles.SetBirthRule(&ExplosionRule);

    Exploded = 0;

    SmokePtx = 0.8f;
    Damage = 0.0f;
    CarFrictionHandling = 1.0f;
    LongSlideMultiplier = 1.0f;

    ExhaustSmokeOffset = {0.66f, 3.98f, 0.93f};
    ExhaustParticleMultiplier = 1.35f;
    EnableExhaust = 0;

    SlipPercentThresh = 0.5f;

    // Drift and spin handling
    DriftThreshold = 0.6f;
    SpinThreshold = 0.9f;

    SpinSight = 0.0f;
    SpinStartTime = 0.25f;
    SpinEndTime = 0.35f;
    SpinStart = 0.9f;
    SpinStop = 1.2f;
    SpinFromMax = 0.3f;
    SpinFromMin = 0.0f;
    SpinToMin = 0.6f;
    SpinToMax = 1.0f;

    FrontDriftFricMultiplier = 0.98f;
    BackDriftFricMultiplier = 0.97f;
    FrontSpinFricMultiplier = 1.0f;
    BackSpinFricMultiplier = 0.2f;
    BrakeFrontFricMultiplier = 1.1f;
    BrakeBackFricMultiplier = 0.9f;

    SteerMultiplier = 1.0f;
    FrontFriction = 1.0f;
    BackFriction = 1.0f;
    field_1F90 = 1.0f;

    DashCamHeadlightOffset = {0.0f, 0.0f, 0.0f};
    POVCamHeadlightOffset = {0.0f, 0.0f, 0.0f};
}

i32 mmCarSim::OnGround()
{
    return FrontLeft.OnGround || FrontRight.OnGround || BackLeft.OnGround || BackRight.OnGround;
}

// The AI's physics realism. The original never writes it - it is a zero the opponent
// and police cars point at, where the player points at MMSTATE.PhysicsRealism.
static f32 AIRealism = 0.0f;

void mmCarSim::Init(aconst char* name, mmCar* car, i32 driver_type)
{
    Car = car;
    Model = &car->Model;

    // The player drives at whatever realism the menu asked for; everyone else does not.
    Realism = &MMSTATE.PhysicsRealism;

    if (driver_type == 2)
    {
        SetName(formatf("%s_opp", name));
        Realism = &AIRealism;
    }
    else if (driver_type == 3)
    {
        SetName(formatf("%s_cop", name));
        Realism = &AIRealism;
    }
    else
    {
        SetName(name);
    }

    Load();

    ConfigureDrivetrain();

    BoundElasticity = 0.3f;
    ICS.Elasticity = 0.3f;

    BoundFriction = 0.2f;
    ICS.Friction = 0.2f;

    // A large vehicle is never treated as stuck - it is meant to shove things aside.
    if (Model->CarFlags & CAR_FLAG_LARGE)
        Stuck.DeactivateNode();

    DriverType = driver_type;
    Engine.GCL = 0.25f;

    InitPtx();

    ICS.SetMass(InertiaBox.x, InertiaBox.y, InertiaBox.z, ICS.Mass);

    ICS.Gravity = {0.0f, PHYS.Gravity, 0.0f};

    ICS.MaxAngVelocity = ARTS_PI * 4.0f;
    ICS.LimitAngVelocity = true;

    Bound.ICS = &ICS;
    Bound.Callback = reinterpret_cast<void (*)(void*, asBound*, mmIntersection*, Vector3*, f32, Vector3*)>(IMPACTCB);
    Bound.Param = this;

    Vector3 bound_min;
    Vector3 bound_max;

    if (DLPTemplate* dlp = GetDLPTemplate(name))
    {
        dlp->BoundBox(bound_min, bound_max, "BODY_H"_xconst);

        Dimensions = {bound_max.x - bound_min.x, bound_max.y - bound_min.y, bound_max.z - bound_min.z};

        // The middle pair of a six-wheeler is not suspended, so where they sit comes
        // straight out of the geometry rather than from a wheel.
        dlp->GetCentroid(WHL2_Pos, "WHL2_H"_xconst);
        dlp->GetCentroid(WHL3_Pos, "WHL3_H"_xconst);

        dlp->Release();
    }
    else
    {
        Warningf("Not able to calc car dimensions");

        Dimensions = {2.0f, 1.5f, 5.0f};

        // The original leaves bound_min and bound_max unwritten on this path and hands
        // them to mmSplash::Init below regardless. Zeroed here rather than passing
        // whatever was on the stack.
        bound_min = {0.0f, 0.0f, 0.0f};
        bound_max = {0.0f, 0.0f, 0.0f};
    }

    Splash.Init(&ICS, bound_min, bound_max);

    const Vector3 origin {0.0f, 0.0f, 0.0f};

    FrontLeft.Init(name, "WHL0_H"_xconst, origin, &ICS, NumWheels, nullptr, 0);
    FrontRight.Init(name, "WHL1_H"_xconst, origin, &ICS, NumWheels, nullptr, 0);

    if (Model->CarFlags & CAR_FLAG_6_WHEELS)
    {
        // On a six-wheeler WHL2 and WHL3 are the fixed middle pair, so the driven rear
        // wheels are 4 and 5.
        BackLeft.Init(name, "WHL4_H"_xconst, origin, &ICS, NumWheels, nullptr, 2);
        BackRight.Init(name, "WHL5_H"_xconst, origin, &ICS, NumWheels, nullptr, 2);
    }
    else
    {
        BackLeft.Init(name, "WHL2_H"_xconst, origin, &ICS, NumWheels, nullptr, 2);
        BackRight.Init(name, "WHL3_H"_xconst, origin, &ICS, NumWheels, nullptr, 2);
    }

    FrontLeft.CarSim = this;
    FrontRight.CarSim = this;
    BackLeft.CarSim = this;
    BackRight.CarSim = this;

    // DriveTrain3 is the one that is attached; 1 and 2 are only configured.
    DriveTrain3.Init(this);
    DriveTrain3.Attach();

    DriveTrain1.Init(this);
    DriveTrain2.Init(this);

    Gyro.CarSim = this;
    Force.CarSim = this;
    AeroCollide.ICS = &ICS;

    Engine.Init(this);
    Trans.Init(this);

    FrontAxle.Init(name, "AXLE0"_xconst, &FrontLeft, &FrontRight);
    BackAxle.Init(name, "AXLE1"_xconst, &BackLeft, &BackRight);

#ifndef ARTS_NO_AUDIO
    // One audio object per kind of driver, built once.
    if (driver_type == 0 && !PlayerCarAudio)
        PlayerCarAudio = new mmPlayerCarAudio(this);

    if (driver_type == 1 && !NetworkCarAudio)
        NetworkCarAudio = new mmNetworkCarAudio(this);

    if (driver_type == 2 && !OpponentCarAudio)
        OpponentCarAudio = new mmOpponentCarAudio(this);

    if (driver_type == 3 && !PoliceCarAudio)
        PoliceCarAudio = new mmPoliceCarAudio(this, 0.95f);
#endif

    if (GameInput()->DoingFF() && EnableFF)
        CarRoadFF->AssignProperties(1.0f, 0);

    MedDamage = MaxDamage * 0.33f;
    MaxDamageScaled = GlobalDamageScale * MaxDamage;
    MedDamageScaled = GlobalDamageScale * MedDamage;

    Reset();
}
