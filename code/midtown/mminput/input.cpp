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

define_dummy_symbol(mminput_input);

#include "input.h"

#include "agi/pipeline.h"
#include "arts7/sim.h"
#include "eventq7/keys.h"
#include "localize/localize.h"
#include "mmcityinfo/state.h"

#include "io.h"
#include "joyman.h"

enum
{
    kcaCantAssign = 0,
    kcaCanAssign = 1,
    kcaAlreadyAssigned = 2,
};

void mmInput::AutoSetup()
{
    MMSTATE.EnableFF = false;

    if (DeviceConnected())
    {
        MMSTATE.InputType = mmInputType::Joystick;

        if (JoystickConnected())
            MMSTATE.InputType = mmInputType::Joystick;
        else if (GamepadConnected())
            MMSTATE.InputType = mmInputType::Gamepad;
        else if (WheelConnected())
            MMSTATE.InputType = mmInputType::Wheel2Axis;

        Init(static_cast<i32>(MMSTATE.InputType));

        if (HasForceFeedback)
            MMSTATE.EnableFF = true;

        GameInput()->HasCoolie = JoystickHasCoolie();
    }
    else
    {
        // Used to be mouse. Bleh.
        MMSTATE.InputType = mmInputType::Keyboard;

        Init(static_cast<i32>(MMSTATE.InputType));
    }
}

void mmInput::FlagIODevChanged()
{
    for (i32 i = 0; i < IOID_COUNT; ++i)
        IO[i].Changed = true;
}

void mmInput::Flush()
{
    ProcessJoyEvents();

    CurrentState = 0;
    QueuedEvents = 0;

    eqEvent event;

    while (Events->Pop(&event))
        ;

    ClearEventHitFlags();
}

// mmIO::Flags is a mask of the input types a control will accept, one bit per
// ioType. There is no name for them in the headers, so they get one here.
#define IO_ACCEPTS_EVENT 1      // ioType::Event
#define IO_ACCEPTS_DISCRETE 2   // ioType::Discrete - key or button
#define IO_ACCEPTS_CONTINUOUS 4 // ioType::Continuous - mouse or joystick axis

void mmInput::IOInit(i32 ioid, LocString* name, ilong flags)
{
    IO[ioid].Init(name->Text, ioid, flags);
}

