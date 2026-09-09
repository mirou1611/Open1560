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

#include "arts7/sim.h"
#include "mmcar/carsim.h"
#include "mminput/input.h"
#include "vector7/randmath.h"

#include <cmath>

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
    Pitch = 0.0f;
    std::memset(gap138, 0, sizeof(gap138));
    IsDash = 0;
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

void PovCamCS::Update()
{
    UpdatePOV();

    // One-shot placement lasts exactly the frame it was asked for.
    OneShot = false;
}

void PovCamCS::UpdatePOV()
{
    // The car's own axes, normalised - its matrix carries scale the camera must not
    // inherit.
    Matrix34 car = *CarMatrix;

    car.m0 = ~car.m0;
    car.m1 = ~car.m1;
    car.m2 = ~car.m2;

    // Sit at the head position, facing the way the car faces.
    matrix_.Identity();
    matrix_.m3 = Offset;
    matrix_.Dot(matrix_, car);

    matrix_.Rotate(matrix_.m0, Pitch);

    if (f32 pan = GameInput()->GetCamPan() * (ARTS_PI * 2.0f); pan != 0.0f)
        matrix_.Rotate(matrix_.m1, pan);

    if (Car->Sim.OnGround() && !Sim()->IsPaused())
    {
        // Road shake, ramped in over half a turn of wheel travel and scaled by how
        // battered the car is - an undamaged car does not shake at all.
        f32 shake =
            (std::fabs(Car->Sim.FrontLeft.RotationSpeed) * Sim()->GetUpdateDelta() - (ARTS_PI * 0.5f)) / ARTS_PI;

        shake = (shake <= 0.0f) ? 0.0f : ((shake >= 1.0f) ? 1.0f : shake);

        Vector3 axis {matrix_.m0.x + matrix_.m2.x, matrix_.m0.y + matrix_.m2.y, matrix_.m0.z + matrix_.m2.z};

        matrix_.Rotate(axis, static_cast<f32>((frand() - 0.5) * Car->Sim.Damage * shake * 0.03));

        // And a slower wobble in step with the front wheel, which takes over as the
        // random shake fades out.
        const f64 scale = (Car->Model.CarFlags & CAR_FLAG_6_WHEELS) ? 0.005 : 0.008;

        matrix_.Rotate(
            axis, static_cast<f32>((1.0f - shake) * Car->Sim.Damage * std::sin(Car->Sim.FrontLeft.Rotation) * scale));
    }

#ifdef ARTS_ANDROID
    // Bring-up probe: where the car actually is, and where that puts the camera.
    if (static bool probed = false; !probed)
    {
        probed = true;

        Displayf("PROBE UpdatePOV car=(%.2f %.2f %.2f) world=(%.2f %.2f %.2f) target=(%.2f %.2f %.2f) cam=(%.2f %.2f %.2f)",
            static_cast<f64>(Car->Sim.ICS.Matrix.m3.x), static_cast<f64>(Car->Sim.ICS.Matrix.m3.y),
            static_cast<f64>(Car->Sim.ICS.Matrix.m3.z), static_cast<f64>(CarMatrix->m3.x),
            static_cast<f64>(CarMatrix->m3.y), static_cast<f64>(CarMatrix->m3.z),
            static_cast<f64>(matrix_.m3.x), static_cast<f64>(matrix_.m3.y), static_cast<f64>(matrix_.m3.z),
            static_cast<f64>(camera_.m3.x), static_cast<f64>(camera_.m3.y), static_cast<f64>(camera_.m3.z));
    }
#endif

    ApproachIt();
}
