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
