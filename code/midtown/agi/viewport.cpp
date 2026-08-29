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

define_dummy_symbol(agi_viewport);

#include "viewport.h"

#include "pipeline.h"
#include "vector7/matrix34.h"

void agiViewParameters::SetWorld(const Matrix34& world)
{
    World = world;
    ModelView.Dot(World, View);
    ++MtxSerial;
}

agiViewport::agiViewport(agiPipeline* pipe)
    : agiRefreshable(pipe)
    , field_144_(pipe_->GetDword38())
{}

agiViewport::~agiViewport()
{
    if (this == Active)
        Active = nullptr;
}

void agiViewport::SetWorld(aconst Matrix34& world)
{
    params_.SetWorld(world);
}

f32 agiViewport::Aspect()
{
    if (state_)
    {
        agiPipeline* pipe = Pipe();

        return (pipe->GetWidth() * params_.Width) / (pipe->GetHeight() * params_.Height);
    }

    return params_.Width / params_.Height;
}

aconst char* agiViewport::GetName()
{
    static char buffer[128]; // FIXME: Static buffer
    arts_sprintf(buffer, "Viewport '%p'", this);
    return buffer;
}

void agiViewParameters::Perspective(f32 arg1, f32 arg2, f32 arg3, f32 arg4)
{
    Fov = arg1;
    Aspect = arg2;
    Near = arg3;
    Far = arg4;

    ProjXZ = 0.0f;
    ProjYZ = 0.0f;
    Orthographic = false;

    // arg1 is a horizontal FOV in degrees; 0.5 * pi / 180 turns it into the half-angle
    f32 tan_half = std::tan(arg1 * 0.008726646f);

    f32 near_height = tan_half * Near;
    f32 near_width = Aspect * near_height;

    ProjBottom = near_height;
    ProjRight = near_width;

    ProjX = Near / near_width;
    ProjY = Near / near_height;

    f32 inv_depth = 1.0f / (Far - Near);

    DepthScale = inv_depth;
    ProjZZ = -((Far + Near) * inv_depth);
    ProjZW = (Far * Near * -2.0f) * inv_depth;

    ++MtxSerial;
}

agiViewParameters::agiViewParameters()
{
    World.Identity();
    Camera.Identity();
    View.Identity();
    ModelView.Identity();

    X = 0.0f;
    Y = 0.0f;
    Width = 1.0f;
    Height = 1.0f;

    ++ViewSerial;

    Perspective(90.0f, 1.25f, 1.0f, 1000.0f);
}
