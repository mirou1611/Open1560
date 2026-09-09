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

define_dummy_symbol(mmcity_cellrend);

#include "cellrend.h"

#include "agi/texdef.h"
#include "agiworld/getmesh.h"
#include "agiworld/meshset.h"
#include "core/assert.h"
#include "data7/printer.h"
#include "mmcity/cullcity.h"
#include "mmcity/inst.h"
#include "mmcityinfo/state.h"
#include "vector7/geomath.h"
#include "stream/stream.h"

#include "agi/rsys.h"
#include "agi/viewport.h"
#include "agiworld/quality.h"
#include "agiworld/texsort.h"
#include "arts7/camera.h"
#include "arts7/sim.h"
#include "mmbangers/dof.h"
#include "mmcity/instchn.h"
#include "mmcity/portal.h"
#include "mmcity/renderweb.h"
#include "vector7/matrix34.h"

#include <cmath>
#include <cstdlib>
#include <cstring>

f32 ObjectMaxDist = 300.0f;

// ?BuildingMaxDist@@3MA
ARTS_EXPORT f32 BuildingMaxDist = 1000.0f;

f32 StaticTerrainLodTable[4][2] {
    {150.0f, 50.0f},
    {200.0f, 100.0f},
    {250.0f, 150.0f},
    {325.0f, 200.0f},
};

// ?LightDistances@@3PAMA
ARTS_EXPORT f32 LightDistances[4] {
    80.0f,
    160.0f,
    250.0f,
    350.0f,
};

void mmCellRenderer::Relight()
{}
// The original zeroes Drawbridge, Meshes, SlideData, VisitTagCount and VisitTags; the
// in-class initializers cover those and a few more.
mmCellRenderer::mmCellRenderer() = default;

// ?GetPolyInfo@@YAHPAVagiMeshSet@@@Z
i32 GetPolyInfo(agiMeshSet* mesh)
{
    // Triangle count, for the static-geometry log only. A surface is four indices;
    // a zero in the fourth means it is a triangle rather than a quad.
    if (!mesh || mesh->Resident <= 1 || mesh->IndicesCount == 0)
        return 0;

    i32 total = 0;

    for (u32 i = 0; i < (mesh->IndicesCount + 3) / 4; ++i)
        total += mesh->SurfaceIndices[i * 4 + 3] ? 2 : 1;

    return total;
}

