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

define_dummy_symbol(agi_cmodelx);

#include "cmodelx.h"

#include "rgba.h"
#include "surface.h"

RcOwner<agiColorModel> agiColorModel::FindMatch(agiSurfaceDesc* surface)
{
    return FindMatch(surface->PixelFormat.RBitMask, surface->PixelFormat.GBitMask, surface->PixelFormat.BBitMask,
        surface->PixelFormat.RGBAlphaBitMask);
}

static agiColorModel* const ColorModels[] {
    &ColorModelRGB888,
    &ColorModelRGB888_Rev,
    &ColorModelRGB555,
    &ColorModelRGB565,
    &ColorModelRGB555_Rev,
    &ColorModelRGB565_Rev,
    &ColorModelRGBA5551,
    &ColorModelRGBA4444,
    &ColorModelARGB,
    &ColorModelABGR,
    nullptr,
};

RcOwner<agiColorModel> agiColorModel::FindMatch(i32 mask_r, i32 mask_g, i32 mask_b, i32 mask_a)
{
    for (agiColorModel* const* models = ColorModels; *models; ++models)
    {
        agiColorModel* model = *models;

        if ((static_cast<u32>(mask_r) == model->GetMaskR()) && (static_cast<u32>(mask_g) == model->GetMaskG()) &&
            (static_cast<u32>(mask_b) == model->GetMaskB()) && (static_cast<u32>(mask_a) == model->GetMaskA()))
        {
            return as_owner AddRc(model);
        }
    }

    Quitf("Couldn't find match for R=%x G=%x B=%x A=%x", mask_r, mask_g, mask_b, mask_a);
}

u32 agiColorModelARGB::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u32*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB555::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB565::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB555_Rev::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB565_Rev::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB888::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u32*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGB888_Rev::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u32*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGBA5551::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelRGBA4444::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u16*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

u32 agiColorModelABGR::GetPixel(agiSurfaceDesc* surface, i32 x, i32 y)
{
    return reinterpret_cast<u32*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x];
}

// Colour model instances and their setup, reimplemented from game.asm.
// Field values are taken directly from the original constructors: pixel size,
// per-channel bit counts, then per-channel shifts.

agiColorModelRGB555::agiColorModelRGB555()
{
    PixelSize = 2;
    BitCountR = 5;
    BitCountG = 5;
    BitCountB = 5;
    BitCountA = 0;
    ShiftR = 10;
    ShiftG = 5;
    ShiftB = 0;
    ShiftA = 0;
}

agiColorModelRGB565::agiColorModelRGB565()
{
    PixelSize = 2;
    BitCountR = 5;
    BitCountG = 6;
    BitCountB = 5;
    BitCountA = 0;
    ShiftR = 11;
    ShiftG = 5;
    ShiftB = 0;
    ShiftA = 0;
}

agiColorModelRGB555_Rev::agiColorModelRGB555_Rev()
{
    PixelSize = 2;
    BitCountR = 5;
    BitCountG = 5;
    BitCountB = 5;
    BitCountA = 0;
    ShiftR = 0;
    ShiftG = 5;
    ShiftB = 10;
    ShiftA = 0;
}

agiColorModelRGB565_Rev::agiColorModelRGB565_Rev()
{
    PixelSize = 2;
    BitCountR = 5;
    BitCountG = 6;
    BitCountB = 5;
    BitCountA = 0;
    ShiftR = 0;
    ShiftG = 5;
    ShiftB = 11;
    ShiftA = 0;
}

agiColorModelRGB888::agiColorModelRGB888()
{
    PixelSize = 4;
    BitCountR = 8;
    BitCountG = 8;
    BitCountB = 8;
    BitCountA = 0;
    ShiftR = 16;
    ShiftG = 8;
    ShiftB = 0;
    ShiftA = 0;
}

agiColorModelRGB888_Rev::agiColorModelRGB888_Rev()
{
    PixelSize = 4;
    BitCountR = 8;
    BitCountG = 8;
    BitCountB = 8;
    BitCountA = 0;
    ShiftR = 0;
    ShiftG = 8;
    ShiftB = 16;
    ShiftA = 0;
}

agiColorModelRGBA5551::agiColorModelRGBA5551()
{
    PixelSize = 2;
    BitCountR = 5;
    BitCountG = 5;
    BitCountB = 5;
    BitCountA = 1;
    ShiftR = 10;
    ShiftG = 5;
    ShiftB = 0;
    ShiftA = 15;
}

agiColorModelRGBA4444::agiColorModelRGBA4444()
{
    PixelSize = 2;
    BitCountR = 4;
    BitCountG = 4;
    BitCountB = 4;
    BitCountA = 4;
    ShiftR = 8;
    ShiftG = 4;
    ShiftB = 0;
    ShiftA = 12;
}

agiColorModelARGB::agiColorModelARGB()
{
    PixelSize = 4;
    BitCountR = 8;
    BitCountG = 8;
    BitCountB = 8;
    BitCountA = 8;
    ShiftR = 16;
    ShiftG = 8;
    ShiftB = 0;
    ShiftA = 24;
}

agiColorModelABGR::agiColorModelABGR()
{
    PixelSize = 4;
    BitCountR = 8;
    BitCountG = 8;
    BitCountB = 8;
    BitCountA = 8;
    ShiftR = 0;
    ShiftG = 8;
    ShiftB = 16;
    ShiftA = 24;
}

u32 agiColorModelARGB::GetColor(agiRgba color)
{
    return (u32(color.A) << 24) | (u32(color.R) << 16) | (u32(color.G) << 8) | u32(color.B);
}

u32 agiColorModelARGB::FindColor(agiRgba color)
{
    return GetColor(color);
}

// Per-channel average of four pixels, truncating.
u32 agiColorModelARGB::Filter(u32 arg1, u32 arg2, u32 arg3, u32 arg4)
{
    u32 a = ((arg1 >> 24) + (arg2 >> 24) + (arg3 >> 24) + (arg4 >> 24)) >> 2;
    u32 r = (((arg1 >> 16) & 0xFF) + ((arg2 >> 16) & 0xFF) + ((arg3 >> 16) & 0xFF) + ((arg4 >> 16) & 0xFF)) >> 2;
    u32 g = (((arg1 >> 8) & 0xFF) + ((arg2 >> 8) & 0xFF) + ((arg3 >> 8) & 0xFF) + ((arg4 >> 8) & 0xFF)) >> 2;
    u32 b = ((arg1 & 0xFF) + (arg2 & 0xFF) + (arg3 & 0xFF) + (arg4 & 0xFF)) >> 2;

    return (a << 24) | (r << 16) | (g << 8) | b;
}

void agiColorModelARGB::SetPixel(agiSurfaceDesc* surface, i32 x, i32 y, u32 color)
{
    reinterpret_cast<u32*>(static_cast<u8*>(surface->Surface) + (y * surface->Pitch))[x] = color;
}

agiColorModelRGB555 ColorModelRGB555;
agiColorModelRGB565 ColorModelRGB565;
agiColorModelRGB555_Rev ColorModelRGB555_Rev;
agiColorModelRGB565_Rev ColorModelRGB565_Rev;
agiColorModelRGB888 ColorModelRGB888;
agiColorModelRGB888_Rev ColorModelRGB888_Rev;
agiColorModelRGBA5551 ColorModelRGBA5551;
agiColorModelRGBA4444 ColorModelRGBA4444;
agiColorModelARGB ColorModelARGB;
agiColorModelABGR ColorModelABGR;
