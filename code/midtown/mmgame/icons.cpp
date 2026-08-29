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

define_dummy_symbol(mmgame_icons);

#include "icons.h"

// The arrow that points at an opponent, and the quad the icon itself sits on
static agiMeshCardVertex TriangleCardVerts[3] {
    {0.0f, 0.0f, 1.0f, 0.0f},
    {0.5f, 2.0f, 1.0f, 1.0f},
    {-0.5f, 2.0f, 1.0f, 1.0f},
};

static agiMeshCardVertex QuadCardVerts[4] {
    {-1.0f, 2.5f, 0.0f, 0.0f},
    {1.0f, 2.5f, 1.0f, 0.0f},
    {1.0f, 4.5f, 1.0f, 1.0f},
    {-1.0f, 4.5f, 0.0f, 1.0f},
};

mmIcons::mmIcons()
{
    field_44 = 0;
    field_50 = 0;

    TriangleCard.Init(ARTS_SSIZE32(TriangleCardVerts), TriangleCardVerts, 1, 1, 1);
    QuadCard.Init(ARTS_SSIZE32(QuadCardVerts), QuadCardVerts, 1, 4, 4);

    field_40 = 0;
}
