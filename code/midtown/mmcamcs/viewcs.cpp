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

define_dummy_symbol(mmcamcs_viewcs);

#include "viewcs.h"

#include "agiworld/quality.h"
#include "arts7/camera.h"
#include "carcamcs.h"
#include "transitioncs.h"

#ifdef ARTS_DEV_BUILD
void mmViewCS::AddWidgets(Bank* /*arg1*/)
{}
#endif

void mmViewCS::Init()
{}

void mmViewCS::Update()
{
    // The current camera works out where it wants to be, and the view simply takes
    // that matrix. Whatever is in CurrentCam is what the player sees - during a
    // blend it is Transition, not the camera the blend is headed for.
    CurrentCam->Update();

    Matrix = CurrentCam->camera_;

    asLinearCS::Update();
}

void mmViewCS::SetCurrentCam(CarCamCS* cam)
{
    CurrentCam = cam;

    if (WideView)
        Camera->SetView(1.74f, 2.54f, cam->CameraNear, agiRQ.FarClip);
    else
        Camera->SetView(cam->CameraFOV * ARTS_DEG_TO_RAD, 1.25f, cam->CameraNear, agiRQ.FarClip);

    cam->MakeActive();
}

i32 mmViewCS::NewCam(CarCamCS* cam, i32 type, f32 time, Callback callback)
{
    TransitionType = static_cast<i16>(type);
    TransitionDone = callback;
    TransitionTime = time;

    if (CurrentCam == Transition)
    {
        // Already blending - redirect the blend in flight rather than starting a
        // second one from a camera that is only halfway anywhere.
        Transition->NextTransition(cam);
    }
    else
    {
        if (LockCam)
            return 0;

        Transition->NewTransition(CurrentCam, cam);
        CurrentCam = Transition;
    }

    if (!cam->Active)
        cam->MakeActive();

    TargetCam = cam;

    return 1;
}
