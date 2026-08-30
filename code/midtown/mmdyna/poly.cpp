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

define_dummy_symbol(mmdyna_poly);

#include "poly.h"

#include "mmdyna/bndtmpl.h"
#include "mmdyna/isect.h"

#include "dyna7/dyna.h"

f32 mmPolygon::CheckCellXSide(f32 plane_x, f32 z_min, f32 z_max)
{
    f32 max_y = -999999.0f;
    i32 num_verts = GetNumVerts();

    for (i32 i = 0; i < num_verts; ++i)
    {
        const Vector3& v1 = mmBoundTemplate::VertPtr[VertIndices[i]];
        const Vector3& v2 = mmBoundTemplate::VertPtr[VertIndices[(i + 1) % num_verts]];

        if ((v1.x < plane_x && v2.x > plane_x) || (v1.x > plane_x && v2.x < plane_x))
        {
            f32 factor = (plane_x - v1.x) / (v2.x - v1.x);
            f32 z_int = (v2.z - v1.z) * factor + v1.z;

            if (z_int >= z_min && z_int <= z_max)
                max_y = std::max(max_y, (v2.y - v1.y) * factor + v1.y);
        }
    }

    return max_y;
}

f32 mmPolygon::CheckCellZSide(f32 plane_z, f32 x_min, f32 x_max)
{
    f32 max_y = -999999.0f;
    i32 num_verts = GetNumVerts();

    for (i32 i = 0; i < num_verts; ++i)
    {
        const Vector3& v1 = mmBoundTemplate::VertPtr[VertIndices[i]];
        const Vector3& v2 = mmBoundTemplate::VertPtr[VertIndices[(i + 1) % num_verts]];

        if ((v1.z < plane_z && v2.z > plane_z) || (v1.z > plane_z && v2.z < plane_z))
        {
            f32 factor = (plane_z - v1.z) / (v2.z - v1.z);
            f32 x_int = (v2.x - v1.x) * factor + v1.x;

            if (x_int >= x_min && x_int <= x_max)
                max_y = std::max(max_y, (v2.y - v1.y) * factor + v1.y);
        }
    }

    return max_y;
}

f32 mmPolygon::CheckCorner(f32 x, f32 z, f32* plane_x, f32* plane_z, f32* plane_d)
{
    if (PlaneN.y == 0.0f)
        return -999999.0f;

    for (i32 i = 0; i < GetNumVerts(); ++i)
    {
        if ((x * plane_x[i]) + (z * plane_z[i]) + plane_d[i] < 0.0f)
            return -999999.0f;
    }

    return GetPlaneY(x, z);
}

f32 mmPolygon::CornersHeight(f32 x1, f32 z1, f32 x2, f32 z2)
{
    i32 num_verts = GetNumVerts();
    f32 sign = (PlaneN.y <= 0.0f) ? 1.0f : -1.0f;
    f32 plane_x[4] {};
    f32 plane_z[4] {};
    f32 plane_d[4] {};

    for (i32 i = 0; i < num_verts; ++i)
    {
        const Vector3& v1 = mmBoundTemplate::VertPtr[VertIndices[i]];
        const Vector3& v2 = mmBoundTemplate::VertPtr[VertIndices[(i + 1) % num_verts]];

        plane_x[i] = -(v2.z - v1.z) * sign;
        plane_z[i] = (v2.x - v1.x) * sign;
        plane_d[i] = -(plane_x[i] * v1.x + plane_z[i] * v1.z);

        f32 length = std::sqrt(plane_x[i] * plane_x[i] + plane_z[i] * plane_z[i]);

        if (length < 1e-9)
            return -999999.0f;

        plane_x[i] /= length;
        plane_z[i] /= length;
        plane_d[i] /= length;
    }

    f32 max_y = -999999.0f;
    max_y = std::max(max_y, CheckCorner(x1, z1, plane_x, plane_z, plane_d));
    max_y = std::max(max_y, CheckCorner(x1, z2, plane_x, plane_z, plane_d));
    max_y = std::max(max_y, CheckCorner(x2, z1, plane_x, plane_z, plane_d));
    max_y = std::max(max_y, CheckCorner(x2, z2, plane_x, plane_z, plane_d));

    return max_y;
}

