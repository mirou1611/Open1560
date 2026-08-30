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

define_dummy_symbol(mmcity_cullcity);

#include "cullcity.h"

#include "agi/dlptmpl.h"
#include "agi/getdlp.h"
#include "agi/pipeline.h"
#include "agi/rsys.h"
#include "agi/texdef.h"
#include "agisw/swrend.h"
#include "agiworld/getmesh.h"
#include "agiworld/meshlight.h"
#include "agiworld/meshrend.h"
#include "agiworld/meshset.h"
#include "agi/rsys.h"
#include "agisw/swrend.h"
#include "mmcity/anim.h"
#include "pcwindis/setupdata.h"

#include <cmath>
#include "agiworld/quality.h"
#include "mmbangers/active.h"
#include "agiworld/texsheet.h"
#include "arts7/cullmgr.h"
#include "arts7/sim.h"
#include "data7/memstat.h"
#include "data7/quitf.h"
#include "localize/localize.h"
#include "mmcityinfo/state.h"
#include "mmdyna/bndtmpl.h"
#include "mmphysics/phys.h"
#include "stream/problems.h"
#include "stream/stream.h"

#include "inst.h"
#include "loader.h"
#include "renderweb.h"

static constexpr i32 MAX_HIT_BANGERS = 80;
static constexpr i32 MAX_PARTICLES = 100;
static constexpr i32 CHICAGO_CELL_RUNWAY = 35;

// clang-format off
t_mmEnvSetup mmEnvSetup[4][4] {
    { // Morning
        {"sky_mc", "refl_mc", "shadmap_morneve",  0.002, 0.01,   0.0, 0x000000, 1.00, 1.00, 0.50},
        {"sky_mf", "refl_mf", "shadmap_day",      0.010, 0.01, 200.0, 0xBEB496, 1.00, 1.00, 0.75},
        {"sky_mr", "refl_mr", "shadmap_rainsnow", 0.000, 0.01, 500.0, 0x000000, 0.00, 0.00, 0.00},
        {"sky_ms", "refl_mr", "shadmap_rainsnow", 0.000, 0.01, 200.0, 0xFFFFFF, 0.00, 0.00, 0.00},
    },
    { // Noon
        {"sky_nc", "refl_nc", "shadmap_day",      0.010, 0.01,   0.0, 0x000000, 0.25, 0.70, 1.00},
        {"sky_nf", "refl_nf", "shadmap_day",      0.010, 0.01, 200.0, 0x82B4C3, 0.50, 0.85, 1.00},
        {"sky_nr", "refl_nr", "shadmap_rainsnow", 0.000, 0.01, 500.0, 0x000000, 0.50, 0.50, 0.50},
        {"sky_ns", "refl_nr", "shadmap_rainsnow", 0.000, 0.01, 200.0, 0xFFFFFF, 0.50, 0.50, 0.50},
    },
    { // Sunset
        {"sky_sc", "refl_sc", "shadmap_morneve",  0.002, 0.01,   0.0, 0x000000, 1.00, 0.75, 0.50},
        {"sky_sf", "refl_sf", "shadmap_day",      0.010, 0.01, 200.0, 0x5A283C, 1.00, 0.80, 0.65},
        {"sky_sr", "refl_sr", "shadmap_rainsnow", 0.000, 0.01, 500.0, 0x000000, 0.60, 0.50, 0.40},
        {"sky_ss", "refl_sr", "shadmap_rainsnow", 0.000, 0.01, 200.0, 0xFFFFFF, 0.60, 0.50, 0.40},
    },
    { // Night/Dark
        {"sky_dc", "refl_dc", "shadmap_nite",     0.000, 0.01,   0.0, 0x000000, 0.0, 0.0, 0.1},
        {"sky_df", "refl_df", "shadmap_nite",     0.000, 0.01, 300.0, 0x141414, 10.0f/255.0f, 10.0f/255.0f, 10.0f/255.0f},
        {"sky_dr", "refl_dr", "shadmap_nite",     0.000, 0.01, 500.0, 0x000000, 0.0, 0.0, 0.0},
        {"sky_ds", "refl_dr", "shadmap_nite",     0.000, 0.01, 200.0, 0xFFFFFF, 0.0, 0.0, 0.0},
    },
};
// clang-format on

