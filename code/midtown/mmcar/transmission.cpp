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

define_dummy_symbol(mmcar_transmission);

#include "transmission.h"

mmTransmission::mmTransmission()
{
    CarSim = nullptr;
    Clutch = 1.0f;
    NumGears = 6;
    InPark = false;

    SetCurrentGear(2);

    for (i32 i = 0; i < ARTS_SSIZE32(GearRatios); ++i)
    {
        GearRatios[i] = 0.0f;
        UpshiftRPM[i] = 6000.0f;
        DownshiftRPM[i] = 2000.0f;
    }

    ManualNumGears = 0;

    // Reverse, then five forward gears
    GearRatios[0] = -10.0f;
    GearRatios[1] = 14.0f;
    GearRatios[2] = 10.0f;
    GearRatios[3] = 8.0f;
    GearRatios[4] = 6.5f;
    GearRatios[5] = 4.0f;

    IsAutomatic = true;
    TimeInGear = 0.4f;
    DownshiftBias = 1.55f;
}