f32 mmPolygon::MaxY(f32 x_min, f32 z_min, f32 x_max, f32 z_max)
{
    f32 max_y = -999999.0f;

    for (i32 i = 0; i < GetNumVerts(); ++i)
    {
        const Vector3& vert = mmBoundTemplate::VertPtr[VertIndices[i]];

        if (vert.x >= x_min && vert.x <= x_max && vert.z >= z_min && vert.z <= z_max)
            max_y = std::max(max_y, vert.y);
    }

    max_y = std::max(max_y, CornersHeight(x_min, z_min, x_max, z_max));
    max_y = std::max(max_y, CheckCellXSide(x_min, z_min, z_max));
    max_y = std::max(max_y, CheckCellXSide(x_max, z_min, z_max));
    max_y = std::max(max_y, CheckCellZSide(z_min, x_min, x_max));
    max_y = std::max(max_y, CheckCellZSide(z_max, x_min, x_max));

    return max_y;
}

void mmPolygon::Plot(mmBoundTemplate* t, i32 poly_index)
{
    if (IsQuad())
    {
        PlotTriangle(0, 1, 2, t, poly_index);
        PlotTriangle(0, 2, 3, t, poly_index);
    }
    else
    {
        PlotTriangle(0, 1, 2, t, poly_index);
    }
}

void mmPolygon::PlotScan(i32 x1, i32 x2, i32 z, mmBoundTemplate* t, i32 poly_index)
{
    if (t)
    {
        x1 = std::clamp(x1, 0, t->XDim - 1);
        x2 = std::clamp(x2, 0, t->XDim - 1);
        z = std::clamp(z, 0, t->ZDim - 1);

        for (i32 x = x1; x <= x2; ++x)
        {
            t->AddIndex(x, z, poly_index);
        }
    }
}

i32 mmPolygon::FullSegment(mmIntersection* isect)
{
    // Signed distance of each end of the segment from this polygon's plane. Note the
    // original compares against a literal zero here, not an epsilon.
    const Vector3& from = isect->LocalMin;
    const Vector3& to = isect->LocalMax;

    const f32 d0 = (PlaneN.x * from.x) + (PlaneN.y * from.y) + (PlaneN.z * from.z) + PlaneD;
    const f32 d1 = (PlaneN.x * to.x) + (PlaneN.y * to.y) + (PlaneN.z * to.z) + PlaneD;

    if (isect->Type == 3)
    {
        // Two-sided: the segment only has to straddle the plane, in either direction.
        if (d0 >= 0.0f && d1 >= 0.0f)
            return 0;

        if (d0 <= 0.0f && d1 <= 0.0f)
            return 0;
    }
    else
    {
        // One-sided: it has to enter through the front face and leave through the back.
        if (d0 <= 0.0f)
            return 0;

        if (d1 >= 0.0f)
            return 0;
    }

    // Where along the segment the plane is crossed, and how far that is squared. The
    // original only computes the distance on the one-sided path, where it is also the
    // reject; computing it either way avoids reading it uninitialized further down.
    const f32 t = d0 / (d0 - d1);
    const f32 distance = t * t * isect->MagnitudeSqr;

    if (isect->Type != 3 && isect->field_70 && distance > isect->field_74)
        return 0; // something nearer has already been hit

    ++SegVCPoly;

    const Vector3 hit {from.x + isect->Size.x * t, from.y + isect->Size.y * t, from.z + isect->Size.z * t};

    // The edge equations are two-dimensional, written in whichever plane this polygon
    // projects onto most cleanly. Bits 0 and 1 of Flags say which pair of axes that is.
    f32 u = 0.0f;
    f32 v = 0.0f;

    if (Flags & 2)
    {
        u = hit.x;
        v = hit.z;
    }
    else if (Flags & 1)
    {
        u = hit.x;
        v = hit.y;
    }
    else
    {
        u = hit.y;
        v = hit.z;
    }

    // Three edges always, and a fourth only for a quad.
    for (i32 i = 0, count = GetNumVerts(); i < count; ++i)
    {
        const Vector3& edge = PlaneEdges[i];

        if ((edge.x * u) + (edge.y * v) < edge.z)
            return 0; // outside this edge
    }

    // A hit. Record it.
    //
    // Not ported: when Type is 3 and this is not the first contact, the original refines
    // the result through mmPolygon::GetCorner, which is still assembly. That path belongs
    // to the body sweeps rather than to a raycast, and the plain record below is what the
    // first contact of a Type 3 sweep gets anyway.
    ++isect->field_70;

    if (isect->field_6C != 0.0f)
        isect->field_7C = t * isect->field_6C;

    isect->field_74 = distance;

    // How far past the plane the far end of the segment reached.
    isect->field_78 = -((d0 < 0.0f) ? d0 : d1);

    isect->Position = hit;
    isect->Normal = PlaneN;
    isect->HitPoly = this;

    return 1;
}
