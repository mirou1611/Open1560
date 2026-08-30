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
