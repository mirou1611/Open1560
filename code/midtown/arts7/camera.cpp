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

define_dummy_symbol(arts7_camera);

#include "camera.h"

#include "agi/bitmap.h"
#include "agi/lmodel.h"
#include "agi/pipeline.h"
#include "agi/rsys.h"
#include "agi/viewport.h"
#include "benchstats.h"
#include "cullmgr.h"
#include "dyna7/gfx.h"
#include "sim.h"

static mem::cmd_param PARAM_fovfix {"fovfix"};

void asCamera::SetView(f32 horz_fov, f32 aspect, f32 near_clip, f32 far_clip)
{
    // Aspect is supposed to represent the intended aspect ratio for the provided horizontal FOV
    // However, if it is ever set to 0, it will always be ignored from then on (this is the case in-game)
    // This means the value is basically useless, so instead assume it was intended for a 4:3 display
    // This preserves intended vertical FOV on wider screens, including in the dash view
    //
    // TODO: Store the vertical fov instead, and handle this in asCamera::Update
    // https://forum.unity.com/threads/adjust-fov-based-on-aspect-ratio-how.474627/#post-3097919
    if (PARAM_fovfix.get_or(true))
    {
        // Calculate the horizontal tangent
        f32 horz_tan = std::tan(horz_fov * 0.5f);

        aspect = 640.0f / 480.0f;

        // Calculate the vertical tangent, based on the intended (4:3) aspect ratio
        f32 vert_tan = horz_tan / aspect;

        // Now calculate the actual aspect ratio
        aspect = static_cast<f32>(Pipe()->GetWidth()) / static_cast<f32>(Pipe()->GetHeight());

        // Calculate the horizontal FOV, based on the actual aspect ratio
        horz_fov = 2.0f * std::atan(vert_tan * aspect);

        // Use auto aspect
        aspect = 0.0f;
    }

    fov_ = horz_fov;
    near_clip_ = near_clip;
    far_clip_ = far_clip;

    if (aspect == 0.0f)
    {
        auto_aspect_ = true;
        aspect_ = 1.0f;
    }
    else
    {
        // auto_aspect_ = false;
        aspect_ = aspect;
    }
}

void asCamera::DrawBegin()
{
    i32 draw_mode = Sim()->GetDrawMode();

    if (draw_mode == agiDrawTextured)
        draw_mode = draw_mode_;

    agiCurState.SetDrawMode(static_cast<agiDrawMode>(draw_mode));

    if (fog_density_ != 0.0f)
    {
        agiCurState.SetFogMode(agiFogMode::Pixel);
        agiCurState.SetFogStart(fog_start_);
        agiCurState.SetFogEnd(fog_end_);
        agiCurState.SetFogDensity(fog_density_);
        agiCurState.SetFogColor((static_cast<u32>(fog_color_.x * 255.0) << 16) |
            (static_cast<u32>(fog_color_.y * 255.0) << 8) | static_cast<u32>(fog_color_.z * 255.0));
    }
    else
    {
        agiCurState.SetFogMode(agiFogMode::None);
    }

    viewport_->Activate();

    if (underlay_bitmap_)
    {
        if (underlay_callback_)
        {
            underlay_callback_->Call();
        }
        else
        {
            Pipe()->CopyBitmap(
                UI_XPos, UI_YPos, underlay_bitmap_, 0, 0, underlay_bitmap_->GetWidth(), underlay_bitmap_->GetHeight());
        }

        if (!underlay_bitmap_->Is3D())
            Pipe()->BeginScene();
    }

    i32 clear_flags = clear_flags_;

    if (draw_mode < agiDrawSolid || Sim()->IsDebugDrawEnabled())
    {
        clear_flags |= AGI_VIEW_CLEAR_TARGET;
    }

    if (underlay_bitmap_ && !underlay_callback_ && agiCurState.GetDrawMode() != agiDrawDepth)
    {
        clear_flags &= ~AGI_VIEW_CLEAR_TARGET;
    }

    viewport_->Clear(clear_flags);

    if (light_model_)
        light_model_->Activate();
}

