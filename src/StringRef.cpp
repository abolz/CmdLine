// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/StringRef.h"

#ifdef _WIN32
#include <string.h>
#else
#include <strings.h>
#endif

using namespace support;

int StringRef::compare(StringRef RHS) const
{
    // Check prefix.
    int comp = traits_type::compare(data(), RHS.data(), Min(size(), RHS.size()));

    if (comp == 0)
    {
        // Prefixes match. Check lengths.
        if (size() < RHS.size())
            comp = -1;
        if (size() > RHS.size())
            comp = 1;
    }

    return comp;
}

int StringRef::compare_no_case(StringRef RHS) const
{
    // Check prefix.
#ifdef _WIN32
    int comp = ::_strnicmp(data(), RHS.data(), Min(size(), RHS.size()));
#else
    int comp = ::strncasecmp(data(), RHS.data(), Min(size(), RHS.size()));
#endif

    if (comp == 0)
    {
        // Prefixes match. Check lengths.
        if (size() < RHS.size())
            comp = -1;
        if (size() > RHS.size())
            comp = 1;
    }

    return comp;
}

size_t StringRef::find(char_type Ch, size_t From) const
{
    if (From >= size())
        return npos;

    if (auto I = traits_type::find(data() + From, size() - From, Ch))
        return I - data();

    return npos;
}

size_t StringRef::find(StringRef Str, size_t From) const
{
    if (Str.size() == 1)
        return find(Str[0], From);

    if (From > size() || Str.size() > size())
        return npos;

    if (Str.empty())
        return From;

    for (auto I = From; I != size() - Str.size() + 1; ++I)
        if (traits_type::compare(data() + I, Str.data(), Str.size()) == 0)
            return I;

    return npos;
}

size_t StringRef::find_first_of(StringRef Chars, size_t From) const
{
    if (From >= size() || Chars.empty())
        return npos;

    for (auto I = From; I != size(); ++I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_first_not_of(StringRef Chars, size_t From) const
{
    if (From >= size())
        return npos;

    for (auto I = From; I != size(); ++I)
        if (!traits_type::find(Chars.data(), Chars.size(), data()[I]))
            return I;

    return npos;
}

size_t StringRef::find_last_of(StringRef Chars, size_t From) const
{
    if (Chars.empty())
        return npos;

    if (From < size())
        From++;
    else
        From = size();

    for (auto I = From; I != 0; --I)
        if (traits_type::find(Chars.data(), Chars.size(), data()[I - 1]))
            return I - 1;

    return npos;
}

size_t StringRef::find_last_not_of(StringRef Chars, size_t From) const
{
    if (From < size())
        From++;
    else
        From = size();

    for (auto I = From; I != 0; --I)
        if (!traits_type::find(Chars.data(), Chars.size(), data()[I - 1]))
            return I - 1;

    return npos;
}

StringRef StringRef::trim_left(StringRef Chars) const
{
    return drop_front(find_first_not_of(Chars));
}

StringRef StringRef::trim_right(StringRef Chars) const
{
    auto I = find_last_not_of(Chars);
    return front(I == npos ? npos : I + 1);
}

StringRef StringRef::trim(StringRef Chars) const
{
    return trim_left(Chars).trim_right(Chars);
}