void mmCellRenderer::Init(aconst char* city_name, i32 index, i32 mesh_flags, i32 room_flags,
    i32 visit_tag_count, i32* visit_tags)
{
    Index = static_cast<i16>(index);

    // Chicago gets a set of hand-written per-cell fixups. They are keyed on raw cell
    // numbers, so they are simply transcribed - there is no rule behind them beyond
    // "this cell in this city needs this bit".
    if (CHICAGO)
    {
        if (index < 200 || index == 0x201 || index == 0x286)
            room_flags |= 0x20;

        if (room_flags & 0x10)
            room_flags |= 0x40;

        if (index == 0x24)
            room_flags |= 0x40;

        if (room_flags & 0x4)
            room_flags |= 0x20;

        if (index == 5)
            room_flags |= 0x80;

        if (index == 0x563 || index == 0x548)
            room_flags |= 0x100;
    }

    RoomFlags = static_cast<i16>(room_flags);
    VisitTagCount = static_cast<i16>(visit_tag_count);

    // The caller hands over i32 tags; they are stored narrowed to u16.
    VisitTags = new u16[VisitTagCount];

    for (i32 i = 0; i < VisitTagCount; ++i)
        VisitTags[i] = static_cast<u16>(visit_tags[i]);

    // Eight mesh slots, two passes of four levels of detail. Pass one is the cell
    // itself (_H / _M / _L / _A), pass two is the second layer (_H2 / _M2 / _L2 / _A2).
    // A cell with the 0x10 bit has no LODs at all and uses the bare CULLnn name in the
    // high slot.
    if (mesh_flags & 0x10)
        Meshes[2] = GetMeshSet(city_name, formatf("CULL%02d", index), nullptr, 7);
    else if (mesh_flags & 0x8)
        Meshes[2] = GetMeshSet(city_name, formatf("CULL%02d_H", index), nullptr, 7);

    if (mesh_flags & 0x2)
        Meshes[0] = GetMeshSet(city_name, formatf("CULL%02d_L", index), nullptr, 7);

    if (mesh_flags & 0x4)
        Meshes[1] = GetMeshSet(city_name, formatf("CULL%02d_M", index), nullptr, 7);

    if (mesh_flags & 0x1)
        Meshes[3] = GetMeshSet(city_name, formatf("CULL%02d_A", index), nullptr, 7);

    // Which passes this cell needs drawing in, counted globally for the stats readout.
    if ((mesh_flags & 0x1F) && (mesh_flags & 0x1E0))
        ++CRPassBoth;
    else if (mesh_flags & 0x1F)
        ++CRPass1Only;
    else if (mesh_flags & 0x1E0)
        ++CRPass3Only;

    if (mesh_flags & 0x100)
        Meshes[6] = GetMeshSet(city_name, formatf("CULL%02d_H2", index), nullptr, 7);

    if (mesh_flags & 0x40)
        Meshes[4] = GetMeshSet(city_name, formatf("CULL%02d_L2", index), nullptr, 7);

    if (mesh_flags & 0x80)
        Meshes[5] = GetMeshSet(city_name, formatf("CULL%02d_M2", index), nullptr, 7);

    if (mesh_flags & 0x20)
        Meshes[7] = GetMeshSet(city_name, formatf("CULL%02d_A2", index), nullptr, 7);

    // A cell with no high-detail geometry in either pass has nothing to draw and is
    // fatal - the city file and the mesh archive disagree.
    if (!Meshes[2] && !Meshes[7] && !Meshes[6])
    {
        Displayf("Flags nlod=%d h=%d m=%d l=%d a=%d h2=%d m2=%d l2=%d a2=%d", mesh_flags & 0x10,
            mesh_flags & 0x8, mesh_flags & 0x4, mesh_flags & 0x2, mesh_flags & 0x1, mesh_flags & 0x100,
            mesh_flags & 0x80, mesh_flags & 0x40, mesh_flags & 0x20);

        Quitf("Group CULL%02d (or _H) is missing from city '%s'", index, city_name);
    }

#ifdef ARTS_DEV_BUILD
    // The static-geometry log is a dev-build artifact; StaticLog does not exist otherwise.
    if (StaticLog)
    {
        arts_fprintf(StaticLog, "%d,%d,%d,%d,%d,%d,%d,%d,%d\n", index, GetPolyInfo(Meshes[0]),
            GetPolyInfo(Meshes[1]), GetPolyInfo(Meshes[2]), GetPolyInfo(Meshes[3]), GetPolyInfo(Meshes[4]),
            GetPolyInfo(Meshes[5]), GetPolyInfo(Meshes[6]), GetPolyInfo(Meshes[7]));
    }
#endif

    // Fill each pass's LOD chain downwards from whatever loaded, so a cell that only
    // shipped a high LOD still draws at distance.
    if (!Meshes[1])
        Meshes[1] = Meshes[2];

    if (!Meshes[0])
        Meshes[0] = Meshes[1];

    if (!Meshes[5])
        Meshes[5] = Meshes[6];

    if (!Meshes[4])
        Meshes[4] = Meshes[5];

    // The cell's bounding sphere comes from whichever mesh actually exists.
    agiMeshSet* bounds = Meshes[2];

    if (!bounds)
        bounds = Meshes[7];

    if (!bounds)
        bounds = Meshes[6];

    GetBoundInfo(bounds->VertexCount, bounds->Vertices, nullptr, nullptr, &CellCenter, &CellMagnitude);

    // The rest is the water surface. A cell flagged 0x4 may have a T_WATER texture in
    // its second-pass alpha mesh; if so, SlideData records which adjuncts belong to it
    // so the renderer can animate them.
    agiMeshSet* water = Meshes[7];

    if (!water || !(RoomFlags & 0x4))
        return;

    water->MakeResident();

    i32 water_texture = 0;

    for (i32 i = 1; i <= water->TextureCount; ++i)
    {
        agiTexDef* tex = water->Textures[0][i];

        if (tex && !std::strncmp(tex->Tex.Name, "T_WATER", 7))
            water_texture = i;
    }

    if (water_texture)
    {
        // The original puts both of these on the stack with alloca. AdjunctCount is
        // bounded by the mesh format, but not by anything small, so they go on the heap
        // here rather than risking the stack.
        u8* used = new u8[water->AdjunctCount] {};
        i32* list = new i32[water->AdjunctCount];

        for (u32 s = 0; s < water->SurfaceCount; ++s)
        {
            if (water->TextureIndices[s] != water_texture)
                continue;

            const u16* corners = &water->SurfaceIndices[s * 4];

            used[corners[0]] = 1;
            used[corners[1]] = 1;
            used[corners[2]] = 1;

            // A zero fourth corner means the surface is a triangle.
            if (corners[3])
                used[corners[3]] = 1;
        }

        i32 count = 0;

        for (u32 i = 0; i < water->AdjunctCount; ++i)
        {
            if (used[i])
                list[count++] = static_cast<i32>(i);
        }

        ArAssert(list[0] < 256, "list[0] < 256");

        // Stored as a u16 count followed by one byte per adjunct, each the gap from the
        // previous one - which is why both the first index and every gap have to fit in
        // a byte.
        SlideData = new u8[count + 2];
        SlideData[0] = static_cast<u8>(count);
        SlideData[1] = static_cast<u8>(count >> 8);

        for (i32 i = 0, previous = 0; i < count; ++i)
        {
            ArAssert(static_cast<u32>(list[i]) < water->AdjunctCount, "list[i] < M->AdjunctCount");
            ArAssert(list[i] - previous < 256, "list[i] - previous < 256");

            SlideData[i + 2] = static_cast<u8>(list[i] - previous);
            previous = list[i];
        }

        delete[] list;
        delete[] used;
    }

    water->Unlock();
}

