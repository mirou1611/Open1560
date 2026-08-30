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

define_dummy_symbol(mmdyna_bndtmpl2);

#include "bndtmpl2.h"

#include "bndtmpl.h"
#include "poly.h"

#include "agi/getdlp.h"
#include "core/string.h"
#include "data7/ipc.h"
#include "data7/pager.h"
#include "data7/printer.h"
#include "stream/fsystem.h"
#include "stream/stream.h"
#include "vector7/geomath.h"

void mmBoundTemplate::PageIn()
{
    if (PageState == 0)
    {
        ++PageState;

        PAGER.Send([this] { DoPageIn(); });
    }
}

void mmBoundTemplate::ComputeBounds()
{
    GetBoundInfo(NumVerts, Verts, &BBMin, &BBMax, &Center, &Radius);
    RadiusSqr = Radius * Radius;
}

bool EdgeInList(i32 v1, i32 v2, ilong count, i32* edges1, i32* edges2)
{
    for (i32 i = 0; i < count; ++i)
    {
        i32 edge1 = edges1[i];
        i32 edge2 = edges2[i];

        if ((v1 == edge1 && v2 == edge2) || (v1 == edge2 && v2 == edge1))
        {
            return true;
        }
    }

    return false;
}

void mmBoundTemplate::ComputeEdges()
{
    i32 total_verts = 0;

    for (i32 i = 0; i < NumPolys; ++i)
    {
        total_verts += Polygons[i].GetNumVerts();
    }

    auto edges1 = arnewa i32[total_verts];
    auto edges2 = arnewa i32[total_verts];
    i32 num_edges = 0;

    for (i32 i = 0; i < NumPolys; ++i)
    {
        const mmPolygon& poly = Polygons[i + 1];

        i32 num_verts = poly.GetNumVerts();
        i32 v1 = poly.VertIndices[num_verts - 1];

        for (i32 k = 0; k < num_verts; ++k)
        {
            i32 v2 = poly.VertIndices[k];

            if (!EdgeInList(v1, v2, num_edges, edges1.get(), edges2.get()))
            {
                edges1[num_edges] = v1;
                edges2[num_edges] = v2;
                ++num_edges;
            }

            v1 = v2;
        }
    }

    if (num_edges)
    {
        EdgeVerts1 = new u32[num_edges];
        EdgeVerts2 = new u32[num_edges];
        HotVerts = Verts;
        NumHotVerts2 = NumVerts;

        for (i32 i = 0; i < num_edges; ++i)
        {
            EdgeVerts1[i] = edges1[i];
            EdgeVerts2[i] = edges2[i];
        }

        NumEdges = num_edges;
    }
}

void mmBoundTemplate::PlotSpan(i32 /*arg1*/, i32 /*arg2*/, i32 /*arg3*/)
{}

// '2DNB' - the tag at the front of a compiled .bnd
static constexpr u32 BoundMagic = 0x424E4432;

// A cached bound is only reusable if it was built at the offset being asked for.
static constexpr f32 BoundOffsetTolerance = 1.0e-6f;

