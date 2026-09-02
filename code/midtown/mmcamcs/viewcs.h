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

#pragma once

#include "arts7/linear.h"

#include "data7/callback.h"

class asCamera;
class CarCamCS;
class TransitionCS;

class mmViewCS final : public asLinearCS
{
public:
    // ??0mmViewCS@@QAE@XZ
    ARTS_IMPORT mmViewCS();

    // ??1mmViewCS@@UAE@XZ
    ARTS_EXPORT ~mmViewCS() override = default;

#ifdef ARTS_DEV_BUILD
    // ?AddWidgets@mmViewCS@@UAEXPAVBank@@@Z | inline
    ARTS_EXPORT void AddWidgets(Bank* arg1) override;
#endif

    // ?GetClass@mmViewCS@@UAEPAVMetaClass@@XZ
    ARTS_IMPORT MetaClass* GetClass() override;

    // ?Init@mmViewCS@@QAEXXZ
    ARTS_EXPORT void Init();

    // ?NewCam@mmViewCS@@QAEHPAVCarCamCS@@HMVCallback@@@Z
    ARTS_EXPORT i32 NewCam(CarCamCS* cam, i32 type, f32 time, Callback callback);

    // ?Reset@mmViewCS@@UAEXXZ
    ARTS_IMPORT void Reset() override;

    // ?SetCamera@mmViewCS@@QAEXPAVasCamera@@@Z
    ARTS_IMPORT void SetCamera(asCamera* arg1);

    // ?SetCurrentCam@mmViewCS@@QAEXPAVCarCamCS@@@Z
    ARTS_EXPORT void SetCurrentCam(CarCamCS* cam);

    // ?Update@mmViewCS@@UAEXXZ
    ARTS_EXPORT void Update() override;

    // ?DeclareFields@mmViewCS@@SAXXZ
    ARTS_IMPORT static void DeclareFields();

    // ?Instance@mmViewCS@@SAPAV1@PAVasCamera@@@Z
    ARTS_IMPORT static mmViewCS* Instance(asCamera* arg1);

    // The wide view: SetCurrentCam swaps in a fixed 1.74 rad / 2.54 aspect view
    // instead of the camera's own FOV.
    b32 WideView;

    // What the pending NewCam asked for. TransitionCS reads them when it starts.
    i16 TransitionType;
    i16 field_8E;
    f32 TransitionTime;
    Callback TransitionDone;

    // Whatever is driving the view this frame - either a CarCamCS, or Transition
    // while one camera is blending into the next.
    CarCamCS* CurrentCam;

    // Where the transition is headed, and the transition itself.
    CarCamCS* TargetCam;
    TransitionCS* Transition;
    i32 field_B4;

    // Set while the view is locked; NewCam refuses to start a transition.
    i32 LockCam;

    asCamera* Camera;
};

check_size(mmViewCS, 0xC0);