i32 mmInput::AttachToPipe()
{
    // Every one of these allocations is why this function had to come before
    // anything else in the input path: mmInput's constructor nulls IO, IODev,
    // Joy and Events, and this is the only place that fills them in. With it
    // stubbed, the first frame of the simulation dereferenced a null IO.
    Joy = new mmJoyMan();

    if (!GetParentNode())
    {
        ARTSPTR->InsertChild(1, this);

        // The original then published DirectInput's device into gpdi and handed
        // it, with the main window handle, to mmJoyMan::Init. None of that
        // exists here - SDL owns input on Android - so the joystick manager is
        // allocated but left uninitialized, exactly as it would be on a machine
        // with no joystick attached.
    }

    Events = new eqEventQ(1, -1, 32);

    // One device record per control per configuration, and one control record
    // per control. Both are plain arrays the original allocates with new[].
    IODev = new mmIODev[IODEV_NUM_CONFIGS * IOID_COUNT];
    IO = new mmIO[IOID_COUNT];

    // The controls, in the order the original registers them - which is neither
    // IOID order nor string order, and is preserved here because the string ids
    // are positional. The flags say which kinds of input each control accepts.
    static constexpr struct
    {
        i32 ioid;
        u32 string_id;
        i32 flags;
    } controls[IOID_COUNT] {
        {IOID_CAM, 0xFD, IO_ACCEPTS_EVENT},
        {IOID_XVIEW, 0xFE, IO_ACCEPTS_EVENT},
        {IOID_TRANS, 0xFF, IO_ACCEPTS_EVENT},
        {IOID_HORN, 0x100, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_THROT, 0x101, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_BRAKE, 0x102, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_STR, 0x103, IO_ACCEPTS_CONTINUOUS},
        {IOID_STRL, 0x104, IO_ACCEPTS_DISCRETE},
        {IOID_STRR, 0x105, IO_ACCEPTS_DISCRETE},
        {IOID_LOKR, 0x106, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_LOKL, 0x107, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_LOKB, 0x108, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_LOKF, 0x109, IO_ACCEPTS_DISCRETE | IO_ACCEPTS_CONTINUOUS},
        {IOID_WFOV, 0x10A, IO_ACCEPTS_EVENT},
        {IOID_DASH, 0x10B, IO_ACCEPTS_EVENT},
        {IOID_UPSH, 0x10C, IO_ACCEPTS_EVENT},
        {IOID_DWNS, 0x10D, IO_ACCEPTS_EVENT},
        {IOID_REV, 0x10E, IO_ACCEPTS_EVENT},
        {IOID_WYPTN, 0x10F, IO_ACCEPTS_EVENT},
        {IOID_WYPTP, 0x110, IO_ACCEPTS_EVENT},
        {IOID_MAP, 0x111, IO_ACCEPTS_EVENT},
        {IOID_HUD, 0x112, IO_ACCEPTS_EVENT},
        {IOID_FMAP, 0x113, IO_ACCEPTS_EVENT},
        {IOID_MAPRES, 0x114, IO_ACCEPTS_EVENT},
        {IOID_CDSHOW, 0x115, IO_ACCEPTS_EVENT},
        {IOID_CDPLAY, 0x116, IO_ACCEPTS_EVENT},
        {IOID_CDNEXT, 0x117, IO_ACCEPTS_EVENT},
        {IOID_CDPRIOR, 0x118, IO_ACCEPTS_EVENT},
        {IOID_MIRROR, 0x119, IO_ACCEPTS_EVENT},
        {IOID_PAN, 0x11A, IO_ACCEPTS_CONTINUOUS},
        {IOID_HAND, 0x11B, IO_ACCEPTS_DISCRETE},
        {IOID_OPPPOS, 0x11C, IO_ACCEPTS_EVENT},
        {IOID_CHAT, 0x11D, IO_ACCEPTS_EVENT},
    };

    for (const auto& c : controls)
        IOInit(c.ioid, AngelReadString(c.string_id), c.flags);

    NumControls = IOID_COUNT;

    RestoreDefaultConfig(0);

    return 1;
}

b32 mmInput::GamepadConnected()
{
    // TODO: Decide type of connected joystick (mmJoystick::DevInfo::dwDevType)
    return false;
}

f32 mmInput::GetBrakes()
{
    return SwapThrottle ? GetThrottleVal() : GetBrakesVal();
}

f32 mmInput::GetBrakesVal()
{
    switch (IO[IOID_BRAKE].GetIODev().IoType)
    {
        case ioType::Discrete: return IsInputPressed(IOID_BRAKE) ? 1.0f : 0.0f;
        case ioType::Continuous: return BrakesVal;
    }

    return 0.0f;
}

b32 mmInput::JoystickConnected()
{
    // TODO: Decide type of connected joystick (mmJoystick::DevInfo::dwDevType)
    return false;
}

b32 mmInput::WheelConnected()
{
    // TODO: Decide type of connected joystick (mmJoystick::DevInfo::dwDevType)
    return false;
}

void mmInput::ProcessEvents()
{
    ProcessKeyboardEvents();
    ProcessJoyEvents();
    ClearEventHitFlags();
}

void mmInput::Update()
{
    if (ResetMouse)
    {
        ResetMouse = false;

        MousePrevX = eqEventHandler::SuperQ->GetMouseVirtualX();
        MousePrevY = eqEventHandler::SuperQ->GetMouseVirtualY();

        ScreenCenterX = Pipe()->GetWidth() * 0.5f;
        ScreenCenterY = Pipe()->GetHeight() * 0.5f;

        InvScreenCenterX = 1.0f / ScreenCenterX;
        InvScreenCenterY = 1.0f / ScreenCenterY;
    }

    if (WantPoll)
    {
        StateCaptured = PollStates();
    }
    else
    {
        if (InputConfiguration > 1)
        {
            Joy->Update();
            Joy->GenerateEvents();
        }

        ProcessEvents();

        CurrentState = ProcessStates();
    }
}

