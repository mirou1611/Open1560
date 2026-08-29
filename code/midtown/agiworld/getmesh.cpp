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

define_dummy_symbol(agiworld_getmesh);

#include "getmesh.h"
#include "agi/texdef.h"
#include "pcwindis/setupdata.h"
#include "texsheet.h"

#include "data7/global.h"
#include "data7/hash.h"
#include "data7/pager.h"
#include "stream/fsystem.h"
#include "stream/stream.h"
#include "agi/getdlp.h"

#include <algorithm>

static bool CheckEquals(const char* name, std::initializer_list<const char*> names)
{
    return std::find_if(names.begin(), names.end(), [name](const char* other) { return !std::strcmp(name, other); }) !=
        names.end();
}

void FixTexFlags(agiTexParameters& tex)
{
    agiTexProp* prop = TEXSHEET.Lookup(tex.Name, 0);

    if (!prop)
        return;

    tex.Flags |= agiTexParameters::WrapU | agiTexParameters::WrapV;

    switch (prop->Flags & agiTexProp::ClampModeMask)
    {
        case agiTexProp::ClampUOrBoth:
        case agiTexProp::ClampUOrNeither: tex.Flags &= ~agiTexParameters::WrapU; break;

        case agiTexProp::ClampVOrBoth:
        case agiTexProp::ClampVOrNeither: tex.Flags &= ~agiTexParameters::WrapV; break;

        case agiTexProp::ClampBoth: tex.Flags &= ~(agiTexParameters::WrapU | agiTexParameters::WrapV); break;
    }

    if (prop->Flags & agiTexProp::Transparent)
        tex.Flags |= agiTexParameters::Alpha;

    if (prop->Flags & (agiTexProp::Chromakey | (GetRendererInfo().AdditiveBlending ? agiTexProp::AlphaGlow : 0)))
        tex.Flags |= agiTexParameters::Chromakey;

    tex.Color = prop->Color; // TODO: TEXSHEET.AllowRemapping ? prop->NightColor : prop->DayColor

    if (prop->Flags & agiTexProp::AlphaGlow)
    {
        tex.Props |= agiTexProp::AlphaGlow;

        if (!GetRendererInfo().AdditiveBlending)
            tex.Flags |= agiTexParameters::Alpha;
    }

    if (prop->Flags & agiTexProp::Snowable)
    {
        tex.Props |= agiTexProp::Snowable;
        tex.Flags |= agiTexParameters::KeepLoaded | agiTexParameters::NoMipMaps;
    }

    if (prop->Flags & agiTexProp::Shadow)
        tex.Props |= agiTexProp::Shadow;

    if (prop->Flags & agiTexProp::DullOrDamaged)
        tex.Props |= agiTexProp::DullOrDamaged;

    if (prop->Flags & agiTexProp::NotLit)
        tex.Props |= agiTexProp::NotLit;

    if (prop->Flags & agiTexProp::RoadFloorCeiling)
        tex.Flags |= agiTexParameters::NoMipMaps;

    if (CheckEquals(tex.Name, {"WOMFACE", "MANFACE", "37_INSIDE"}))
        tex.Props |= agiTexProp::AlwaysModulate;

    if (!std::strcmp(tex.Name, "SNOW"))
        tex.Flags |= agiTexParameters::KeepLoaded;

    if (CheckEquals(tex.Name,
            {"T_STOP", "CHECK_POINT_02", "T_1WAY", "T_2WAY", "T_75", "T_DO_NOT_ENTER", "T_EXIT", "T_GLASS", "T_L_ONLY",
                "T_PARK02", "T_WRONGWAY", "FREEWAY_EXITS", "VPSEMI_TRAILER_BED", "T_TUN_TOP", "VABUS_SD"}))
        tex.Props |= agiTexProp::AlwaysPerspCorrect;

    // Reflections will draw over a transparent texture even if part of it is not visible
    if (tex.Flags & agiTexParameters::Alpha)
        tex.Props |= agiTexProp::DullOrDamaged;
}

// ?MeshCurrentObject@@3PADA
char* MeshCurrentObject = nullptr;