asCamera::asCamera()
{
    viewport_ = Pipe()->CreateViewport().release();
    VW = viewport_;

    light_model_ = Pipe()->CreateLightModel().release();
    light_params_ = new agiLightModelParameters();

    underlay_bitmap_ = nullptr;
    underlay_callback_ = nullptr;

    SetViewport(0.0f, 0.0f, 1.0f, 1.0f, 1);
    SetLighting(1);

    bg_color_ = {0.0f, 0.0f, 0.0f};
    shadow_color_ = {0.0f, 0.0f, 0.0f, 1.0f};

    field_5C = 0;
    field_60 = 0;
    field_64 = 0;

    clear_flags_ = AGI_VIEW_CLEAR_TARGET | AGI_VIEW_CLEAR_ZBUFFER;

    draw_mode_ = 0xF;
    field_C0 = 1;

    // The original cleared auto_aspect_ at the end of the constructor, after SetView.
    // It could do that because its SetView never touched the field. Open1560's SetView
    // does (fovfix asks for auto aspect), so clear it first and let SetView have the
    // last word.
    auto_aspect_ = false;

    SetView(1.5707964f, 1.33f, 0.1f, 1000.0f);
    SetFog(0.0f, 0.0f, 0.0f, 0.0f);

    float_C4 = 1.0f;
    field_C8 = 0;
    field_CC = 0;

    float_E0 = 1.0f;
    fog_start_ = 1.0f;
    fog_end_ = 100.0f;

    camera_.Identity();
    view_.Identity();

    field_14C = 0;
    field_150 = 0;
    field_154 = 0;
    field_158 = 0;
    field_15C = 0;

    pause_fade_ = 0;
    fade_amount_ = 0.0f;
    fade_speed_ = 0.2f;
    max_fade_ = 0.0f;
    fade_color_ = {0.0f, 0.0f, 0.0f};
    float_17C = 1.0f;
    fade_ticks_ = 0;
    field_184 = 0;
}

asCamera::~asCamera()
{
    if (viewport_)
        viewport_->Release();

    if (light_model_)
        light_model_->Release();

    delete light_params_;

    if (underlay_bitmap_)
        underlay_bitmap_->Release();
}

void asCamera::Update()
{
    asBenchStats& stats = Sim()->GetStats();

    stats.field_4 += field_14C;
    stats.field_8 += field_150;
    stats.field_14 += field_15C;
    stats.field_10 += field_158;
    stats.field_C += field_154;

    field_15C = 0;
    field_150 = 0;
    field_14C = 0;

    if (auto_aspect_)
    {
        aspect_ = (static_cast<f32>(Pipe()->GetWidth()) * x_size_) /
            (static_cast<f32>(Pipe()->GetHeight()) * y_size_);
    }

    // Horizontal half-angle: the frustum's side planes
    f32 half_fov = fov_ * 0.5f;
    f32 sin_h = std::sin(half_fov);
    f32 cos_h = std::cos(half_fov);
    f32 cot_h = cos_h / sin_h;

    float_80 = cot_h;
    float_84 = cot_h * aspect_;
    left_clip_scale_ = sin_h;
    bottom_clip_scale_ = cos_h;

    // Vertical FOV follows from the horizontal one and the aspect ratio
    fov_radians_ = 2.0f * std::atan(1.0f / (cot_h * aspect_));

    f32 half_vfov = fov_radians_ * 0.5f;
    right_clip_scale_ = std::sin(half_vfov);
    top_clip_scale_ = std::cos(half_vfov);

    SetClipArea(-1.0f, 1.0f, -1.0f, 1.0f);

    if (fade_amount_ != max_fade_ && pause_fade_ == 0)
    {
        f32 delta = max_fade_ - fade_amount_;
        f32 step = fade_speed_ * Sim()->GetUpdateDelta();

        if (delta >= 0.0f)
            fade_amount_ = (delta > step) ? fade_amount_ + step : max_fade_;
        else
            fade_amount_ = (-delta > step) ? fade_amount_ - step : max_fade_;
    }

    camera_ = *Sim()->GetCurrentMatrix();
    view_.FastInverse(camera_);

    if (Sim()->IsFullUpdate())
    {
        agiViewParameters& params = viewport_->GetParams();

        params.Perspective(fov_radians_ * 57.29578f, aspect_, near_clip_, far_clip_);

        params.Camera = camera_;
        params.View = view_;
        params.ModelView.Dot(params.World, params.View);
        ++agiViewParameters::MtxSerial;

        viewport_->SetBackground(Sim()->GetDrawMode() == agiDrawTextured ? ORIGIN : bg_color_);

        CULLMGR->DeclareCamera(this);
    }

    asNode::Update();
}