void mmInput::ProcessKeyboardEvents()
{
    eqEvent event;

    while (Events->Pop(&event))
    {
        bool scan = false;

        if (event.Type == eqEventType::Keyboard)
        {
            if (event.Key.Char == 0) // Ignore WM_CHAR/SDL_TEXTINPUT
            {
                event.Key.Key = VirtualKeyToScanCode(event.Key.Key);

                if (event.Key.Key == 0)
                    event.Key.Key = ((event.Key.State >> 16) & 0xFF) | ((event.Key.State >> 17) & 0x80);

                scan = event.Key.IsPressEvent();
            }
        }
        else if (event.Type == eqEventType::Mouse)
        {
            scan = event.Mouse.Buttons;
        }

        if (scan)
        {
            if (i64 index = ScanForEvent(&event); index > 0)
                PutEventInQueue(index);
        }
    }
}

i64 mmInput::ScanForEvent(eqEvent* event)
{
    for (i32 i = 0; i < NumControls; ++i)
    {
        mmIO& io = IO[i];

        if (io.GetIODev().IoType == ioType::Event && !io.Pressed && (io == event))
        {
            io.Pressed = true;

            return i + 1;
        }
    }

    return 0;
}

#define kEvent (i32)(ioType::Event)
#define kContinuous (i32)(ioType::Continuous)
#define kDiscrete (i32)(ioType::Discrete)

#define MM_MOUSE (i32)(mmIODevice::Mouse)
#define MM_KEYBOARD (i32)(mmIODevice::Keyboard)
#define MM_JOYSTICK1 (i32)(mmIODevice::Joystick1)

#define mmiMOUSE (i32)(mmInputType::Mouse)
#define mmiKEYBOARD (i32)(mmInputType::Keyboard)
#define mmiJOYSTICK (i32)(mmInputType::Joystick)
#define mmiGAMEPAD (i32)(mmInputType::Gamepad)
#define mmiWHEEL2AXIS (i32)(mmInputType::Wheel2Axis)

#define kXaxis (i32)(mmJoyInput::Xaxis)
#define kYaxis (i32)(mmJoyInput::Yaxis)
#define kZaxis (i32)(mmJoyInput::Zaxis)
#define kUaxis (i32)(mmJoyInput::Uaxis)
#define kRaxis (i32)(mmJoyInput::Raxis)
#define kVaxis (i32)(mmJoyInput::Vaxis)
#define kPOVaxis (i32)(mmJoyInput::POVaxis)
#define kXaxisLeft (i32)(mmJoyInput::XaxisLeft)
#define kXaxisRight (i32)(mmJoyInput::XaxisRight)
#define kYaxisUp (i32)(mmJoyInput::YaxisUp)
#define kYaxisDown (i32)(mmJoyInput::YaxisDown)
#define kJButton1 (i32)(mmJoyInput::JButton1)
#define kJButton2 (i32)(mmJoyInput::JButton2)
#define kJButton3 (i32)(mmJoyInput::JButton3)
#define kJButton4 (i32)(mmJoyInput::JButton4)
#define kJButton5 (i32)(mmJoyInput::JButton5)
#define kJButton6 (i32)(mmJoyInput::JButton6)
#define kJButton7 (i32)(mmJoyInput::JButton7)
#define kJButton8 (i32)(mmJoyInput::JButton8)
#define kJButton9 (i32)(mmJoyInput::JButton9)
#define kJButton10 (i32)(mmJoyInput::JButton10)
#define kJButton11 (i32)(mmJoyInput::JButton11)
#define kJButton12 (i32)(mmJoyInput::JButton12)
#define kZaxisUp (i32)(mmJoyInput::ZaxisUp)
#define kZaxisDown (i32)(mmJoyInput::ZaxisDown)
#define kRaxisUp (i32)(mmJoyInput::RaxisUp)
#define kRaxisDown (i32)(mmJoyInput::RaxisDown)

