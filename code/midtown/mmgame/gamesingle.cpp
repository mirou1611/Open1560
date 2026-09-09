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

define_dummy_symbol(mmgame_gamesingle);

#include "gamesingle.h"

#include "agiworld/meshlight.h"
#include "arts7/lamp.h"
#include "data7/memstat.h"
#include "mmai/aiMap.h"
#include "mmai/aiaudiomanager.h"
#include "mmaudio/manager.h"
#include "mmaudio/sound.h"
#include "mmaudio/mmvoicecommentary.h"
#include "mmcity/cullcity.h"
#include "mmcityinfo/racedata.h"
#include "mmcityinfo/state.h"
#include "mminput/input.h"
#include "player.h"
#include "popup.h"
#include "waypoints.h"

#ifndef ARTS_STANDALONE
i32 mmGameSingle::OppNumCheck[MaxOpponents] {};
i16 mmGameSingle::OppFinishPositions[MaxOpponents] {};
#endif

b32 mmGameSingle::Init()
{
    if (!mmGame::Init())
        return false;

    RaceData = arnew mmRaceData();
    RaceData->Load(arts_formatf<128>("%s\\mmracedata", RaceDir));

    GameState = 0;

#ifndef ARTS_STANDALONE
    std::memset(&OldOppNumCheck, 0xAA, sizeof(OldOppNumCheck));
    std::memset(&OldOppFinishPositions, 0xAA, sizeof(OldOppFinishPositions));
#endif

    for (usize i = 0; i < MaxOpponents; ++i)
    {
        OppNumCheck[i] = 1;
        OppFinishPositions[i] = 0;
    }

    NumFinished = 0;

    AddChild(Player.get());

    if (HasAIMap)
        AddChild(&AIMAP);

    AddChild(pCullCity.get());
    AddChild(&Icons);

    if (Waypoints)
        AddChild(Waypoints);

    AddChild(&Player->HudMap);
    AddChild(&Player->Hud);

    LampCS->AddChild(Lamp.get());
    AddChild(LampCS.get());

    AddChild(&FooBar);
    AddChild(Popup.get());

    return true;
}

void mmGameSingle::Reset()
{
    AudMgr()->Reset();
    InWater = false;

    if (MMSTATE.HasMidtownCD)
        AudMgr()->StopCD();

    if (AiAudMgr())
        AiAudMgr()->LoadCopVoice();

    GameInput()->Reset();

    GameStateWait = 0.0f;
    GameState = 0;

    for (usize i = 0; i < MaxOpponents; ++i)
    {
        OppNumCheck[i] = 1;
        OppFinishPositions[i] = 0;
    }

    NumFinished = 0;

    if (MMSTATE.GameMode == mmGameMode::Checkpoint)
        DisableRacers();

    mmGame::Reset();

    if (MMSTATE.GameMode == mmGameMode::Cruise)
    {
        if (VoiceCommentary)
            VoiceCommentary->PlayRoam();
    }
    else
    {
        Player->SetPreRaceCam();

        if (MMSTATE.HasMidtownCD)
            AudMgr()->PlayCDTrack(GetCDTrack(4), 1);

        if (VoiceCommentary)
            VoiceCommentary->PlayPreRace();

        Player->Hud.StopTimers();
    }
}

void mmGameSingle::UpdateDebugKeyInput(i32 /*arg1*/)
{}
// The original also builds and immediately destroys a temporary mmGame on the stack;
// that is a compiler artifact, not behaviour. Everything else it does - InWater = 0 and
// the FooBar node - is covered by the in-class initializers.
mmGameSingle::mmGameSingle() = default;

void mmGameSingle::InitMyPlayer()
{
    Player = arnew mmPlayer();
}

// Defining this is what makes the compiler emit mmGameSingle's vtable. Without a key
// function the class has none, gen_stubs.py synthesizes one, and every slot in it -
// including virtuals that are implemented in C++, like InitMyPlayer - points at
// ArtsVirtualStub instead.
mmGameSingle::~mmGameSingle()
{
    {
        ARTS_MEM_STAT("mmGameSingle Destructor");

        delete Waypoints;
        Waypoints = nullptr;

        RaceData = nullptr;
    }

    StartSounds = nullptr;
}

void mmGameSingle::Update()
{
    agiBeginCones();

    mmGame::Update();
}

mmWaypoints* mmGameSingle::GetWaypoints()
{
    return Waypoints;
}

void mmGameSingle::InitGameObjects()
{
    Waypoints = nullptr;

    if (MMSTATE.GameMode == mmGameMode::Checkpoint)
    {
        Waypoints = new mmWaypoints();

        if (VoiceCommentary)
            Waypoints->VoiceCommentary = VoiceCommentary.get();

        // The twelve races wrap, so event 12 is race 0 of the second set.
        i32 event = MMSTATE.EventId;

        if (event >= 12)
            event -= 12;

        char path[64];
        arts_sprintf(path, "%s\\race%d", RaceDir, event);

        if (!Waypoints->Init(Player.get(), path, 2, 0, 1, 0))
        {
            // No race file. Fall back to the city's own waypoints, and if those are
            // missing too there is no race to run and this becomes a cruise.
            if (Waypoints->Init(Player.get(), MapName, 2, 0, 1, 0))
            {
                MMSTATE.EventId = 0;
            }
            else
            {
                delete Waypoints;
                Waypoints = nullptr;

                MMSTATE.GameMode = mmGameMode::Cruise;
            }
        }
    }

    Player->HudMap.SetWaypoints(Waypoints);

    if (MMSTATE.GameMode == mmGameMode::Checkpoint)
    {
        // A race starts on its own grid; a cruise starts wherever the city says.
        Waypoints->GetStart(ResetPosition);

        RespawnPosition = Waypoints->GetStartAngle() * -ARTS_DEG_TO_RAD;
    }

    // Where the player begins, and which way they face.
    Player->Car.Sim.SetResetPos(ResetPosition);
    Player->Car.Sim.ResetRotation = RespawnPosition;
    Player->Car.Reset();

#ifndef ARTS_NO_AUDIO
    // PORT SHIM: mmaudio is excluded from this build and AudSound's constructor is a
    // stub, so building these would leave an object with a garbage vtable in
    // StartSounds - which mmGame::Update calls straight into.
    if (MMSTATE.GameMode != mmGameMode::Cruise)
    {
        StartSounds = arnew AudSound(AudSound::Get2DFlags(), 6, -1);

        StartSounds->Load("Startracelow", 0);
        StartSounds->SetVolume(0.9f, -1);
        StartSounds->Load("Startracehigh", 1);
        StartSounds->SetVolume(0.9f, -1);
        StartSounds->Load("Endofracetag", 2);
        StartSounds->SetVolume(0.925f, -1);
        StartSounds->Load("Youlose", 3);
        StartSounds->SetVolume(0.925f, -1);
        StartSounds->Load("Damgelose", 4);
        StartSounds->SetVolume(0.925f, -1);
        StartSounds->Load("Messagenote", 5);
        StartSounds->SetVolume(0.9f, -1);
    }
    else
    {
        StartSounds = arnew AudSound(AudSound::Get2DFlags(), 1, -1);

        StartSounds->Load("Messagenote", 0);
    }

    StartSounds->SetPriority(23);
#endif
}
