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

define_dummy_symbol(mmcityinfo_vehinfo);

#include "vehinfo.h"

#include "agiworld/texsheet.h"
#include "stream/stream.h"

// The original only zeroes Valid and IsLocked here; the in-class initializers cover
// those and every other field.
mmVehInfo::mmVehInfo() = default;

i32 mmVehInfo::Load(char* arg1)
{
    if (Ptr<Stream> input {arts_fopen(arg1, "r")})
    {
        // Every line has to be present and in this order
        Valid = arts_fscanf(input.get(), "BaseName=%s", BaseName) &&
            arts_fscanf(input.get(), "Description=%[^\r]", Description) &&
            arts_fscanf(input.get(), "Colors=%[^\r]", Colors) &&
            arts_fscanf(input.get(), "Flags=%d", &Flags) && arts_fscanf(input.get(), "Order=%d", &Order) &&
            arts_fscanf(input.get(), "ScoringBias=%f", &ScoringBias) &&
            arts_fscanf(input.get(), "UnlockScore=%d", &UnlockScore) &&
            arts_fscanf(input.get(), "UnlockFlags=%d", &UnlockFlags) &&
            arts_fscanf(input.get(), "Horsepower=%d", &Horsepower) &&
            arts_fscanf(input.get(), "Top Speed=%d", &TopSpeed) &&
            arts_fscanf(input.get(), "Durability=%d", &Durability) &&
            arts_fscanf(input.get(), "Mass=%d", &Mass);
    }
    else
    {
        Valid = false;
    }

    if (Valid)
        TEXSHEET.Load(arts_formatf<64>("mtl/%s.tsh", BaseName));

    return Valid;
}