void mmInput::SetDefaultConfig(i32 config)
{
    // NOTE: mmInput::SaveCodeConfig does not support IOID_OPPPOS, IOID_CHAT, k(Z|R)Axis(Up|Down)

    i32 old_config = std::exchange(InputConfiguration, config);

    switch (InputConfiguration)
    {
        case mmiMOUSE:
            IO[IOID_MAP].InitDev(kEvent, 0, MM_KEYBOARD, 0xf);
            IO[IOID_FMAP].InitDev(kEvent, 0, MM_KEYBOARD, 0x10);
            IO[IOID_MAPRES].InitDev(kEvent, 0, MM_KEYBOARD, 0x12);
            IO[IOID_HUD].InitDev(kEvent, 0, MM_KEYBOARD, 0x23);
            IO[IOID_STR].InitDev(kContinuous, 0, MM_MOUSE, kXaxis);
            IO[IOID_STRL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcb);
            IO[IOID_STRR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcd);
            IO[IOID_THROT].InitDev(kDiscrete, 0, MM_MOUSE, EQ_BUTTON_LEFT);
            IO[IOID_BRAKE].InitDev(kDiscrete, 0, MM_MOUSE, EQ_BUTTON_RIGHT);
            IO[IOID_HAND].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x39);
            IO[IOID_CAM].InitDev(kEvent, 0, MM_KEYBOARD, 0x2e);
            IO[IOID_XVIEW].InitDev(kEvent, 0, MM_KEYBOARD, 0x2f);
            IO[IOID_HORN].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x1c);
            IO[IOID_LOKL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4b);
            IO[IOID_LOKR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4d);
            IO[IOID_LOKB].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x50);
            IO[IOID_LOKF].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x48);
            IO[IOID_WFOV].InitDev(kEvent, 0, MM_KEYBOARD, 0x11);
            IO[IOID_DASH].InitDev(kEvent, 0, MM_KEYBOARD, 0x20);
            IO[IOID_TRANS].InitDev(kEvent, 0, MM_KEYBOARD, 0x14);
            IO[IOID_UPSH].InitDev(kEvent, 0, MM_KEYBOARD, 0x1e);
            IO[IOID_DWNS].InitDev(kEvent, 0, MM_KEYBOARD, 0x2c);
            IO[IOID_REV].InitDev(kEvent, 0, MM_KEYBOARD, 0x13);
            IO[IOID_WYPTN].InitDev(kEvent, 0, MM_KEYBOARD, 0x1f);
            IO[IOID_WYPTP].InitDev(kEvent, 0, MM_KEYBOARD, 0x2d);
            IO[IOID_CDSHOW].InitDev(kEvent, 0, MM_KEYBOARD, 0x3);
            IO[IOID_CDPLAY].InitDev(kEvent, 0, MM_KEYBOARD, 0x4);
            IO[IOID_CDPRIOR].InitDev(kEvent, 0, MM_KEYBOARD, 0x5);
            IO[IOID_CDNEXT].InitDev(kEvent, 0, MM_KEYBOARD, 0x6);
            IO[IOID_MIRROR].InitDev(kEvent, 0, MM_KEYBOARD, 0xe);
            IO[IOID_PAN].InitDev(kContinuous, 0, MM_JOYSTICK1, kPOVaxis);
            IO[IOID_OPPPOS].InitDev(kEvent, 0, MM_KEYBOARD, 0x17);
            IO[IOID_CHAT].InitDev(kEvent, 0, MM_KEYBOARD, 0x15);
            break;
        case mmiKEYBOARD:
            IO[IOID_MAP].InitDev(kEvent, 0, MM_KEYBOARD, 0xf);
            IO[IOID_FMAP].InitDev(kEvent, 0, MM_KEYBOARD, 0x10);
            IO[IOID_MAPRES].InitDev(kEvent, 0, MM_KEYBOARD, 0x12);
            IO[IOID_HUD].InitDev(kEvent, 0, MM_KEYBOARD, 0x23);
            IO[IOID_STR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x9);
            IO[IOID_STRL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcb);
            IO[IOID_STRR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcd);
            IO[IOID_THROT].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xc8);
            IO[IOID_BRAKE].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xd0);
            IO[IOID_HAND].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x39);
            IO[IOID_CAM].InitDev(kEvent, 0, MM_KEYBOARD, 0x2e);
            IO[IOID_XVIEW].InitDev(kEvent, 0, MM_KEYBOARD, 0x2f);
            IO[IOID_HORN].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x1c);
            IO[IOID_LOKL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4b);
            IO[IOID_LOKR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4d);
            IO[IOID_LOKB].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x50);
            IO[IOID_LOKF].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x48);
            IO[IOID_WFOV].InitDev(kEvent, 0, MM_KEYBOARD, 0x11);
            IO[IOID_DASH].InitDev(kEvent, 0, MM_KEYBOARD, 0x20);
            IO[IOID_TRANS].InitDev(kEvent, 0, MM_KEYBOARD, 0x14);
            IO[IOID_UPSH].InitDev(kEvent, 0, MM_KEYBOARD, 0x1e);
            IO[IOID_DWNS].InitDev(kEvent, 0, MM_KEYBOARD, 0x2c);
            IO[IOID_REV].InitDev(kEvent, 0, MM_KEYBOARD, 0x13);
            IO[IOID_WYPTN].InitDev(kEvent, 0, MM_KEYBOARD, 0x1f);
            IO[IOID_WYPTP].InitDev(kEvent, 0, MM_KEYBOARD, 0x2d);
            IO[IOID_CDSHOW].InitDev(kEvent, 0, MM_KEYBOARD, 0x3);
            IO[IOID_CDPLAY].InitDev(kEvent, 0, MM_KEYBOARD, 0x4);
            IO[IOID_CDPRIOR].InitDev(kEvent, 0, MM_KEYBOARD, 0x5);
            IO[IOID_CDNEXT].InitDev(kEvent, 0, MM_KEYBOARD, 0x6);
            IO[IOID_MIRROR].InitDev(kEvent, 0, MM_KEYBOARD, 0xe);
            IO[IOID_PAN].InitDev(kContinuous, 0, MM_JOYSTICK1, kPOVaxis);
            IO[IOID_OPPPOS].InitDev(kEvent, 0, MM_KEYBOARD, 0x17);
            IO[IOID_CHAT].InitDev(kEvent, 0, MM_KEYBOARD, 0x15);
            break;
        case mmiJOYSTICK:
            IO[IOID_MAP].InitDev(kEvent, 0, MM_KEYBOARD, 0xf);
            IO[IOID_FMAP].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton7);
            IO[IOID_MAPRES].InitDev(kEvent, 0, MM_KEYBOARD, 0x12);
            IO[IOID_HUD].InitDev(kEvent, 0, MM_KEYBOARD, 0x23);
            IO[IOID_STR].InitDev(kContinuous, 0, MM_JOYSTICK1, kXaxis);
            IO[IOID_STRL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcb);
            IO[IOID_STRR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcd);
            IO[IOID_THROT].InitDev(kContinuous, 0, MM_JOYSTICK1, kZaxisUp);
            IO[IOID_BRAKE].InitDev(kContinuous, 0, MM_JOYSTICK1, kZaxisDown);
            IO[IOID_HAND].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton3);
            IO[IOID_CAM].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton2);
            IO[IOID_XVIEW].InitDev(kEvent, 0, MM_KEYBOARD, 0x2f);
            IO[IOID_HORN].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton1);
            IO[IOID_LOKL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4b);
            IO[IOID_LOKR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4d);
            IO[IOID_LOKB].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x50);
            IO[IOID_LOKF].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x48);
            IO[IOID_WFOV].InitDev(kEvent, 0, MM_KEYBOARD, 0x11);
            IO[IOID_DASH].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton4);
            IO[IOID_TRANS].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton9);
            IO[IOID_UPSH].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton5);
            IO[IOID_DWNS].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton6);
            IO[IOID_REV].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton10);
            IO[IOID_WYPTN].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton8);
            IO[IOID_WYPTP].InitDev(kEvent, 0, MM_KEYBOARD, 0x2d);
            IO[IOID_CDSHOW].InitDev(kEvent, 0, MM_KEYBOARD, 0x3);
            IO[IOID_CDPLAY].InitDev(kEvent, 0, MM_KEYBOARD, 0x4);
            IO[IOID_CDPRIOR].InitDev(kEvent, 0, MM_KEYBOARD, 0x5);
            IO[IOID_CDNEXT].InitDev(kEvent, 0, MM_KEYBOARD, 0x6);
            IO[IOID_MIRROR].InitDev(kEvent, 0, MM_KEYBOARD, 0xe);
            IO[IOID_PAN].InitDev(kContinuous, 0, MM_JOYSTICK1, kPOVaxis);
            IO[IOID_OPPPOS].InitDev(kEvent, 0, MM_KEYBOARD, 0x17);
            IO[IOID_CHAT].InitDev(kEvent, 0, MM_KEYBOARD, 0x15);
            break;
        case mmiGAMEPAD:
            IO[IOID_MAP].InitDev(kEvent, 0, MM_KEYBOARD, 0xf);
            IO[IOID_FMAP].InitDev(kEvent, 0, MM_KEYBOARD, 0x10);
            IO[IOID_MAPRES].InitDev(kEvent, 0, MM_KEYBOARD, 0x12);
            IO[IOID_HUD].InitDev(kEvent, 0, MM_KEYBOARD, 0x23);
            IO[IOID_STR].InitDev(kContinuous, 0, MM_JOYSTICK1, kXaxis);
            IO[IOID_STRL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcb);
            IO[IOID_STRR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcd);
            IO[IOID_THROT].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton1);
            IO[IOID_BRAKE].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton2);
            IO[IOID_HAND].InitDev(kContinuous, 0, MM_JOYSTICK1, kYaxisDown);
            IO[IOID_CAM].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton4);
            IO[IOID_XVIEW].InitDev(kEvent, 0, MM_KEYBOARD, 0x2f);
            IO[IOID_HORN].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton3);
            IO[IOID_LOKL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4b);
            IO[IOID_LOKR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4d);
            IO[IOID_LOKB].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x50);
            IO[IOID_LOKF].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x48);
            IO[IOID_WFOV].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton6);
            IO[IOID_DASH].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton5);
            IO[IOID_TRANS].InitDev(kEvent, 0, MM_KEYBOARD, 0x14);
            IO[IOID_UPSH].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton8);
            IO[IOID_DWNS].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton7);
            IO[IOID_REV].InitDev(kEvent, 0, MM_KEYBOARD, 0x39);
            IO[IOID_WYPTN].InitDev(kEvent, 0, MM_KEYBOARD, 0x1f);
            IO[IOID_WYPTP].InitDev(kEvent, 0, MM_KEYBOARD, 0x2d);
            IO[IOID_CDSHOW].InitDev(kEvent, 0, MM_KEYBOARD, 0x3);
            IO[IOID_CDPLAY].InitDev(kEvent, 0, MM_KEYBOARD, 0x4);
            IO[IOID_CDPRIOR].InitDev(kEvent, 0, MM_KEYBOARD, 0x5);
            IO[IOID_CDNEXT].InitDev(kEvent, 0, MM_KEYBOARD, 0x6);
            IO[IOID_MIRROR].InitDev(kEvent, 0, MM_KEYBOARD, 0xe);
            IO[IOID_PAN].InitDev(kContinuous, 0, MM_JOYSTICK1, kPOVaxis);
            IO[IOID_OPPPOS].InitDev(kEvent, 0, MM_KEYBOARD, 0x17);
            IO[IOID_CHAT].InitDev(kEvent, 0, MM_KEYBOARD, 0x15);
            break;
        case mmiWHEEL2AXIS:
            IO[IOID_MAP].InitDev(kEvent, 0, MM_KEYBOARD, 0xf);
            IO[IOID_FMAP].InitDev(kEvent, 0, MM_KEYBOARD, 0x10);
            IO[IOID_MAPRES].InitDev(kEvent, 0, MM_KEYBOARD, 0x12);
            IO[IOID_HUD].InitDev(kEvent, 0, MM_KEYBOARD, 0x23);
            IO[IOID_STR].InitDev(kContinuous, 0, MM_JOYSTICK1, kXaxis);
            IO[IOID_STRL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcb);
            IO[IOID_STRR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0xcd);
            IO[IOID_THROT].InitDev(kContinuous, 0, MM_JOYSTICK1, kYaxisUp);
            IO[IOID_BRAKE].InitDev(kContinuous, 0, MM_JOYSTICK1, kYaxisDown);
            IO[IOID_HAND].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x39);
            IO[IOID_CAM].InitDev(kEvent, 0, MM_JOYSTICK1, kJButton2);
            IO[IOID_XVIEW].InitDev(kEvent, 0, MM_KEYBOARD, 0x2f);
            IO[IOID_HORN].InitDev(kDiscrete, 0, MM_JOYSTICK1, kJButton1);
            IO[IOID_LOKL].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4b);
            IO[IOID_LOKR].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x4d);
            IO[IOID_LOKB].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x50);
            IO[IOID_LOKF].InitDev(kDiscrete, 0, MM_KEYBOARD, 0x48);
            IO[IOID_WFOV].InitDev(kEvent, 0, MM_KEYBOARD, 0x11);
            IO[IOID_DASH].InitDev(kEvent, 0, MM_KEYBOARD, 0x20);
            IO[IOID_TRANS].InitDev(kEvent, 0, MM_KEYBOARD, 0x14);
            IO[IOID_UPSH].InitDev(kEvent, 0, MM_KEYBOARD, 0x1e);
            IO[IOID_DWNS].InitDev(kEvent, 0, MM_KEYBOARD, 0x2c);
            IO[IOID_REV].InitDev(kEvent, 0, MM_KEYBOARD, 0x13);
            IO[IOID_WYPTN].InitDev(kEvent, 0, MM_KEYBOARD, 0x1f);
            IO[IOID_WYPTP].InitDev(kEvent, 0, MM_KEYBOARD, 0x2d);
            IO[IOID_CDSHOW].InitDev(kEvent, 0, MM_KEYBOARD, 0x3);
            IO[IOID_CDPLAY].InitDev(kEvent, 0, MM_KEYBOARD, 0x4);
            IO[IOID_CDPRIOR].InitDev(kEvent, 0, MM_KEYBOARD, 0x5);
            IO[IOID_CDNEXT].InitDev(kEvent, 0, MM_KEYBOARD, 0x6);
            IO[IOID_MIRROR].InitDev(kEvent, 0, MM_KEYBOARD, 0xe);
            IO[IOID_PAN].InitDev(kContinuous, 0, MM_JOYSTICK1, kPOVaxis);
            IO[IOID_OPPPOS].InitDev(kEvent, 0, MM_KEYBOARD, 0x17);
            IO[IOID_CHAT].InitDev(kEvent, 0, MM_KEYBOARD, 0x15);
            break;
    }

    InputConfiguration = old_config;
}
// The original copies each of these into its own 32-byte buffer rather than pointing at
// the string table, so they stay writable.
static char* LocStrCopy(const char* text)
{
    char* buffer = new char[32];
    arts_strcpy(buffer, 32, text);
    return buffer;
}