#ifdef ARTS_DEV_BUILD
void mmRunwayLight::AddWidgets(Bank* /*arg1*/)
{}
#endif

void ARTS_FASTCALL mmRunwayLight::FromMatrix(const Matrix34& /*arg1*/)
{}

// ?fix_fill1@@YAXXZ
ARTS_IMPORT /*static*/ void fix_fill1();

// ?fix_fill2@@YAXXZ
ARTS_IMPORT /*static*/ void fix_fill2();

// ?fix_sun@@YAXXZ
ARTS_IMPORT /*static*/ void fix_sun();

// ?parseARGB@@YAXAAI@Z
ARTS_IMPORT /*static*/ void parseARGB(u32& arg1);

// ?parseFloat@@YAXAAM@Z
ARTS_IMPORT /*static*/ void parseFloat(f32& arg1);

// ?parseRGB@@YAXAAI@Z
ARTS_IMPORT /*static*/ void parseRGB(u32& arg1);

// ?parseVector3@@YAXAAVVector3@@@Z
ARTS_IMPORT /*static*/ void parseVector3(Vector3& arg1);

// ?ShowRenderStats@@YAXXZ
ARTS_IMPORT /*static*/ void ShowRenderStats();

void mmCullCity::Cull()
{
    if (FogEnd == 0.0f || agiCurState.GetDrawMode() == agiDrawDepth)
    {
        agiCurState.SetFogMode(agiFogMode::None);
        agiMeshSet::SetFog(0.0, 0);
    }
    else
    {
        agiCurState.SetFogMode(UsePixelFog ? agiFogMode::Pixel : agiFogMode::Vertex);
        agiCurState.SetFogColor(SkyColor | swIsInterlaced);

        f32 fog_end = FogEnd;

        if (!agiCurState.GetSoftwareRendering())
            fog_end = std::min(fog_end, agiRQ.FarClip);

        if (UsePixelFog)
        {
            agiMeshSet::SetFog(0.0f, 0);
            agiCurState.SetFogStart(1.0f);
            agiCurState.SetFogEnd(fog_end);
        }
        else if (UseFogEnd2)
        {
            agiMeshSet::SetFog(FogEnd2, UseFogEnd2);
        }
        else
        {
            agiMeshSet::SetFog(fog_end, 0);
        }
    }
}

static agiMeshCardVertex RainMeshCard[4] {
    {-0.1f, 2.0f, 0.4f, 1.0f},
    {-0.1f, -2.0f, 0.4f, 0.0f},
    {0.1f, -2.0f, 0.6f, 0.0f},
    {0.1f, 2.0f, 0.6f, 1.0f},
};