void mmCellRenderer::Cull(b32 sub_cull)
{
    Viewport()->SetWorld(IDENTITY);

    Matrix34& camera_matrix = *CullCity()->Camera->GetCameraMatrix();

    // The bounding sphere is worked out the first time the cell is drawn, from
    // whichever of its meshes loaded - the same choice mmCellRenderer::Init makes.
    // If that mesh is not paged in there is nothing to measure and nothing to draw,
    // so ask for it and come back next frame.
    if (CellMagnitude == 0.0f)
    {
        agiMeshSet* bounds = Meshes[2];

        if (!bounds)
            bounds = Meshes[7];

        if (!bounds)
            bounds = Meshes[6];

        if (!bounds->LockIfResident())
        {
            bounds->PageIn();
            return;
        }

        GetBoundInfo(static_cast<i32>(bounds->VertexCount), bounds->Vertices, nullptr, nullptr, &CellCenter,
            &CellMagnitude);

        bounds->Unlock();
    }

    // Distance to the near edge of the cell, and the level of detail that earns.
    // Slot 0 is the _L mesh and slot 2 the _H, so far away is lod 0.
    const f32 distance = camera_matrix.m3.Dist(CellCenter) - CellMagnitude;
    const f32* lod_table = StaticTerrainLodTable[agiRQ.TerrainQuality];

    i32 lod = (distance > lod_table[0]) ? 0 : ((distance > lod_table[1]) ? 1 : 2);

    const f32 old_fog = agiMeshSet::FogValue;

    // The terrain's environment map is the city's shadow map texture, scrolled by
    // EnvMatrix - mmCullCity::Update is what animates that matrix.
    agiTexDef* env_map = CullCity()->ShadowMap;
    Matrix34& env_matrix = CullCity()->EnvMatrix;

    if (RoomFlags & ROOM_FLAG_80)
        agiMeshSet::FogValue = 0.25f;

    if (asRenderWeb::PassMask & RENDER_PASS_TERRAIN)
    {
        // The software rasterizer skips Z testing on terrain unless the cell insists,
        // which is what ROOM_FLAG_20 is for.
        agiCurState.SetZEnable(!agiCurState.GetSoftwareRendering() || (RoomFlags & ROOM_FLAG_20));
        agiCurState.SetZWrite(ZWRITE != 0);

        // Water. SlideData is a run of byte deltas over the vertex index list, and
        // each vertex it names has its U coordinate pushed along by the clock - that
        // is the whole of the flowing-water effect.
        if (EnableSlide && !EnableBinaryFileMapping && SlideData)
        {
            agiMeshSet* water = Meshes[7];

            if (water->LockIfResident())
            {
                const f32 phase = static_cast<f32>(std::fmod(Sim()->GetElapsed() * -0.1f, 1.0));

                const u8* slide = SlideData;
                const i32 count = slide[0] | (slide[1] << 8);

                slide += 2;

                for (i32 i = 0, index = 0; i < count; ++i)
                {
                    index += *slide++;

                    water->TexCoords[index].x = phase - (water->Vertices[water->VertexIndices[index]].x * -0.1f);
                }

                water->Unlock();
            }
            else
            {
                water->PageIn();
            }
        }

        TexSorter()->Cull(1);

        const i32 tris_before = agiTexSorter::TotalTris;

        if (Meshes[7] && !Sim()->IsDebugDrawEnabled())
        {
            if (lod && !(RoomFlags & ROOM_FLAG_1))
                Meshes[7]->DrawLitEnv(mmInstance::StaticLighter, env_map, env_matrix, 1);
            else
                Meshes[7]->DrawLit(nullptr, 1, nullptr);
        }

        if (Meshes[lod] && !Sim()->IsDebugDrawEnabled())
        {
            // A mesh still paging in drops to the coarser one for this frame rather
            // than leaving a hole in the ground.
            if (!Meshes[lod]->IsFullyResident(0) && lod && Meshes[lod - 1])
                --lod;

            if (lod && !(RoomFlags & ROOM_FLAG_1))
                Meshes[lod]->DrawLitEnv(mmInstance::StaticLighter, env_map, env_matrix, 1);
            else
                Meshes[lod]->DrawLit(nullptr, 1, nullptr);
        }

        TexSorter()->Cull(1);

        // Cell statistics, bucketed by where the cell number falls. The boundaries
        // are raw cell indices, as everywhere else in this file.
        const i32 bucket = (Index < 0xC9) ? 0 : ((Index >= 0x35C) ? 2 : 1);

        ++CellTypeCount[bucket];
        CellTriCount[bucket] += agiTexSorter::TotalTris - tris_before;

        CullCity()->BuildingChain.Draw(Index, 0, 0, 0, BuildingMaxDist); // PATCH: Custom max building dist

        if (Drawbridge)
            Drawbridge->Draw(lod);
    }

    if (asRenderWeb::PassMask & RENDER_PASS_SHADOWS)
    {
        // Shadows are laid over what is already there: no Z writing, and Z testing
        // only when a bias keeps them off the surface they sit on.
        agiCurState.SetZEnable(ShadowZBias != 0.0f);
        agiCurState.SetZWrite(false);

        if (agiCurState.GetSoftwareRendering())
            agiCurState.SetTexturePerspective(false);

        if (agiRQ.Shadow == 3)
            CullCity()->ShadowChain.Draw(Index, 0, 1, 0, 30.0f);

        // The extra 0x2000 goes on whenever the shadow quality is low or it is late
        // enough in the day that the objects need their own lighting instead.
        const i16 object_flags = (agiRQ.Shadow >= 2 && static_cast<i32>(MMSTATE.TimeOfDay) < 3) ? 2 : 0x2002;

        CullCity()->ObjectsChain.Draw(Index, object_flags, 1, 0, 70.0f);

        if (agiCurState.GetSoftwareRendering())
            agiCurState.SetTexturePerspective(true);
    }

    i32 second_lod = lod;

    if (asRenderWeb::PassMask & RENDER_PASS_BUILDINGS)
    {
        agiCurState.SetZEnable(ZREAD != 0);
        agiCurState.SetZWrite(ZWRITE != 0);

        if (Meshes[3] && !Sim()->IsDebugDrawEnabled())
            Meshes[3]->DrawLit(mmInstance::StaticLighter, 1, nullptr);

        if (DrawLabelFArg)
            LabelInstances = 1;

        CullCity()->BuildingChain.Draw(Index, 0, 0, 0, BuildingMaxDist); // PATCH: Custom max building dist

        if (DrawLabelFArg)
            LabelInstances = 0;

        if (Meshes[4 + second_lod] && !Sim()->IsDebugDrawEnabled())
        {
            if (!Meshes[4 + second_lod]->IsFullyResident(0) && second_lod && Meshes[3 + second_lod])
                --second_lod;

            // The building chain has left its own transform behind.
            Viewport()->SetWorld(IDENTITY);

            Meshes[4 + second_lod]->DrawLit(mmInstance::StaticLighter, 1, nullptr);
        }
    }

    if ((asRenderWeb::PassMask & RENDER_PASS_OBJECTS) && distance < ObjectMaxDist)
    {
        agiCurState.SetZEnable(ZREAD != 0);
        agiCurState.SetZWrite(ZWRITE != 0);

        if (agiCurState.GetSoftwareRendering())
            agiCurState.SetTexturePerspective(false);

        const b32 old_sph_map = agiRQ.SphMap;

        if (RoomFlags & ROOM_FLAG_1)
            agiRQ.SphMap = false;

        if (DrawLabelPArg)
            LabelInstances = 1;

        CullCity()->ObjectsChain.Draw(Index, 0, 1, 1, ObjectMaxDist);

        // The original sets it again rather than clearing it, unlike the building
        // pass above. Left as it is.
        if (DrawLabelPArg)
            LabelInstances = 1;

        agiRQ.SphMap = old_sph_map;

        if (agiCurState.GetSoftwareRendering())
            agiCurState.SetTexturePerspective(true);

        if (Drawbridge)
            Drawbridge->Draw(second_lod);
    }

    agiMeshSet::FogValue = old_fog;

    if (asRenderWeb::PassMask & RENDER_PASS_LIGHTS)
    {
        CullCity()->ObjectsChain.Draw(
            Index, 0x400, 1, 1, LightDistances[agiRQ.TerrainQuality]);
    }

    // Cells this one can see into that the portal walk did not reach. A negative tag
    // means draw it regardless; a positive one is skipped once it has been visited
    // this frame.
    if (sub_cull & 1)
    {
        for (i32 i = 0; i < VisitTagCount; ++i)
        {
            const i16 tag = static_cast<i16>(VisitTags[i]);
            const i32 index = std::abs(static_cast<i32>(tag));

            asPortalCell* cell = nullptr;

            if (index < CullCity()->RenderWeb.MaxCells)
                cell = CullCity()->RenderWeb.CellArray[index];

            // PORT SHIM: the original dereferences the cell without checking. An out
            // of range tag would be a null here rather than a fault.
            if (!cell)
                continue;

            if (tag >= 0 && cell->VisitTag == asPortalWeb::VisitTag)
                continue;

            cell->CellRenderer->Cull(0);
        }
    }
}