void asCamera::SetClipArea(f32 arg1, f32 arg2, f32 arg3, f32 arg4)
{
    Vector2 plane;

    plane = {-bottom_clip_scale_, arg1 * left_clip_scale_};
    left_clip_ = plane * plane.InvMag();

    plane = {bottom_clip_scale_, -(arg2 * left_clip_scale_)};
    right_clip_ = plane * plane.InvMag();

    plane = {-top_clip_scale_, arg3 * right_clip_scale_};
    bottom_clip_ = plane * plane.InvMag();

    plane = {top_clip_scale_, -(arg4 * right_clip_scale_)};
    top_clip_ = plane * plane.InvMag();
}

void asCamera::SetLighting(i32 arg1)
{
    if (light_model_)
    {
        light_model_->Params.Changed = true;
        light_model_->Params.Enabled = arg1;
    }
}

void asCamera::SetWorld(Matrix34& arg1)
{
    viewport_->SetWorld(arg1);
}

void asCamera::SetViewport(f32 arg1, f32 arg2, f32 arg3, f32 arg4, i32 arg5)
{
    x_origin_ = arg1;
    y_origin_ = arg2;
    x_size_ = arg3;
    y_size_ = arg4;

    agiViewParameters& params = viewport_->GetParams();

    params.X = arg1;
    params.Y = arg2;
    params.Width = arg3;
    params.Height = arg4;

    ++agiViewParameters::ViewSerial;

    field_C0 = arg5;
}

void asCamera::SetFog(f32 arg1, f32 arg2, f32 arg3, f32 arg4)
{
    fog_density_ = arg1;
    fog_color_ = {arg2, arg3, arg4};
}

void asCamera::SetUnderlay(aconst char* arg1)
{
    // As in the original, the previous bitmap is dropped without a Release
    if (underlay_bitmap_)
    {
        underlay_bitmap_ = nullptr;
        underlay_callback_ = nullptr;
    }

    if (arg1)
        underlay_bitmap_ = Pipe()->GetBitmap(arg1, 1.0f, 1.0f, 0).release();
}

void asCamera::SetUnderlayCB(agiBitmap* arg1, Callback* arg2)
{
    underlay_bitmap_ = arg1;

    if (arg1)
        arg1->AddRef();

    underlay_callback_ = arg2;
}

void asCamera::FadeOut(f32 arg1, i32 arg2)
{
    if (arg1 == 0.0f)
    {
        fade_amount_ = 1.0f;
    }
    else
    {
        fade_amount_ = 0.0f;
        fade_speed_ = 1.0f / arg1;
    }

    max_fade_ = 1.0f;
    pause_fade_ = 1;
    field_184 = arg2;
}

void asCamera::FadeIn(f32 arg1, i32 arg2)
{
    if (arg1 == 0.0f)
    {
        fade_amount_ = 0.0f;
    }
    else
    {
        fade_amount_ = 1.0f;
        fade_speed_ = 1.0f / arg1;
    }

    max_fade_ = 0.0f;
    pause_fade_ = 1;
    field_184 = arg2;
}

void asCamera::Regen()
{
    if (light_model_)
        light_model_->Init(*light_params_);
}

void asCamera::DrawEnd()
{
    if (fade_amount_ == 0.0f && max_fade_ == 0.0f && pause_fade_ != 1 && fade_ticks_ <= 0)
        return;

    pause_fade_ = 0;

    f32 alpha = fade_ticks_ ? 1.0f : (fade_amount_ * fade_amount_);
    f32 z = near_clip_ * -1.001f;

    Vector4 color {fade_color_.x, fade_color_.y, fade_color_.z, alpha};

    Vector3 verts[4] {
        {-10.0f, -10.0f, z},
        {-10.0f, 10.0f, z},
        {10.0f, 10.0f, z},
        {10.0f, -10.0f, z},
    };

    ::DrawBegin(camera_);

    agiCurState.SetAlphaEnable(true);
    agiCurState.SetZEnable(false);
    agiCurState.SetZWrite(false);

    DrawColor(color);
    DrawQuad(nullptr, verts[0], verts[1], verts[2], verts[3]);

    ::DrawEnd();

    if (fade_ticks_)
        --fade_ticks_;
}