void mmCullCity::Init(char* name, asCamera* camera)
{
    IsSnowing = MMSTATE.Weather == mmWeather::Snow;
    CityName = name;

    TEXSHEET.SetUseAlternate((MMSTATE.TimeOfDay == mmTimeOfDay::Sunset) || (MMSTATE.TimeOfDay == mmTimeOfDay::Night));

    switch (MMSTATE.Weather)
    {
        case mmWeather::Rain: TextureSuffix = "_fall"_xconst; break;
        case mmWeather::Snow: TextureSuffix = "_win"_xconst; break;
        default: TextureSuffix = nullptr; break;
    }

#ifdef ARTS_DEV_BUILD
    StaticLog = as_raw arts_fopen(arts_formatf<64>("%s_static.csv", name), "w");
#endif

    Loader()->BeginTask(LOC_STRING(MM_IDS_LOADING_CITY_LAYOUT));

    DLPTemplate* city_dlp = nullptr;
    DLPTemplate* lm_dlp = nullptr;

    if (DevelopmentMode)
    {
        city_dlp = GetDLPTemplate(arts_formatf<128>("%scity", name));
        lm_dlp = GetDLPTemplate(arts_formatf<128>("%slm", name));
    }

    ARTS_MEM_STAT("mmCullCity::Init");

    InitProblems();

    Camera = camera;
    AddChild(&BangerDataManager);
    AddChild(&BangerActiveManager);
    AddChild(&BangerManager);
    AddChild(&field_34AF0);
    AddChild(&PHYS);
    AddChild(&RenderWeb);
    AddChild(&Particles);

    Particles.Init(
        MAX_PARTICLES, 4, 4, 4, (MMSTATE.Weather == mmWeather::Rain) ? RainMeshCard : agiMeshSet::DefaultQuad);

    SnowBirthRule.SetName("SnowRule");
    AddChild(&SnowBirthRule);
    SnowBirthRule.Load();

    RainBirthRule.SetName("RainRule");
    AddChild(&RainBirthRule);
    RainBirthRule.Load();

    BirthRule = nullptr;

    Sky.Init("mmsky"_xconst);
    Loader()->EndTask();

    {
        ARTS_MEM_STAT("mmCullCity.WEB");
        RenderWeb.Load(name, 1);
    }

    HitIdBound = RenderWeb.HitIdBound;
    BuildingChain.Init(RenderWeb.MaxCells);
    ObjectsChain.Init(RenderWeb.MaxCells);
    ShadowChain.Init(RenderWeb.MaxCells);

    InitObjectDetail();

    StartOfBangers = static_cast<mmInstance*>(mmInstanceHeap.GetHead());

    LoadBangers(name);

    switch (MMSTATE.GameMode)
    {
        case mmGameMode::Cruise: LoadBangers(arts_formatf<64>("%s_roam", name)); break;
        case mmGameMode::Checkpoint: LoadBangers(arts_formatf<64>("%s_r%d", name, MMSTATE.EventId)); break;
        case mmGameMode::CnR: LoadBangers(arts_formatf<64>("%s_cops", name)); break;
        case mmGameMode::Circuit: LoadBangers(arts_formatf<64>("%s_c%d", name, MMSTATE.EventId)); break;
        case mmGameMode::Blitz: LoadBangers(arts_formatf<64>("%s_b%d", name, MMSTATE.EventId)); break;
    }

    EndOfBangers = new mmYInstance();

    LoadFacades(name);

    EnableSphereCull = CHICAGO;

    if (CHICAGO)
    {
        Vector3 pos_1 = {995.782, 0.173, 1188.804};
        Vector3 pos_2 = {995.782, 0.173, 742.411};
        Vector3 pos_3 = {1015.383, 0.173, 1188.805};
        Vector3 pos_4 = {1015.383, 0.173, 742.411};

        BuildingChain.Parent(new mmRunwayLight("fxltglow"_xconst, pos_1, pos_2), CHICAGO_CELL_RUNWAY);
        BuildingChain.Parent(new mmRunwayLight("fxltglow"_xconst, pos_3, pos_4), CHICAGO_CELL_RUNWAY);
    }

    InitTimeOfDayAndWeather();

    if (IsSnowing)
        InitSnowTextures();

    BangerMgr()->Init(MAX_HIT_BANGERS);

    if (city_dlp && city_dlp->Release())
        Errorf("Someone is still holding a ref on the city's template");

    if (lm_dlp)
        lm_dlp->Release();

#ifdef ARTS_DEV_BUILD
    CullMgr()->AddPage(ShowRenderStats);

    if (StaticLog)
    {
        delete StaticLog;
    }
#endif
}

void mmCullCity::ReparentObject(mmInstance* inst)
{
    if (asPortalCell* cell = RenderWeb.GetStartCell(inst->GetPos(), 0, nullptr))
    {
        if (inst->ChainId == -1)
        {
            ObjectsChain.Parent(inst, cell->CellIndex);
        }
        else if (i16 cell_id = cell->CellIndex; cell_id != inst->ChainId)
        {
            ObjectsChain.Reparent(inst, cell_id);
        }
    }
}

