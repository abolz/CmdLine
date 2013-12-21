// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"

#include <cstddef>
#include <iterator>

namespace support
{
namespace strings
{

//--------------------------------------------------------------------------------------------------
// SplitResult
//

template <class Splitter>
class SplitResult
{
    // The string to split
    StringRef Str;
    // The delimiters
    Splitter Split;
    // Whether to skip empty tokens
    bool SkipEmpty;

    class Iterator
    {
        friend class SplitResult;

        // The splitter
        SplitResult const* Split;
        // The current token
        StringRef Tok;
        // The rest of the string to split
        StringRef Str;

    public:
        using iterator_category     = std::forward_iterator_tag;
        using value_type            = StringRef;
        using reference             = value_type const&;
        using pointer               = value_type const*;
        using difference_type       = ptrdiff_t;

    public:
        // Construct the past-the-end iterator
        Iterator()
            : Split(nullptr)
            , Tok()
            , Str()
        {
        }

        // Construct a new iterator
        // SplitResult must be non-null!
        Iterator(SplitResult const* Split)
            : Split(Split)
            , Tok()
            , Str(Split->Str)
          {
            this->operator++(); // grab the first token
        }

        // Returns the current token
        reference operator*() const
        {
            assert(Split && "dereferencing past-the-end iterator");
            return Tok;
        }

        // Returns a pointer to the current token
        pointer operator->() const
        {
            assert(Split && "dereferencing past-the-end iterator");
            return &Tok;
        }

        // Grab the next token
        Iterator& operator++()
        {
            assert(Split && "incrementing past-the-end iterator");

            Split->next(Tok, Str);

            if (Tok.data() == nullptr)
                Split = nullptr;

            return *this;
        }

        // Grab the next token
        Iterator operator++(int)
        {
            auto I = *this;
            this->operator ++();
            return I;
        }

        // Returns whether the given iterators are equal
        friend bool operator==(Iterator const& LHS, Iterator const& RHS)
        {
            assert((!LHS.Split || !RHS.Split || LHS.Split == RHS.Split) && "invalid comparison");

            return LHS.Tok.data() == RHS.Tok.data();
        }

        // Returns whether the given iterators are inequal
        friend bool operator!=(Iterator const& LHS, Iterator const& RHS) {
            return !(LHS == RHS);
        }
    };

public:
    using iterator = Iterator;
    using const_iterator = Iterator;

public:
    // Construct a new splitter
    SplitResult(StringRef S, Splitter D, bool SkipEmpty = false)
        : Str(S)
        , Split(std::move(D))
        , SkipEmpty(SkipEmpty)
    {
        static const StringRef kEmpty = "";

        // Make Str non-null!
        // NOTE: assert(StringRef() == "")
        if (Str.data() == nullptr)
            Str = kEmpty;
    }

    // Returns an iterator to the first token
    const_iterator begin() const {
        return { this };
    }

    // Returns the past-the-end iterator
    const_iterator end() const {
        return {};
    }

    // Construct a container from this range
    template <class T>
    explicit operator T() const { return T(begin(), end()); }

    std::pair<StringRef, StringRef> operator ()() const
    {
        auto I = begin();
        return { I.Tok, I.Str };
    }

private:
    void parse(StringRef& T, StringRef& S) const
    {
        auto Sep = Split(S);

        if (Sep.data() == nullptr)
        {
            // If the splitter returns null, stop immediately.
            T = {};
            S = {};
        }
        else if (Sep.begin() == S.end())
        {
            assert(Sep.empty());

            // If the splitter returns an empty separator beginning at the end
            // of the current string, use the current string as the next token
            // and stop on the next iteration.
            T = S;
            S = {};
        }
        else
        {
            assert(S.begin() <= Sep.begin());
            assert(Sep.begin() <= Sep.end());
            assert(Sep.end() <= S.end());

            // Otherwise, split out the separator from the current string.
            // NOTE: Sep may be empty!
            T = { S.begin(), Sep.begin() };
            S = { Sep.end(), S.end() };
        }
    }

    void next(StringRef& T, StringRef& S) const
    {
        parse(T, S);

        while (SkipEmpty && (T.empty() && T.data() != nullptr))
        {
            parse(T, S);
        }
    }
};

//--------------------------------------------------------------------------------------------------
// SplitAnyOf
//

class SplitAnyOf
{
    StringRef Chars;

public:
    SplitAnyOf(StringRef Chars = {})
        : Chars(Chars)
    {
    }

    StringRef operator()(StringRef Str) const {
        return Str.substr(Str.find_first_of(Chars), 1);
    }
};

inline SplitAnyOf any_of(StringRef Chars)
{
    return { Chars };
}

//--------------------------------------------------------------------------------------------------
// SplitLiteral
//

class SplitLiteral
{
    StringRef Needle;

public:
    SplitLiteral(StringRef Needle = {})
        : Needle(Needle)
    {
    }

    StringRef operator()(StringRef Str) const
    {
        // Handle empty needles
        if (Needle.empty())
        {
#if !defined(SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL) || (SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL == 0)
            // Returns the whole string as a token.
            // Makes literal("") behave exactly as any_of("").
            return Str.back(0);
#else
            if (Str.empty())
                return {}; // Empty needle, empty haystack: Stop immediately
            else
                return Str.substr(1,0); // Split after the first character.
#endif
        }

        return Str.substr(Str.find(Needle), Needle.size());
    }
};

inline SplitLiteral literal(StringRef Str)
{
    return { Str };
}

//--------------------------------------------------------------------------------------------------
// SplitWrap
//

class SplitWrap
{
    // The maximum length of a single line
    size_t Len;
    // The list of separators
    StringRef Spaces;

public:
    SplitWrap(size_t Len, StringRef Spaces)
        : Len(Len)
        , Spaces(Spaces)
    {
        assert(Len > 0);
    }

    StringRef operator()(StringRef Str) const
    {
        // If the string fits into the current line,
        // just return this last line.
        if (Str.size() <= Len)
            return Str.back(0);

        // Search for the first space preceding the line length.
        auto pos = Str.find_last_of(Spaces, Len);

        if (pos != StringRef::npos)
            return Str.substr(pos, 1); // There is a space.
        else
            return Str.substr(Len, 0); // No space in current line, break at Len.
    }
};

inline SplitWrap wrap(size_t Width, StringRef Spaces = " \n")
{
    return SplitWrap(Width, Spaces);
}

//--------------------------------------------------------------------------------------------------
// split
//

template <class Splitter>
inline SplitResult<Splitter> split(StringRef Str, Splitter D, bool SkipEmpty = false)
{
    return { Str, std::move(D), SkipEmpty };
}

inline SplitResult<SplitLiteral> split(StringRef Str, std::string const& Chars, bool SkipEmpty = false)
{
    return split(Str, literal(Chars), SkipEmpty);
}

inline SplitResult<SplitLiteral> split(StringRef Str, StringRef Chars, bool SkipEmpty = false)
{
    return split(Str, literal(Chars), SkipEmpty);
}

inline SplitResult<SplitLiteral> split(StringRef Str, char const* Chars, bool SkipEmpty = false)
{
    return split(Str, literal(Chars), SkipEmpty);
}

} // namespace strings
} // namespace support
