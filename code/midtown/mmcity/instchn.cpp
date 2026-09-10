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

define_dummy_symbol(mmcity_instchn);

#include "instchn.h"

#include "inst.h"

#include "agi/viewport.h"
#include "agiworld/meshset.h"
#include "dyna7/gfx.h"
#include "memory/alloca.h"
#include "mmcity/renderweb.h"
#include "vector7/matrix34.h"

#include <utility>

b32 EnableSphereCull = true;
b32 LabelInstances = false;
b32 NormalsOnInstances = false;
i32 WorstCount = 0;
i32 WorstRoom = 0;

void mmInstChain::Draw(i16 chain, i16 mask, u32 lod_table, i32 sort, f32 max_dist)
{
    ArAssert(chain >= 0 && chain < NumChains, "chain >= 0 && chain < NumChains");

    agiViewParameters& params = Viewport()->GetParams();

    mmInstance::LodTableIndex = static_cast<i32>(lod_table);

    const i32 count = ChainCounts[chain];

    // Two parallel arrays: how far into the scene each instance sits, and the instance
    // it belongs to. The sort below moves them together.
    f32* depths = ARTS_ALLOCA(f32, count);
    mmInstance** visible = ARTS_ALLOCA(mmInstance*, count);

    if (count > WorstCount)
    {
        WorstCount = count;
        WorstRoom = chain;
    }

    i32 kept = 0;
    i32 dropped = 0;

    for (mmInstance* inst = Chains[chain]; inst; inst = inst->ChainNext)
    {
        // The mask does double duty: it picks which instances take part, and further
        // down it picks which of the three draw entry points they take. A zero mask is
        // the ordinary geometry pass, which everything active takes part in.
        if (!inst->TestFlags(INST_FLAG_ACTIVE) || !inst->GetMeshSet(INST_LOD_HIGH, 0) ||
            inst->GetFlags(static_cast<u16>(mask)) != mask)
        {
            ++dropped;
            continue;
        }

        Vector3& pos = inst->GetPos();

        if (EnableSphereCull)
        {
            f32 radius = 0.0f;

            if (agiMeshSet* mesh = inst->GetMeshSet(INST_LOD_HIGH, 0))
                radius = inst->GetScale() * mesh->Radius;

            if (!params.SphereVisible(pos, radius))
            {
                ++dropped;
                continue;
            }
        }

        // How far down the camera's own forward axis the instance sits, rather than how
        // far it is from the camera - so what the sort orders is depth into the scene.
        depths[kept] = (params.Camera.m3 - pos) ^ params.Camera.m2;

        if (depths[kept] > max_dist)
        {
            ++dropped;
            continue;
        }

        visible[kept] = inst;
        ++kept;
    }

    ArAssert(dropped + kept == count, "i+j == count");

    if (sort)
    {
        // Nearest first, so the depth buffer throws away as much of the far geometry as
        // it can before ever shading it. A selection sort, because a room holds a few
        // dozen instances at most.
        for (i32 i = 0; i < kept; ++i)
        {
            i32 nearest = i;

            for (i32 j = i + 1; j < kept; ++j)
            {
                if (depths[j] < depths[nearest])
                    nearest = j;
            }

            std::swap(depths[i], depths[nearest]);
            std::swap(visible[i], visible[nearest]);
        }
    }

    if (mask)
    {
        // A shadow or a glow pass draws only that, and takes no LOD - the mesh it uses
        // is chosen inside the instance.
        if (mask & INST_FLAG_SHADOW)
        {
            for (i32 i = 0; i < kept; ++i)
                visible[i]->DrawShadow();
        }
        else
        {
            for (i32 i = 0; i < kept; ++i)
                visible[i]->DrawGlow();
        }

        return;
    }

    for (i32 i = 0; i < kept; ++i)
    {
        mmInstance* inst = visible[i];

        // Anything behind the camera counts as being on top of it.
        f32 dist = (depths[i] < 0.0f) ? 0.0f : depths[i];

        i32 lod = inst->ComputeLod(dist, asRenderWeb::InvLodFactor);

        inst->Draw(lod);

#ifdef ARTS_DEV_BUILD
        if (LabelInstances)
        {
            DrawBegin(IDENTITY);

            mmBoundTemplate* bound = inst->GetBound();
            aconst char* name = inst->MeshIndex ? mmInstance::MeshSetNames[inst->MeshIndex - 1] : "(none)";

            // Upper case for a flag the instance has, lower case for one it does not.
            DrawLabelf(inst->GetPos(), "%s:%c%c%c", name, inst->TestFlags(INST_FLAG_COLLIDER) ? 'C' : 'c',
                inst->TestFlags(INST_FLAG_MOVER) ? 'M' : 'm', bound ? 'B' : 'b');

            DrawEnd();
        }
#endif

        if (NormalsOnInstances)
        {
            Matrix34 world;
            params.SetWorld(inst->ToMatrix(world));

            Vector3 color {1.0f, 0.0f, 0.0f};

            // The original hands DrawNormals a null mesh when the instance has none.
            if (agiMeshSet* mesh = inst->GetMeshSet(lod, 0))
                mesh->DrawNormals(color);
        }
    }
}

void mmInstChain::Init(i32 num_rooms)
{
    Chains = arnewa mmInstance* [num_rooms] {};
    ChainCounts = arnewa i16[num_rooms] {};
    NumChains = num_rooms;
}

void mmInstChain::Parent(mmInstance* inst, i16 room)
{
    ArAssert(inst->ChainId == -1, "Instance is already parented");
    ArAssert(room >= 0 && room < NumChains, "Invalid room");

    inst->ChainPrev = nullptr;

    mmInstance* next = Chains[room];
    inst->ChainNext = next;
    if (next)
        next->ChainPrev = inst;

    Chains[room] = inst;
    ++ChainCounts[room];
    inst->ChainId = room;
}

void mmInstChain::Reparent(mmInstance* inst, i16 room)
{
    Unparent(inst);
    Parent(inst, room);
}

void mmInstChain::Unparent(mmInstance* inst)
{
    ArAssert(inst->ChainId != -1, "Instance is not parented");
    ArAssert(inst->ChainId >= 0 && inst->ChainId < NumChains, "Invalid inst room");

    if (mmInstance* prev = inst->ChainPrev)
        prev->ChainNext = inst->ChainNext;
    else
        Chains[inst->ChainId] = inst->ChainNext;

    if (mmInstance* next = inst->ChainNext)
        next->ChainPrev = inst->ChainPrev;

    --ChainCounts[inst->ChainId];
    inst->ChainId = -1;
}

void mmInstChain::Relight(i16 room)
{
    for (mmInstance* i = Chains[room]; i; i = i->ChainNext)
        i->Relight();
}

void mmInstChain::RelightEverything()
{
    for (i16 i = 0; i < NumChains; ++i)
        Relight(i);
}

#ifdef ARTS_DEV_BUILD
void mmInstChain::AddWidgets(Bank* /*arg1*/)
{}
#endif
