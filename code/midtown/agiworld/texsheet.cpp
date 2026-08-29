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

define_dummy_symbol(agiworld_texsheet);

#include "texsheet.h"

#include "data7/quitf.h"
#include "stream/stream.h"

i32 cmpTex(const void* arg1, const void* arg2)
{
    return arts_stricmp(*static_cast<const char* const*>(arg1), *static_cast<const char* const*>(arg2));
}

// Like strtok, but it does not skip runs of delimiters - an empty field yields an empty
// token rather than being swallowed, which is what makes the table columns line up.
char* mystrtok(char* str, const char* delims)
{
    static char* context = nullptr;

    if (str)
        context = str;

    char* here = context;

    if (*here == '\0')
        return nullptr;

    while (!std::strchr(delims, *here))
    {
        char next = here[1];
        ++here;

        if (next == '\0')
            break;
    }

    *here = '\0';

    char* result = context;
    context = here + 1;

    return result;
}

static void ToUpper(char* text)
{
    for (char* c = text; *c; ++c)
        *c = static_cast<char>(std::toupper(static_cast<unsigned char>(*c)));
}

void agiTexSheet::Load(aconst char* arg1)
{
    Ptr<Stream> stream {arts_fopen(arg1, "r")};

    if (!stream)
        Quitf("Can't open texsheet '%s'", arg1);

    char buffer[256];

    // Count the rows the hard way, so the table can be sized in one go. The first line
    // is a header, hence one less than the number of lines.
    i32 rows = -1;

    if (arts_fgets(buffer, ARTS_SSIZE(buffer), stream.get()))
    {
        do
        {
            ++rows;
        } while (arts_fgets(buffer, ARTS_SSIZE(buffer), stream.get()));
    }

    stream->Seek(0);

    // Grow the table, keeping whatever was already in it
    agiTexProp* props = new agiTexProp[prop_count_ + rows] {};

    if (prop_count_)
        std::memcpy(props, props_, prop_count_ * sizeof(agiTexProp));

    delete[] props_;
    props_ = props;

    arts_fgets(buffer, ARTS_SSIZE(buffer), stream.get());

    for (i32 i = prop_count_; i < prop_count_ + rows; ++i)
    {
        agiTexProp& prop = props_[i];

        arts_fgets(buffer, ARTS_SSIZE(buffer), stream.get());

        prop.Name = arts_strdup(mystrtok(buffer, ",\r\n"));
        ToUpper(prop.Name);

        std::strncpy(prop.Palette, mystrtok(nullptr, ",\r\n"), 3);

        prop.High = static_cast<u8>(std::atoi(mystrtok(nullptr, ",\r\n")));
        prop.Medium = static_cast<u8>(std::atoi(mystrtok(nullptr, ",\r\n")));
        prop.Low = static_cast<u8>(std::atoi(mystrtok(nullptr, ",\r\n")));

        prop.Flags = 0;

        // 'm' (AlwaysModulate) and 'p' (AlwaysPerspCorrect) are named in the flag enum
        // but this build does not parse them - they report as unknown, as they did in
        // the original.
        if (const char* flags = mystrtok(nullptr, ",\r\n"))
        {
            for (; *flags; ++flags)
            {
                switch (*flags)
                {
                    case 'w': prop.Flags |= agiTexProp::Snowable; break;
                    case 'g': prop.Flags |= agiTexProp::AlphaGlow; break;
                    case 'l': prop.Flags |= agiTexProp::Lightmap; break;
                    case 's': prop.Flags |= agiTexProp::Shadow; break;
                    case 't': prop.Flags |= agiTexProp::Transparent; break;
                    case 'k': prop.Flags |= agiTexProp::Chromakey; break;
                    case 'n': prop.Flags |= agiTexProp::NotLit; break;
                    case 'd': prop.Flags |= agiTexProp::DullOrDamaged; break;
                    case 'u': prop.Flags |= agiTexProp::ClampUOrBoth; break;
                    case 'v': prop.Flags |= agiTexProp::ClampVOrBoth; break;
                    case 'c': prop.Flags |= agiTexProp::ClampBoth; break;
                    case 'U': prop.Flags |= agiTexProp::ClampUOrNeither; break;
                    case 'V': prop.Flags |= agiTexProp::ClampVOrNeither; break;
                    case 'e': prop.Flags |= agiTexProp::RoadFloorCeiling; break;

                    default: Errorf("Row %d of '%s': Unknown flag '%c'", i - prop_count_ + 2, arg1, *flags); break;
                }
            }
        }

        // Everything past the flags is optional, and a row may simply stop
        char* token = mystrtok(nullptr, ",\r\n");

        if (!token)
            continue;

        if (*token)
        {
            prop.AlternateName = arts_strdup(token);
            ToUpper(prop.AlternateName);
        }

        token = mystrtok(nullptr, ",\r\n");

        if (!token)
            continue;

        if (*token)
        {
            prop.Sibling = arts_strdup(token);
            ToUpper(prop.Sibling);
        }

        token = mystrtok(nullptr, ",\r\n");

        if (!token)
            continue;

        prop.Width = static_cast<u16>(std::atoi(token));

        token = mystrtok(nullptr, ",\r\n");

        if (!token)
            continue;

        prop.Height = static_cast<u16>(std::atoi(token));

        token = mystrtok(nullptr, ",\r\n");

        if (!token)
            continue;

        prop.Color = static_cast<u32>(std::strtol(token, nullptr, 16));
    }

    prop_count_ += rows;

    // Lookup is a binary search, so the table has to stay sorted by name
    std::qsort(props_, prop_count_, sizeof(agiTexProp), cmpTex);
}

agiTexProp* agiTexSheet::Lookup(aconst char* name, i32 variation)
{
    agiTexProp* result = nullptr;

    while (name)
    {
        result = static_cast<agiTexProp*>(std::bsearch(&name, props_, prop_count_, sizeof(agiTexProp), cmpTex));

        if (!result)
            return nullptr;

        if (variation == 0)
            break;

        --variation;
        name = result->Sibling;
    }

    if (result)
        result->Flags |= agiTexProp::Referenced;

    return result;
}

i32 agiTexSheet::GetVariationCount(aconst char* arg1)
{
    i32 count = 0;

    for (const char* name = arg1; name;)
    {
        agiTexProp* prop = static_cast<agiTexProp*>(std::bsearch(&name, props_, prop_count_, sizeof(agiTexProp), cmpTex));

        if (!prop)
            break;

        ++count;
        name = prop->Sibling;
    }

    return count;
}

char* agiTexSheet::RemapName(aconst char* arg1, i32 arg2)
{
    agiTexProp* prop = Lookup(arg1, arg2);

    if (!prop)
        return const_cast<char*>(arg1);

    if (use_alternate_ && prop->AlternateName)
        return prop->AlternateName;

    return prop->Name;
}