void mmCullCity::Reset()
{
    asNode::Reset();

    // Directly iterating over the instance heep? EEK!
    for (mmInstance* inst = StartOfBangers; inst != EndOfBangers;
        inst = reinterpret_cast<mmInstance*>(reinterpret_cast<char*>(inst) + inst->SizeOf()))
    {
        if (inst->TestFlags(INST_FLAG_40) && !inst->TestFlags(INST_FLAG_UNHIT_BANGER) &&
            inst->SizeOf() == sizeof(mmUnhitBangerInstance))
        {
            inst->Reset();
        }
    }

    if (IsSnowing)
    {
        for (i32 i = 0; i < SnowTextureCount; ++i)
        {
            if (agiTexDef* tex = SnowTexturesDst[i])
            {
                tex->EndGfx();
                tex->SafeBeginGfx();
            }
        }

        if (MMSTATE.TimeOfDay == mmTimeOfDay::Night)
        {
            for (i32 i = 0; i < 10000; ++i)
                UpdateSnowTextures();
        }

        SnowFrictionStartTime = Sim()->GetElapsed();
    }

    Pipe()->Defragment();
}

static mem::cmd_param PARAM_conelighter {"conelighter", "Use agiConeLighter"};

void fix_lighting()
{
    if (PARAM_conelighter)
    {
        mmInstance::DynamicLighter = agiConeLighter;
        mmInstance::StaticLighter = agiConeLighter;
        return;
    }

    switch (agiRQ.LightQuality)
    {
        case AGI_QUALITY_LOW:
            mmInstance::StaticLighter = nullptr;
            mmInstance::DynamicLighter = nullptr;
            break;
        case AGI_QUALITY_MEDIUM:
            mmInstance::DynamicLighter = agiMeshLighterQuarter;
            mmInstance::StaticLighter = agiMeshLighterQuarter;
            break;
        case AGI_QUALITY_HIGH:
            mmInstance::DynamicLighter = agiMeshLighterQuarter;
            mmInstance::StaticLighter = agiMeshLighterTriple;
            break;
        case AGI_QUALITY_VERY_HIGH:
            mmInstance::DynamicLighter = agiMeshLighterTriple;
            mmInstance::StaticLighter = agiMeshLighterTriple;
            break;

        default: Quitf("agiRQ.LightQuality = %d", agiRQ.LightQuality);
    }

    if (agiCurState.GetSoftwareRendering())
        mmInstance::StaticLighter = nullptr;
}
mmCullCity::mmCullCity()
{
    SetNodeFlag(NODE_FLAG_UPDATE_PAUSED);

    if (Instance)
        Quitf("Already have a CullCity");

    Instance = this;

    mmInstanceHeap.Init(0xB9000);

    WeatherFriction = 1.0f;
    RainFriction = 0.75f;
    SnowFrictionMax = 0.75f;
    SnowFrictionMin = 0.5f;
    SnowFrictionBlendSpeed = 60.0f;
    SnowFrictionStartTime = 0.0f;

    // Only the translation of the environment matrix is initialised, as in the original
    EnvMatrix.m3 = {0.0f, 0.0f, 0.0f};

    if (agiCurState.GetSoftwareRendering())
    {
        agiRQ.EnvMap = false;
        agiRQ.SphMap = false;

        agiMeshSet::DepthScale = 0.495f;
    }
    else
    {
        agiMeshSet::DepthScale = 0.499f;
    }

    ShadowZBias = 0.005f;
}

// The card the glow is drawn on: a unit quad, two units wide, with its texture
// coordinates covering the right half of the sheet
static agiMeshCardVertex RunwayLightCard[4] {
    {-1.0f, 0.0f, 0.0f, 0.5f},
    {1.0f, 0.0f, 1.0f, 0.5f},
    {1.0f, 1.0f, 1.0f, 1.0f},
    {-1.0f, 1.0f, 0.0f, 1.0f},
};

mmRunwayLight::mmRunwayLight(aconst char* arg1, Vector3& arg2, Vector3& arg3)
{
    Start = arg2;
    End = arg3;

    f32 length = Start.Dist(End);

    Scale = length * 0.5f;

    // One light every fifteen units, plus the one at the near end
    f32 count = std::floor(length * (1.0f / 15.0f)) + 1.0f;

    Step = (End - Start) * (1.0f / count);

    Texture = GetPackedTexture(arg1, 0).release();

    Center = (Start + End) * 0.5f;
    NumLights = static_cast<i32>(count);

    MeshCard.Init(ARTS_SSIZE32(RunwayLightCard), RunwayLightCard, 1, 1, 1);

    SetFlags(INST_FLAG_SHADOW | INST_FLAG_2000);

    InitMeshes("bluelight"_xconst, 0, nullptr, nullptr);

    // The original divides unconditionally. It could: its InitMeshes always found the
    // mesh. Ours cannot while mmInstance::GetMeshSetSet is still a stub, and MeshIndex
    // stays zero, so guard it rather than index the table at -1.
    if (agiMeshSet* mesh = GetMeshSet(INST_LOD_HIGH, 0))
        Scale /= mesh->Radius;
}