i32 mmBoundTemplate::Load(char* name, char* part, Vector3* offset, i32 no_paging, i32 /*arg5*/,
    i32 /*arg6*/, i32 /*arg7*/, i32 /*arg8*/, i32 /*arg9*/)
{
    Vector3 wanted {0.0f, 0.0f, 0.0f};

    if (offset)
        wanted = *offset;

    char geo_path[256];
    arts_sprintf(geo_path, "geo/%s.geo", name);

    // Bounds nest the same way meshes do: bnd/<name>/<part>.bnd, or bnd/<name>.bnd with
    // no part.
    char bnd_path[256];
    arts_sprintf(bnd_path, "bnd/%s", name);

    if (part)
    {
        arts_strcat(bnd_path, "/");
        arts_strcat(bnd_path, part);
    }

    arts_strcat(bnd_path, ".bnd");

    // The shipping path: the archive carries compiled bounds, so nothing is read here at
    // all. The bound is registered with where it lives and paged in on demand.
    if (!DevelopmentMode && !no_paging && (EnablePaging & ARTS_PAGE_BOUNDS))
    {
        PagerInfo_t pager;

        if (FileSystem::PagerInfoAny(bnd_path, pager))
        {
            PagerInfo = pager;
            return 1;
        }
    }

    Ptr<Stream> file;

    if (!DevelopmentMode || !OutOfDate(geo_path, bnd_path))
        file = arts_fopen(bnd_path, "r"_xconst);

    if (file)
    {
        u32 magic = 0;
        Vector3 baked {};

        // The tag, the offset it was built at, and the table dimensions come first so
        // that a mismatch can be rejected before anything is allocated.
        // YDim is declared b32, which in this build is `bool` - one byte. Reading it with
        // sizeof would consume one byte where the file holds four and put every later read
        // three bytes out of step, which is exactly what it did.
        i32 y_dim = 0;

        file->Read(&magic, sizeof(magic));
        file->Read(&baked, sizeof(baked));
        file->Read(&XDim, sizeof(XDim));
        file->Read(&y_dim, sizeof(y_dim));
        file->Read(&ZDim, sizeof(ZDim));

        YDim = y_dim;

        if (magic == BoundMagic && (baked - wanted).Mag2() < BoundOffsetTolerance)
        {
            file->Read(&Center, sizeof(Center));
            file->Read(&Radius, sizeof(Radius));
            file->Read(&RadiusSqr, sizeof(RadiusSqr));
            file->Read(&BBMin, sizeof(BBMin));
            file->Read(&BBMax, sizeof(BBMax));
            file->Read(&NumVerts, sizeof(NumVerts));
            file->Read(&NumPolys, sizeof(NumPolys));
            file->Read(&NumHotVerts1, sizeof(NumHotVerts1));
            file->Read(&NumHotVerts2, sizeof(NumHotVerts2));
            file->Read(&NumEdges, sizeof(NumEdges));
            file->Read(&XScale, sizeof(XScale));
            file->Read(&ZScale, sizeof(ZScale));

            // NumIndexs is a static, not a member - the table reader below is the only
            // thing that uses it and it does so immediately.
            file->Read(&NumIndexs, sizeof(NumIndexs));
            file->Read(&HeightScale, sizeof(HeightScale));

            i32 unused = 0;
            file->Read(&unused, sizeof(unused));
            // ReadMapped either points these straight into the mapped file or allocates
            // and reads them, which is what the original spells out nine times.
            Verts = file->ReadMapped<Vector3>(NumVerts);
            Polygons = file->ReadMapped<mmPolygon>(NumPolys + 1);
            HotVerts = file->ReadMapped<Vector3>(NumHotVerts2);
            EdgeVerts1 = file->ReadMapped<u32>(NumEdges);
            EdgeVerts2 = file->ReadMapped<u32>(NumEdges);
            EdgePlaneNs = file->ReadMapped<Vector3>(NumEdges);
            EdgePlaneDs = file->ReadMapped<f32>(NumEdges);

            // The acceleration table is optional - a bound small enough to test
            // exhaustively ships without one.
            if (XDim && YDim && ZDim)
            {
                RowOffsets = file->ReadMapped<u32>(ZDim);
                BucketOffsets = file->ReadMapped<u16>(ZDim * XDim);
                RowBuckets = file->ReadMapped<u16>(NumIndexs);
                FixedHeights = file->ReadMapped<u8>(ZDim * XDim);
            }

            file.reset();

            // Two per-polygon scratch arrays the collision routines fill in as they run.
            field_70 = new u16[NumPolys + 1];
            field_74 = new u8[NumPolys + 1];

            PageState = 2;

            return 1;
        }

        Warningf("Bound file '%s/%s' format or offset doesn't match, regenerating.", name, part);
    }

    // Everything past here in the original builds the .bnd from geo/<name>.geo and writes
    // it back out - roughly 1700 of this function's 2032 lines, and the mirror image of
    // the DLP compiler that GetMeshSet also does not port. Retail data ships compiled, so
    // it never runs.
    Errorf("mmBoundTemplate::Load: no compiled bound at '%s' and the builder is not ported", bnd_path);

    return 0;
}
