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

define_dummy_symbol(mmcar_carmodel);

#include "carmodel.h"

#include "carsim.h"

#include "agiworld/meshset.h"
#include "mmbangers/data.h"

#include "car.h"

enum
{
    MESH_BODY = 0,
    MESH_SHADOW = 1,
    MESH_HLIGHT = 2,
    MESH_TLIGHT = 3,
    MESH_SLIGHT0 = 4,
    MESH_SLIGHT1 = 5,
    MESH_WHL0 = 6,
    MESH_WHL1 = 7,
    MESH_WHL2 = 8,
    MESH_WHL3 = 9,
    MESH_WHL4 = 10,
    MESH_WHL5 = 11,
    MESH_AXLE0 = 12,
    MESH_AXLE1 = 13,
    MESH_FNDR0 = 14,
    MESH_FNDR1 = 15,
    MESH_REDLIGHT = 16,
    MESH_BLUELIGHT = 17,
    MESH_REDCONE = 18,
    MESH_BLUECONE = 19,
    MESH_RLIGHT = 20,
    MESH_BLIGHT = 21,
};

mmCarModel::mmCarModel()
{
    SetFlags(INST_FLAG_SHADOW | INST_FLAG_MOVER | INST_FLAG_100 | INST_FLAG_GLOW | INST_FLAG_2000);
    CarFlags |= CAR_FLAG_ACTIVE;

    Sparks.Init(256, GetSparkLut("tune/spark.tga"_xconst));
}

i32 mmCarModel::GetCarFlags(char* /*arg1*/)
{
    return 0;
}

void mmCarModel::Activate()
{
    Flags |= INST_FLAG_ACTIVE;
    CarFlags |= CAR_FLAG_ACTIVE;
}

void mmCarModel::Deactivate()
{
    // The player's car stays in the world when it is not drawn - the physics and the
    // culling still want it - so only the draw flag comes off. Everyone else leaves.
    if (CarSim->IsPlayer())
        CarFlags &= ~CAR_FLAG_ACTIVE;
    else
        Flags &= ~INST_FLAG_ACTIVE;
}

void mmCarModel::DashActivated()
{
    Flags |= INST_FLAG_ACTIVE;
    CarFlags &= ~CAR_FLAG_ACTIVE;
}

void mmCarModel::DashDeactivated()
{
    CarFlags |= CAR_FLAG_ACTIVE;
}

// Key function - see the note in joint3dof.cpp.
mmCarModel::~mmCarModel() = default;

Vector3& mmCarModel::GetPos()
{
    return CarSim->LCS.World.m3;
}

void mmCarModel::Init(aconst char* name, mmCar* car, i32 paint_job)
{
    Entity = car;
    CarSim = &car->Sim;

    // The wheel and fender bangers - the pieces that can come off - are registered
    // with the banger manager rather than loaded here.
    WHL0_Entry = static_cast<i16>(BangerDataMgr()->AddBangerDataEntry(name, "WHL0"));

    if (CarFlags & CAR_FLAG_FENDERS)
        FNDR0_Entry = static_cast<i16>(BangerDataMgr()->AddBangerDataEntry(name, "FNDR0"));

    PaintJobIndex = paint_job;

    // The paint job picks which colour variant of the body and fenders to load. The
    // original shifts a single bit up into the variant field rather than storing the
    // index there - and the body then ORs in 0x100 again, which is MESH_SET_KEEP_LOADED
    // and variant bit 0 at once. Kept as it is.
    const i32 paint_variant = (1 << MESH_SET_VARIANT_SHIFT) << paint_job;

    const i32 mesh_flags = MESH_SET_UV | MESH_SET_NORMAL | MESH_SET_CPV;
    const i32 part_flags = mesh_flags | MESH_SET_NO_BOUND;
    const i32 breakable_flags = mesh_flags | MESH_SET_BREAKABLE;

    InitMeshes(name, paint_variant | MESH_SET_KEEP_LOADED | mesh_flags, "BODY", nullptr);

    InitDamage();

    AddMeshes(name, part_flags, "SHADOW", nullptr);
    AddMeshes(name, part_flags, "HLIGHT", nullptr);

    // The headlight glow can stick out past the body. When lights are on, widen the
    // body's cull radius to cover it, or the car goes dark at the wrong moment.
    // BODY is mesh 0, SHADOW is 1 and HLIGHT is 2, in the order they were added.
    agiMeshSet* body = GetMeshSet(INST_LOD_HIGH, 0);
    agiMeshSet* hlight = GetMeshSet(INST_LOD_HIGH, 2);

    if (body && hlight && ShowLights && body->Radius < hlight->Radius)
    {
        body->Radius = hlight->Radius;
        body->RadiusSqr = hlight->RadiusSqr;
    }

    AddMeshes(name, part_flags, "TLIGHT", nullptr);
    AddMeshes(name, part_flags, "SLIGHT0", nullptr);
    AddMeshes(name, part_flags, "SLIGHT1", nullptr);

    // The wheels are drawn at their own centres, so each one carries the offset its
    // suspension puts it at.
    AddMeshes(name, breakable_flags, "WHL0", &CarSim->FrontLeft.Center);
    AddMeshes(name, part_flags, "WHL1", &CarSim->FrontRight.Center);

    if (CarFlags & CAR_FLAG_6_WHEELS)
    {
        // A six-wheeler carries the middle pair as fixed positions rather than as
        // suspended wheels.
        AddMeshes(name, part_flags, "WHL2", &CarSim->WHL2_Pos);
        AddMeshes(name, part_flags, "WHL3", &CarSim->WHL3_Pos);
    }
    else
    {
        AddMeshes(name, part_flags, "WHL2", &CarSim->BackLeft.Center);
        AddMeshes(name, part_flags, "WHL3", &CarSim->BackRight.Center);
    }

    AddMeshes(name, part_flags, "WHL4", nullptr);
    AddMeshes(name, part_flags, "WHL5", nullptr);

    AddMeshes(name, part_flags, "AXLE0", &CarSim->FrontAxle.Center);
    AddMeshes(name, part_flags, "AXLE1", &CarSim->BackAxle.Center);

    // The fenders sit on the front wheels and take the paint job with them.
    AddMeshes(name, paint_variant | breakable_flags, "FNDR0", &CarSim->FrontLeft.Center);
    AddMeshes(name, paint_variant | part_flags, "FNDR1", &CarSim->FrontRight.Center);

    // Police lights and their cones, and the reverse lights.
    AddMeshes(name, part_flags, "REDLIGHT", nullptr);
    AddMeshes(name, part_flags, "BLUELIGHT", nullptr);
    AddMeshes(name, part_flags, "REDCONE", nullptr);
    AddMeshes(name, part_flags, "BLUECONE", nullptr);
    AddMeshes(name, part_flags, "RLIGHT", nullptr);
    AddMeshes(name, part_flags, "BLIGHT", nullptr);
}