mmInput::mmInput()
{
    ArAssert(GameInputPtr == nullptr, "Only one mmInput allowed");

    LocStrUndef = LocStrCopy(LOC_STR(MM_IDS_248));
    LocStrButton = LocStrCopy(LOC_STR(MM_IDS_249));
    LocStrJoystick = LocStrCopy(LOC_STR(MM_IDS_250));
    LocStrKey = LocStrCopy(LOC_STR(MM_IDS_251));

    GameInputPtr = this;
    IODev = nullptr;

    // ARTS_ZEROED already zeroed everything; only the non-zero defaults are set here,
    // plus the pointers the original clears explicitly.
    Joy = nullptr;
    IO = nullptr;
    Events = nullptr;

    CollideCooldown = 1.0f;
    CollidePeriod = 0.1f;

    AutoReverse = 1;

    DiscreteSteeringLimit = 1.0f;
    ForceFeedbackScale = 1.0f;
    RoadForceScale = 1.0f;
    JoyDeadZone = 0.1f;
    MouseSensitivity = 1.0f;
    UserSteeringSensitivity = 1.0f;
    DiscreteSteeringDeltaIn = 1.0f;
    DiscreteSteeringDeltaOut = 1.0f;
    SteeringExponent = 2.0f;

    ResetMouse = true;

    UseDIKey = std::atoi(LOC_STR(MM_IDS_USE_DI_KEYS));
}