static HashTable MeshHash {64, "MeshHash"};
static HashTable BadMeshHash {64, "BadMeshHash"};

// The original keeps the result in a file-scope pointer and returns it from a common
// tail, rather than returning from each branch.
static agiMeshSet* CurrentMeshSet = nullptr;

// '3HSM' - the header on a compiled .bms
static constexpr u32 MeshSetMagic = 0x4D534833;

// Squared-distance tolerance on the offset baked into a cached mesh
static constexpr f32 MeshOffsetTolerance = 1.0e-6f;

agiMeshSet* GetMeshSet(aconst char* name, aconst char* group, Vector3* offset, i32 flags)
{
    if (!TEXSHEET.GetPropCount())
        TEXSHEET.Load("mtl/global.tsh"_xconst);

    Vector3 wanted {0.0f, 0.0f, 0.0f};

    if (offset)
        wanted = *offset;

    // Static so MeshCurrentObject stays valid after this returns - the original points it
    // at a stack buffer, which dangles the moment the function exits.
    static char key[256];

    arts_strcpy(key, name);

    if (group)
    {
        arts_strcat(key, "_");
        arts_strcat(key, group);
    }

    MeshCurrentObject = key;

    if (agiMeshSet* cached = static_cast<agiMeshSet*>(MeshHash.Access(key)))
    {
        cached->AddRef();
        CurrentMeshSet = cached;
        return cached;
    }

    // Names that failed to load once are remembered, so the failure is reported once
    if (BadMeshHash.Access(key))
        return nullptr;

    char bms_path[256];
    char geo_path[256];

    arts_sprintf(bms_path, "bms/%s", name);
    arts_sprintf(geo_path, "geo/%s.geo", name);

    if (group)
    {
        arts_strcat(bms_path, "_");
        arts_strcat(bms_path, group);
    }

    arts_strcat(bms_path, ".bms");

    // The shipping path: the archive carries compiled meshes, so the mesh is not read
    // here at all - it is registered with its pager info and read on demand by
    // agiMeshSet::DoPageIn. The radius defaults stand in until then.
    if (!DevelopmentMode && !(flags & MESH_SET_NO_PAGING) && (EnablePaging & ARTS_PAGE_GEOMETRY))
    {
        PagerInfo_t pager;

        if (FileSystem::PagerInfoAny(bms_path, pager))
        {
            agiMeshSet* mesh = new agiMeshSet();

            mesh->Pager = pager;
            mesh->Radius = 100.0f;
            mesh->RadiusSqr = 10000.0f;
            mesh->BoundingBoxRadius = 100.0f;

            CurrentMeshSet = mesh;
            return mesh;
        }
    }

    Ptr<Stream> stream;

    if (!DevelopmentMode || !OutOfDate(bms_path, geo_path))
        stream = arts_fopen(bms_path, "r");

    if (stream)
    {
        u32 magic = 0;
        Vector3 baked {};

        stream->Read(&magic, sizeof(magic));
        stream->Read(&baked, sizeof(baked));

        if (magic == MeshSetMagic && (!offset || (baked - wanted).Mag2() < MeshOffsetTolerance))
        {
            agiMeshSet* mesh = new agiMeshSet();

            if (flags & 0xF00)
                mesh->Variant = static_cast<u32>(flags >> 8);

            mesh->BinaryLoad(stream.get());
            mesh->Resident = 2;

            CurrentMeshSet = mesh;
            return mesh;
        }

        Warningf("Meshset %s.%s changed version or offset, recomputing", name, group ? group : "");
    }

    // Everything past here in the original is the DLP compiler - about 1500 assembly
    // lines that read geo/*.geo, deduplicate vertices, build adjacency and normals and
    // write the .bms back out. It only runs when there is no compiled mesh to load,
    // which on retail data means never. Not reimplemented.
    Errorf("GetMeshSet: no compiled mesh for '%s' and the DLP compiler is not ported", key);

    BadMeshHash.Insert(key, reinterpret_cast<void*>(1));

    CurrentMeshSet = nullptr;
    return nullptr;
}