void mmCullCity::InitObjectDetail()
{
    // One multiplier per Object Detail setting, lowest first.
    static constexpr f32 ParticleMultiplierTable[] {0.25f, 0.5f, 0.75f, 1.0f};

    EnableSubClip = 0;
    ParticleMultiplier = ParticleMultiplierTable[agiRQ.TerrainQuality];

    fix_clip();
}

// Both city object files are a count followed by count records, each a fixed-size
// binary header immediately followed by a NUL-terminated name. The name is not
// length-prefixed and not padded, so it has to be read a byte at a time - which is
// exactly what the original does, with Stream::GetCh inlined.
static void ReadInstanceName(Stream* stream, char* out)
{
    char* p = out;

    do
    {
        *p = static_cast<char>(stream->GetCh());
    } while (*p++);
}

void mmCullCity::LoadBangers(char* city_name)
{
    // Bangers: the loose props - cones, hydrants, parked cars, mailboxes.
    struct BangerRecord
    {
        u16 Flags;
        u16 Type;
        Vector3 Position;
        Vector3 Rotation;
    };

    static_assert(sizeof(BangerRecord) == 0x1C, "BangerRecord must match the file layout");

    BeginMemStat("mmCullCity bangers");
    Loader()->BeginTask(AngelReadString(0xD), 0.2f);

    char path[36];
    arts_sprintf(path, "city/%s.bng", city_name);

    if (Ptr<Stream> stream {arts_fopen(path, "r"_xconst)})
    {
        i32 count = 0;
        stream->Read(&count, sizeof(count));
        Displayf("***** %d bangers in city", count);

        for (i32 i = 0; i < count; ++i)
        {
            BangerRecord rec {};
            stream->Read(&rec, sizeof(rec));

            char name[64];
            ReadInstanceName(stream.get(), name);

            // Bangers carry no scale and no third vector; facades below do.
            AddInstance(rec.Flags, name, nullptr, rec.Type, &rec.Position, &rec.Rotation, nullptr, 0.0f);
        }
    }

    EndMemStat();
    Loader()->EndTask(0.34f);
}

void mmCullCity::LoadFacades(char* city_name)
{
    // Facades: the flat building fronts that make up most of what you see.
    struct FacadeRecord
    {
        u16 Flags;
        u16 Type;
        Vector3 Position;
        Vector3 Rotation;
        Vector3 Extents;
        f32 Scale;
    };

    static_assert(sizeof(FacadeRecord) == 0x2C, "FacadeRecord must match the file layout");

    BeginMemStat("mmCullCity facades");
    Loader()->BeginTask(AngelReadString(0xE), 0.0f);

    char path[36];
    arts_sprintf(path, "city/%s.fcd", city_name);

    if (Ptr<Stream> stream {arts_fopen(path, "r"_xconst)})
    {
        // No count message here, unlike the bangers.
        i32 count = 0;
        stream->Read(&count, sizeof(count));

        for (i32 i = 0; i < count; ++i)
        {
            FacadeRecord rec {};
            stream->Read(&rec, sizeof(rec));

            char name[64];
            ReadInstanceName(stream.get(), name);

            AddInstance(
                rec.Flags, name, nullptr, rec.Type, &rec.Position, &rec.Rotation, &rec.Extents, rec.Scale);
        }
    }

    EndMemStat();
    Loader()->EndTask(0.61f);
}

