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

// agiSurfaceDesc::Load, reimplemented from game.asm.
//
// The search order and the surface it produces follow the original exactly:
//
//   * `path` is a double-NUL terminated list of directories. For each in turn it
//     tries "<dir>/<name>.jpg", then "<dir>/<name>.dds" - or
//     "<dir>/<name>.%04d.dds" when an index is given.
//   * a JPEG becomes a 16 bit R5G6B5 surface, stored bottom row first, scaled to
//     the requested size when one was given.
//   * a DDS is read as a 124 byte header (32 bit fields on disk) followed by
//     agiSurfaceDesc::Reload for the pixels, with `pack` dropping mip levels.
//
// The one deliberate difference: the original drove the JPEG decoder that lives
// in the game binary (mmdjpeg, which has no C++ definition of
// jpeg_decompress_struct at all). This uses stb_image instead, which decodes the
// same files and lets the loader work on platforms the original decoder cannot
// reach.

define_dummy_symbol(agi_surfaceload);

#include "surface.h"

#include "pipeline.h"
#include "texdef.h"
#include "stream/fsystem.h"
#include "stream/stream.h"

#define STB_IMAGE_IMPLEMENTATION
#define STBI_NO_STDIO
#define STBI_ONLY_JPEG
#define STBI_NO_LINEAR
#define STBI_NO_HDR
#define STBI_ASSERT(x) ArAssert(x, "stb_image")
#include <stb_image.h>

// The DDS magic, as it appears at the start of the file.
static constexpr u32 DdsMagic = 0x20534444; // 'DDS '

static constexpr u32 JpegPixelFormatSize = 0x20;

// R5G6B5, matching the masks the original set up by hand.
static u16 PackRgb565(u32 r, u32 g, u32 b)
{
    return static_cast<u16>(((r & 0xF8) << 8) | ((g & 0xFC) << 3) | (b >> 3));
}

// Reads the whole stream, decodes it, and writes a 16 bit R5G6B5 image of
// `width` x `height` into `pixels`, bottom row first. Nearest neighbour when the
// requested size differs from the file's.
static bool DecodeJpeg(Stream* input, u32 width, u32 height, u16* pixels)
{
    isize size = static_cast<isize>(input->Size());

    if (size <= 0)
        return false;

    Ptr<u8[]> file = arnewa u8[size];

    if (input->Read(file.get(), size) != size)
        return false;

    int src_width = 0;
    int src_height = 0;
    int src_channels = 0;

    stbi_uc* src = stbi_load_from_memory(file.get(), static_cast<int>(size), &src_width, &src_height, &src_channels, 3);

    if (src == nullptr)
    {
        Errorf("Failed to decode JPEG: %s", stbi_failure_reason());

        return false;
    }

    for (u32 y = 0; y < height; ++y)
    {
        // The surface is stored bottom row first.
        u16* dst_row = pixels + (height - 1 - y) * width;
        const stbi_uc* src_row = src + static_cast<usize>((y * src_height) / height) * src_width * 3;

        for (u32 x = 0; x < width; ++x)
        {
            const stbi_uc* texel = src_row + ((x * src_width) / width) * 3;

            dst_row[x] = PackRgb565(texel[0], texel[1], texel[2]);
        }
    }

    stbi_image_free(src);

    return true;
}

// Steps to the next entry of a double-NUL terminated list, or nullptr at the end.
static const char* NextSearchPath(const char* path)
{
    const char* next = path + std::strlen(path) + 1;

    return (*next != '\0') ? next : nullptr;
}

[[nodiscard]] Owner<agiSurfaceDesc> agiSurfaceDesc::Load(
    aconst char* name, aconst char* path, i32 index, i32 pack, i32 width, i32 height)
{
    char file_path[ARTS_MAX_PATH];

    Ptr<Stream> input;
    bool is_jpeg = false;

    for (const char* dir = path; dir; dir = NextSearchPath(dir))
    {
        arts_sprintf(file_path, "%s/%s.jpg", dir, name);
        input = arts_fopen(file_path, "r");

        if (input)
        {
            is_jpeg = true;

            break;
        }

        if (index)
            arts_sprintf(file_path, "%s/%s.%04d.dds", dir, name, index);
        else
            arts_sprintf(file_path, "%s/%s.dds", dir, name);

        input = arts_fopen(file_path, "r");

        if (input)
            break;

        if (fsVerbose)
            Displayf("Image file '%s' not found.", file_path);
    }

    if (input == nullptr)
    {
        if (fsVerbose)
            Errorf("Image %s not found.", name);

        return nullptr;
    }

    if (is_jpeg)
    {
        Ptr<agiSurfaceDesc> result = arnew agiSurfaceDesc();

        // A requested size wins; otherwise the file decides.
        int src_width = 0;
        int src_height = 0;

        if (width && height)
        {
            result->Width = static_cast<u32>(width);
            result->Height = static_cast<u32>(height);
        }
        else
        {
            // stb needs the header to answer this, and the stream is read again
            // below - cheap next to decoding.
            isize size = static_cast<isize>(input->Size());
            Ptr<u8[]> header = arnewa u8[size];

            if (input->Read(header.get(), size) != size ||
                !stbi_info_from_memory(header.get(), static_cast<int>(size), &src_width, &src_height, nullptr))
            {
                Errorf("Failed to read JPEG header for '%s'", file_path);

                return nullptr;
            }

            input->Seek(0);

            result->Width = static_cast<u32>(src_width);
            result->Height = static_cast<u32>(src_height);
        }

        result->Flags = AGISD_HEIGHT | AGISD_WIDTH;
        result->Pitch = static_cast<i32>(result->Width * 2);
        result->Surface = new u8[result->Width * result->Height * 2];

        result->PixelFormat.Size = JpegPixelFormatSize;
        result->PixelFormat.Flags = AGIPF_RGB;
        result->PixelFormat.RGBBitCount = 16;
        result->PixelFormat.RBitMask = 0xF800;
        result->PixelFormat.GBitMask = 0x07E0;
        result->PixelFormat.BBitMask = 0x001F;
        result->PixelFormat.RGBAlphaBitMask = 0;

        if (!DecodeJpeg(input.get(), result->Width, result->Height, static_cast<u16*>(result->Surface)))
            return nullptr;

        return as_owner result;
    }

    if (fsVerbose)
        Displayf("Loading %s/%s (pack=%d)...", path, name, pack);

    u32 magic = 0;

    if (input->Read(&magic, sizeof(magic)) != sizeof(magic) || magic != DdsMagic)
        return nullptr;

    Ptr<agiSurfaceDesc> result = arnew agiSurfaceDesc();

    // The header on disk is the 32 bit form of this struct.
    agiSurfaceDescDisk disk {};

    if (input->Read(&disk, sizeof(disk)) != sizeof(disk))
        return nullptr;

    disk.CopyTo(*result);

    // `pack` drops mip levels: the smaller image starts that many levels in.
    result->MipMapCount -= static_cast<u32>(pack);
    result->Width >>= pack;
    result->Height >>= pack;
    result->Surface = nullptr;

    result->Reload(xconst(name), xconst(path), index, pack, input.get(), 0, 0);

    if (result->PixelFormat.Flags & AGIPF_PALETTEINDEXED8)
    {
        // The palette name is the four characters stored in place of the lut pointer.
        arts_sprintf(file_path, "%s/nbr%s.lut", path, result->szLut);

        result->lpLut = as_raw Pipe()->GetTexLut(file_path);
    }

    return as_owner result;
}
