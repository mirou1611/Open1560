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

define_dummy_symbol(mmgame_hud);

#include "hud.h"

#include "arts7/sim.h"
#include "agi/pipeline.h"
#include "localize/localize.h"
#include "mmaudio/sound.h"
#include "mmcity/cullcity.h"
#include "mmcityinfo/state.h"
#include "mmeffects/meshform.h"

#include "gameman.h"

f32 mmTimer::GetTime()
{
    return Time;
}

void mmTimer::Init(b32 count_down, f32 start_time)
{
    CountDown = count_down;
    StartTime = start_time;
    Time = start_time;
}

void mmTimer::Reset()
{
    Time = StartTime;
}

void mmTimer::Start()
{
    Running = true;
}

void mmTimer::StartStop()
{
    Running ^= true;
}

void mmTimer::Stop()
{
    Running = false;
}

void mmTimer::Update()
{
    if (!Running)
        return;

    if (CountDown)
    {
        Time -= Sim()->GetUpdateDelta();

        if (Time <= 0.0f)
        {
            Time = 0.0f;
            Running = false;
        }
    }
    else
    {
        Time += Sim()->GetUpdateDelta();
    }
}

void mmHUD::ToggleMirror()
{
    ScreenClearCount = 3;
    CullCity()->RenderWeb.EnableMirror ^= true;
    MMSTATE.EnableMirror = CullCity()->RenderWeb.EnableMirror;
}

void mmHUD::TogglePositionDisplay(i32 mode)
{
    // FIXME: Move to constructor
    if (DashView.GetParentNode() == &HudElements)
    {
        // DashView is not a HUD element, and should not be hidden when a menu is shown.
        // To avoid drawing over the HUD, it should also come before HudElements
        HudElements.RemoveChild(&DashView);
        InsertChild(1, &DashView);

        // Show position text while paused
        PositionText.SetNodeFlag(NODE_FLAG_UPDATE_PAUSED);
    }

    ShowPosition = (mode != -1) ? (mode != 0) : !ShowPosition;

    if (ShowPosition)
        PositionText.ActivateNode();
    else
        PositionText.DeactivateNode();
}

void mmHUD::UpdatePaused()
{}

mmArrow::mmArrow()
{
    LinearCS = arnew asLinearCS();

    GreenArrow = arnew asMeshSetForm();
    GreenArrow->SetShape("hudarrow1"_xconst, "BOTTOM"_xconst, nullptr);
    asMeshSetForm::Lighter = nullptr;
    asMeshSetForm::SphMapTex = nullptr;
    GreenArrow->SetZRead(false);
    GreenArrow->SetZWrite(false);
    GreenArrow->Color = 0x8000FF00;

    YellowArrow = arnew asMeshSetForm();
    YellowArrow->SetShape("hudarrow2"_xconst, "BOTTOM"_xconst, nullptr);
    asMeshSetForm::Lighter = nullptr;
    asMeshSetForm::SphMapTex = nullptr;
    YellowArrow->SetZRead(false);
    YellowArrow->SetZWrite(false);
    YellowArrow->Color = 0x80FFFF00;

    LinearCS->Matrix.m3 = {0.0f, 2.0f, -6.1f};
    Interest = 0;
    Transform = 0;

    LinearCS->AddChild(GreenArrow.get());
    LinearCS->AddChild(YellowArrow.get());
    AddChild(LinearCS.get());

    Color = 2;
}

void mmArrow::Init(Matrix34* transform)
{
    Transform = transform;
}

void mmArrow::Reset()
{
    Interest = nullptr;
    asNode::Reset();
}

void mmArrow::SetInterest(Vector3* interest)
{
    Interest = interest;
}

// The original passes the addresses of three zeroed dwords here - empty strings the
// messages are filled into later.
static LocString HudUpperText {};
static LocString HudLowerText {};
static LocString HudChatText {};

mmHUD::mmHUD()
{
    WpHud = nullptr;
    CircuitHud = nullptr;
    CrHud = nullptr;
    TimerCountDown = 0;

    AddChild(&HudElements);
    HudElements.AddChild(&DashView);
    HudElements.AddChild(&ExternalView);

    NumberFont.LoadFont(LOC_STR(MM_IDS_94), 24, 0xFFFFFF);

    PositionFont = mmText::CreateLocFont(LOC_STRING(MM_IDS_95), Pipe()->GetWidth());

    TimerParts[2] = 10;
    TimerParts[5] = 10;
    TimerY = 0;
    WaypointDist = 0;
    ShowTimer = 1;
    field_B4C = 8;

    HudElements.AddChild(&Arrow);

    UpperMessage.Init(0.1f, 0.8f, 0.8f, 0.075f, 1, 1);
    LowerMessage.Init(0.1f, 0.875f, 0.8f, 0.075f, 1, 1);
    ChatMessages.Init(0.0f, 0.65f, 0.33f, 0.25f, 5, 1);

    ShowMessageAtTop = false;

    MessageFont = mmText::CreateLocFont(LOC_STRING(MM_IDS_96), Pipe()->GetWidth());
    UpperMessage.AddText(MessageFont, &HudUpperText, 3, 0.0f, 0.0f);
    LowerMessage.AddText(MessageFont, &HudLowerText, 3, 0.0f, 0.0f);

    ChatFont = mmText::CreateLocFont(LOC_STRING(MM_IDS_97), Pipe()->GetWidth());

    // Five chat lines, one twentieth of the screen apart
    for (i32 i = 0; i < 5; ++i)
        ChatMessages.AddText(ChatFont, &HudChatText, 0, 0.0f, static_cast<f32>(i) * 0.05f);

    AddChild(&UpperMessage);
    AddChild(&LowerMessage);
    AddChild(&ChatMessages);

    LowerMessage.ClearNodeFlag(NODE_FLAG_ACTIVE);
    ChatMessages.ClearNodeFlag(NODE_FLAG_ACTIVE);

    if (MMSTATE.NetworkStatus)
    {
        AlertSound = new AudSound(AudSound::Get2DFlags(), 1, -1);

        static char alert_name[] = "Carhorn1double";
        AlertSound->Load(alert_name, 0);
        AlertSound->SetPriority(23);
        AlertSound->SetVolume(0.85f, -1);
    }

    PositionText.Init(0.2f, 0.25f, 0.75f, 0.075f, 1, 1);
    PositionText.AddText(PositionFont, LOC_TEXT("Position"), 0, 0.0f, 0.0f);
    HudElements.AddChild(&PositionText);

    TogglePositionDisplay(0);

    CDPlayer.Init(this);
    HudElements.AddChild(&CDPlayer);
}

void mmHUD::ActivateDash()
{
    MMSTATE.DashView = true;

    DashView.Activate();

    // The dash and the external view are mutually exclusive.
    ExternalView.DeactivateNode();
}

void mmHUD::DeactivateDash()
{
    MMSTATE.DashView = false;

    DashView.Deactivate();

    if (MMSTATE.ExternalView)
        ExternalView.ActivateNode();
}
