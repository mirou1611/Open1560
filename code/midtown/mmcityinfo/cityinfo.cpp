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

define_dummy_symbol(mmcityinfo_cityinfo);

#include "cityinfo.h"

#include "data7/str.h"
#include "stream/stream.h"

mmCityInfo::mmCityInfo() = default;
mmCityInfo::~mmCityInfo() = default;
b32 mmCityInfo::Load(char* path)
{
    // Each name list is a '|'-separated string, read whole and then counted
    char blitz_names[512] {};
    char circuit_names[512] {};
    char checkpoint_names[512] {};

    if (Ptr<Stream> input {arts_fopen(path, "r")})
    {
        Loaded = arts_fscanf(input.get(), "LocalizedName=%[^\r]", LocalizedName) &&
            arts_fscanf(input.get(), "MapName=%s", MapName) && arts_fscanf(input.get(), "RaceDir=%s", RaceDir) &&
            arts_fscanf(input.get(), "BlitzCount=%d", &BlitzCount) &&
            arts_fscanf(input.get(), "CircuitCount=%d", &CircuitCount) &&
            arts_fscanf(input.get(), "CheckpointCount=%d", &CheckpointCount) &&
            arts_fscanf(input.get(), "BlitzNames=%[^\r]", blitz_names) &&
            arts_fscanf(input.get(), "CircuitNames=%[^\r]", circuit_names) &&
            arts_fscanf(input.get(), "CheckpointNames=%[^\r]", checkpoint_names);
    }
    else
    {
        Loaded = false;
    }

    // A count of zero means the city has no races of that kind, and its name list is
    // left alone. Otherwise the count comes from the list itself, not the file.
    if (Loaded)
    {
        if (BlitzCount)
        {
            BlitzCount = string(blitz_names).NumSubStrings();
            BlitzNames = blitz_names;
        }

        if (CheckpointCount)
        {
            CheckpointCount = string(checkpoint_names).NumSubStrings();
            CheckpointNames = checkpoint_names;
        }

        if (CircuitCount)
        {
            CircuitCount = string(circuit_names).NumSubStrings();
            CircuitNames = circuit_names;
        }
    }

    return Loaded;
}
