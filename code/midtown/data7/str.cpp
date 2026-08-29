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

define_dummy_symbol(data7_str);

#include "str.h"

char ExecPath[1024];
char ProjPath[1024];
i32 string::NumSubStrings() const
{
    i32 count = 1;

    for (const char* c = data_; c && *c; ++c)
    {
        if (*c == '|')
            ++count;
    }

    return count;
}

void string::Init(i32 arg1)
{
    i32 capacity = arg1 + 50;

    if (capacity > capacity_)
    {
        if (capacity_)
            delete[] data_;

        capacity_ = capacity;
        data_ = new char[capacity];
    }
}

void string::operator=(const char* arg1)
{
    i32 needed = static_cast<i32>(std::strlen(arg1)) + 1;

    if (needed >= capacity_)
    {
        if (capacity_)
            delete[] data_;

        capacity_ = needed + 50;
        data_ = new char[capacity_];
    }

    std::memcpy(data_, arg1, needed);
}

void string::operator+=(char arg1)
{
    i32 len = static_cast<i32>(std::strlen(data_));

    if (len + 1 >= capacity_)
    {
        i32 capacity = len + 51;
        char* buffer = new char[capacity];

        std::memcpy(buffer, data_, len + 1);

        delete[] data_;
        data_ = buffer;
        capacity_ = capacity;
    }

    data_[len] = arg1;
    data_[len + 1] = '\0';
}

void string::operator+=(const char* arg1)
{
    i32 len = static_cast<i32>(std::strlen(data_));
    i32 tail = static_cast<i32>(std::strlen(arg1));

    if (len + tail >= capacity_)
    {
        i32 capacity = len + tail + 50;
        char* buffer = new char[capacity];

        std::memcpy(buffer, data_, len + 1);

        delete[] data_;
        data_ = buffer;
        capacity_ = capacity;
    }

    std::memcpy(data_ + len, arg1, tail + 1);
}

string string::SubString(i32 arg1) const
{
    Ptr<char[]> buffer {new char[std::strlen(data_) + 1]};

    i32 num = 1;
    i32 pos = 0;

    // Walk forward until the start of the requested sub-string, or the end
    if (arg1 != 1)
    {
        for (char c; (c = data_[pos]) != '\0';)
        {
            if (c == '|')
                ++num;

            ++pos;

            if (num == arg1)
                break;
        }
    }

    i32 len = 0;

    if (num == arg1)
    {
        for (char c; (c = data_[pos]) != '\0' && c != '|'; ++pos)
            buffer[len++] = c;
    }

    buffer[len] = '\0';

    return string(buffer.get());
}

i32 string::Contains(string& arg1) const
{
    i32 count = NumSubStrings();

    for (i32 i = 1; i <= count; ++i)
    {
        if (!std::strcmp(arg1.get(), SubString(i).get()))
            return 1;
    }

    return 0;
}

// Splits a path the way the original did: only a backslash separates the directory,
// and the extension keeps its leading dot. Any part that is not present comes back
// empty.
void string::DirFileExt(string& arg1, string& arg2, string& arg3) const
{
    arg1 = "";
    arg3 = "";
    arg2 = data_;

    for (i32 i = static_cast<i32>(std::strlen(data_)) - 1; i >= 0; --i)
    {
        if (data_[i] == '\\')
        {
            arg1 = data_;
            arg1.get()[i] = '\0';

            arg2 = data_ + i + 1;
            break;
        }
    }

    for (i32 i = static_cast<i32>(std::strlen(arg2.get())) - 1; i >= 0; --i)
    {
        if (arg2.get()[i] == '.')
        {
            arg3 = arg2.get() + i;
            arg2.get()[i] = '\0';
            break;
        }
    }
}

void string::SaveName(const string& arg1, i32 arg2, const string& arg3, const string& arg4)
{
    string dir, file, ext;
    arg1.DirFileExt(dir, file, ext);

    if (arg2 >= 1 && arg2 <= 9999)
        file += arts_formatf<16>(".%04d", arg2);

    if (dir.get()[0] == '\0')
    {
        // No directory in the name, so take the first of the defaults offered
        string fallback = arg3.SubString(1);
        *this = fallback.get();
    }
    else
    {
        *this = dir.get();
    }

    *this += "/";
    *this += file.get();
    *this += ext.get();

    // If the name did not already carry one of the accepted extensions, add the first
    if (!arg4.Contains(ext))
        *this += arg4.SubString(1).get();
}
