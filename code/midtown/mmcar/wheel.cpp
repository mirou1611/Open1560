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

define_dummy_symbol(mmcar_wheel);

#include "wheel.h"

#include "arts7/sim.h"
#include "mmbangers/active.h"

#include "car.h"

void mmWheel::GenerateSkidParticles()
{
    const f32 drift = std::abs(LongSlipPercent);
    const f32 skid = std::abs(LatSlipPercent);
    const f32 slip = std::max(drift, skid);
    const f32 speed = std::clamp(CarSim->Speed * 0.1f, 0.1f, 1.0f);
    ParticleCount += Sim()->GetUpdateDelta() * ParticleMultiplier * PtxMaxSkidCount * std::max(slip * speed, 0.25f);
}

static const f32 PtxFrameRate = 30.0f;
static mem::cmd_param PARAM_maxskid {"maxskid"};
hook_func(INIT_main, [] { mmWheel::PtxMaxSkidCount = PtxFrameRate * PARAM_maxskid.get_or(1.0f); });
mmWheel::mmWheel()
{
    ICS = nullptr;

    Spring = 40000.0f;
    Damping = 4000.0f;
    SteeringRatio = 0.5f;
    BrakeRatio = 0.85f;
    StaticFriction = 1.0f;
    SuspensionLimit = 0.1f;
    SuspensionBlend = 0.5f;
    RenderableSuspensionLimit = 0.1f;

    dword168 = 0;

    Center = {0.0f, 0.0f, 0.0f};

    Radius = 0.3f;
    Width = 0.1f;

    BumpHeight = 0.0f;
    BumpWidth = 2.5f;

    dword1C0 = 0;
    Wobble = 0.0f;
    Rotation = 0.0f;
    MaybeGrip = 0.0f;
    Steering = 0.0f;
    Suspension = 0.0f;
    dword1E4 = 0.0f;
    OtherNormalLoad = 0.25f;
    FullUpdated = 0;

    CurrentTireDispLat = 0.0f;
    CurrentTireDispLong = 0.0f;
    TireResistance = 0.0f;
    RotationSpeed = 0.0f;

    LatSlipPercent = 200000.0f;
    RubberSpring = 40000.0f;
    RubberDamp = 2000.0f;
    OptimumSlipPercent = 0.14f;
    StaticFric = 0.8f;
    SlidingFric = 0.4f;
    RubberSpringLat = 0.0f;
    RubberDampLat = 0.0f;

    ComputeConstants();

    FricMultiplier = 1.0f;
    SteerMultiplier = 1.0f;

    dword244 = 0;
    SteeringInput = 0.0f;
    BrakingInput = 0.0f;
    Bound = nullptr;
    HitPoly = nullptr;
    Phys = nullptr;
    PtxCount = 0.0f;
    ParticleCount = 0.0f;
}
