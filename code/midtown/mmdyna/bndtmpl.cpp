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

define_dummy_symbol(mmdyna_bndtmpl);

#include "bndtmpl.h"

#include "core/string.h"
#include "data7/hash.h"
#include "isect.h"

#ifdef ARTS_DEV_BUILD
void mmBoundTemplate::DrawGraph()
{}
#endif

i32 mmBoundTemplate::LineSphere(mmIntersection* /*arg1*/)
{
    return 0;
}

// One entry per name_part, shared by every instance that asks for the same bound.
static HashTable BoundTemplateHash {128, "BoundTemplates"};

mmBoundTemplate::mmBoundTemplate()
{
    RefCount = 0;
    Handle = 0;
    PageState = 0;

    Center = {0.0f, 0.0f, 0.0f};
    Radius = 0.0f;
    RadiusSqr = 0.0f;
    BBMin = {0.0f, 0.0f, 0.0f};
    BBMax = {0.0f, 0.0f, 0.0f};

    NumVerts = 0;
    NumPolys = 0;
    Verts = nullptr;
    Polygons = nullptr;

    NumHotVerts1 = 0;
    NumHotVerts2 = 0;
    NumEdges = 0;
    HotVerts = nullptr;
    EdgeVerts1 = nullptr;
    EdgeVerts2 = nullptr;
    EdgePlaneNs = nullptr;
    EdgePlaneDs = nullptr;

    XDim = 0;
    YDim = false;
    ZDim = 0;
    RowOffsets = nullptr;
    BucketOffsets = nullptr;
    RowBuckets = nullptr;
    FixedHeights = nullptr;

    XScale = 0.0f;
    ZScale = 0.0f;
    Flags = 0;
    field_AC = 0;

    // Deliberately left alone, exactly as the original leaves them: PagerInfo, field_70,
    // field_74 and HeightScale. Load writes all four before anything reads them.

    MaxPerBucket = 0;
    NumIndexs = 0;
    ConstructionTable = nullptr;
    DrawGrid = 0;
    WinID = 0;
}

mmBoundTemplate::~mmBoundTemplate()
{
    if (Name)
        BoundTemplateHash.Delete(Name.get());

    // Name itself is a ConstString and frees its own copy.

    // Only a development build owns these outright. In a shipping build they are either
    // mapped from the archive or handed out by the pager, and freeing them here would be
    // freeing someone else's memory.
    if (DevelopmentMode)
    {
        delete[] FixedHeights;
        delete[] RowBuckets;
        delete[] RowOffsets;
        delete[] BucketOffsets;
        delete[] Polygons;
        delete[] Verts;
        delete[] EdgeVerts1;
        delete[] EdgeVerts2;
        delete[] EdgePlaneNs;
        delete[] EdgePlaneDs;
    }
}

void mmBoundTemplate::AddRef()
{
    ValidatePtr(const_cast<char*>("AddRef"));

    ++RefCount;
}

i32 mmBoundTemplate::Release()
{
    ValidatePtr(const_cast<char*>("Release"));

    if (--RefCount == 0)
    {
        delete this;
        return 0;
    }

    return static_cast<i32>(RefCount);
}

RcOwner<mmBoundTemplate> mmBoundTemplate::GetBoundTemplate(
    aconst char* name, aconst char* part, Vector3* offset, i32 arg4, i32 arg5, i32 arg6, i32 arg7, i32 arg8)
{
    // The original builds the key with sprintf and no null check, so a caller passing no
    // part - asRenderWeb::LoadHitId does - gets whatever the C library prints for a null
    // %s in the key. That is stable within a run, which is all the key needs to be, so it
    // is left as it was rather than quietly changed.
    char key[128];
    arts_sprintf(key, "%s_%s", name, part);

    mmBoundTemplate* tmpl = static_cast<mmBoundTemplate*>(BoundTemplateHash.Access(key));

    if (!tmpl)
    {
        tmpl = new mmBoundTemplate();

        if (!tmpl->Load(const_cast<char*>(name), const_cast<char*>(part), offset, arg4, arg5, arg6, arg7, 0, arg8))
        {
            delete tmpl;
            return nullptr;
        }

        // The table keys off the template's own copy of the name, not the stack buffer.
        tmpl->Name = key;
        BoundTemplateHash.Insert(tmpl->Name.get(), tmpl);
    }

    tmpl->AddRef();

    return RcOwner<mmBoundTemplate> {tmpl};
}

i32 mmBoundTemplate::Collide(mmIntersection* isect)
{
    // Pure dispatch. Two axes decide it: what shape is being swept, and how much
    // structure this bound has to test it against.
    //
    //   NumPolys == 0    there is no geometry at all, only the bounding sphere
    //   RowBuckets == 0  there is geometry but no acceleration table, so every
    //                    polygon gets tested after a cheap sphere reject
    //   otherwise        the table narrows it to a few buckets first
    if (isect->Type == 5 || isect->Type == 6)
    {
        if (!NumPolys)
            return SphereSphere(isect);

        VertPtr = Verts;

        if (!RowBuckets)
            return SphereGeometry(isect);

        return QuickSphereBox(isect) ? SphereTable(isect) : 0;
    }

    if (!NumPolys)
        return LineSphere(isect);

    if (!RowBuckets)
        return QuickLineSphere(isect) ? LineGeometry(isect) : 0;

    return QuickLineBox(isect) ? LineTable(isect) : 0;
}

i32 mmBoundTemplate::QuickLineBox(mmIntersection* isect)
{
    Flags |= 4;

    const Vector3& from = isect->LocalMin;
    const Vector3& to = isect->LocalMax;

    // Slab rejection: the segment cannot touch the box if both of its ends sit outside
    // the same face. The per-axis order below is the original's, which tests a different
    // face first on Y than on X and Z; the result is the same either way.
    if (from.y > BBMax.y && to.y > BBMax.y)
        return 0;

    if (from.y < BBMin.y && to.y < BBMin.y)
        return 0;

    if (from.x < BBMin.x && to.x < BBMin.x)
        return 0;

    if (from.x > BBMax.x && to.x > BBMax.x)
        return 0;

    if (from.z < BBMin.z && to.z < BBMin.z)
        return 0;

    if (from.z > BBMax.z && to.z > BBMax.z)
        return 0;

    Flags |= 8;

    return 1;
}
