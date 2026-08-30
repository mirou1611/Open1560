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

define_dummy_symbol(mmeffects_meshform);

#include "meshform.h"

#include "agiworld/getmesh.h"
#include "agiworld/meshset.h"
#include "data7/printer.h"
#include "vector7/vector3.h"

void asMeshSetForm::SetShape(aconst char* name, aconst char* group, Vector3* offset)
{
    // 0x107: UV, normals and per-vertex colour, plus 0x100 - the flag that tells
    // GetMeshSet to read the mesh here and now rather than register it with the pager.
    Mesh = GetMeshSet(name, group, offset, MESH_SET_UV | MESH_SET_NORMAL | MESH_SET_CPV | MESH_SET_NO_PAGING);

    if (!Mesh)
        Errorf("asMeshSetForm::SetShape(%s,%s) failed.", name, group);
}