// This is mmCullCity's key function, and that matters as much as what it does.
//
// It was the class's first non-inline virtual and it lived in assembly, so no
// translation unit emitted mmCullCity's vtable and gen_stubs.py synthesized one -
// every slot pointing at ArtsVirtualStub. mmCullCity::Cull has been written in C++
// for some time and had never once been called. Defining the destructor here makes
// the compiler emit the real vtable.
mmCullCity::~mmCullCity()
{
    Displayf("%d bytes remaining in instance heap", mmInstanceHeap.GetFreeSize());

    mmInstanceHeap.Kill();

    mmInstance::ResetAll();
    TEXSHEET.Kill();

    Instance = nullptr;

    // Everything the original releases by hand here is owned by a member that releases
    // it anyway - HitIdBound is an Rc and CityName a ConstString, and calling Release
    // or arts_free on those as well would be a double free. The one thing with no owner
    // is this node's children.
    while (asNode* child = field_34AF0.GetChildNode())
        delete child;

    DumpProblems();
}

// The original binds this to a dev-build bank slider labelled "D3D Lod bias" and reads
// it back here. It has no name in the symbol table and nothing else touches it, so it
// stays file-local and at its default of zero.
static f32 D3DLodBias = 0.0f;

void mmCullCity::Update()
{
    if (!Sim()->IsFullUpdate())
    {
        // The city can only be culled once per frame, so an oversampled sub-step has
        // nothing to do here.
        Errorf("Hey!  Some hoser is oversampling.");
        return;
    }

    // Two quality settings the current renderer may not be able to honour.
    if (RenderWeb.Debug || debugTriMatch)
        agiRQ.TexturedSky = false;

    if (!dxiInfo[dxiRendererChoice].SmoothAlpha)
        agiRQ.EnvMap = false;

    // This is the line that puts the whole city in front of the cull manager. Without it
    // the manager reports zero cullables and nothing in the world is ever drawn.
    CullMgr()->DeclareCullable(this);

    if (!Sim()->IsPaused())
    {
        const f32 delta = Sim()->GetUpdateDelta();

        // Scroll the environment map across the world. The original writes the second
        // line as a subtraction of a negated constant; it is a positive drift twice as
        // fast as the first.
        EnvMatrix.m3.x += delta * EnvVel;
        EnvMatrix.m3.y += delta * EnvVel * 2.0f;

        mmRunwayLight::Phase += delta;

        if (MMSTATE.Weather == mmWeather::Snow)
        {
            // Swing the snow sideways so it does not fall straight down.
            Particles.SetWind({std::sin(Sim()->GetElapsed()) * 2.0f, 0.0f, 0.0f});

            UpdateSnowTextures();
        }

        // PORT SHIM: the null test on BirthRule. It is assigned by
        // mmCullCity::InitTimeOfDayAndWeather, which is still assembly, so there is no
        // birth rule to orient. Remove the test once that function is written.
        if (BirthRule && Particles.IsNodeActive())
        {
            // Weather is born in front of the camera, not at a fixed point in the world:
            // two metres up and ten ahead, rotated into the camera's frame.
            Vector3 spawn {0.0f, 2.0f, -10.0f};
            spawn.Dot(spawn, *Camera->GetCameraMatrix());

            BirthRule->Position = spawn;
        }

        mmAnimInstState::PreUpdate(delta);

        asNode::Update();
    }
    else
    {
        // Paused. Rain and snow are node children, so they are switched off across the
        // update rather than special-cased inside it.
        if (MMSTATE.Weather >= mmWeather::Rain)
            Particles.ClearNodeFlag(NODE_FLAG_ACTIVE);

        asNode::Update();

        if (MMSTATE.Weather >= mmWeather::Rain)
            Particles.SetNodeFlag(NODE_FLAG_ACTIVE);
    }

    if (Particles.IsNodeActive())
    {
        // Weather only draws outdoors, so it is registered for this frame unless the
        // camera is in a room flagged as interior.
        if (!(GetRoomFlags(GetHitId(Camera->GetCameraMatrix()->m3)) & 1))
        {
            // PATCH kept from the original port: the array is 64 entries and the original
            // does not check before writing.
            if (RenderWeb.PtxCount != ARTS_SIZE(RenderWeb.Particles))
                RenderWeb.Particles[RenderWeb.PtxCount++] = &Particles;
        }
    }

    agiCurState.SetLodBias(D3DLodBias);
}
