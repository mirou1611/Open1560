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

#include "core/pointer.h"

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
    if (static bool probed = false; !probed)
    {
        probed = true;
        Displayf("PROBE mmViewCS::Update current=%p transition=%p camera=%p children=%i", CurrentCam, Transition,
            Camera, NumChildren());
    }

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

mmViewCS::mmViewCS()
{
    LockCam = 0;
    TargetCam = nullptr;
    CurrentCam = nullptr;

    // Every camera change goes through this one node: it blends from whatever the
    // view is showing to whatever was asked for, and is the view current camera
    // for as long as the blend lasts.
    Transition = new TransitionCS();
    Transition->View = this;

    AddChild(Transition);

    // The view keeps tracking while the simulation is paused, so the camera can
    // still finish a move over a stopped world.
    SetNodeFlag(NODE_FLAG_UPDATE_PAUSED);

    Camera = nullptr;
    WideView = 0;
}

void mmViewCS::SetCamera(asCamera* camera)
{
    Camera = camera;

    // Nothing to set the view from until a camera is current.
    if (CurrentCam)
        camera->SetView(CurrentCam->CameraFOV * ARTS_DEG_TO_RAD, 1.25f, CurrentCam->CameraNear, agiRQ.FarClip);
}

mmViewCS* mmViewCS::Instance(asCamera* camera)
{
    mmViewCS* view = new mmViewCS();

    view->Init();

    // The camera hangs under the view, so asLinearCS::Update pushes the view matrix
    // before asCamera::Update reads it.
    view->AddChild(camera);
    view->SetCamera(camera);

    return view;
}

// The destructor is this class's key function, so it is defined here rather than
// inline: with it in the header the vtable is never emitted, and gen_stubs.py
// synthesizes one whose every slot is ArtsVirtualStub.
mmViewCS::~mmViewCS() = default;
