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
