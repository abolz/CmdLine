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

template <class Delimiter>
class SplitResult
{
    class Iterator;

    // The string to split
    StringRef Str;
    // The delimiters
    Delimiter Delim;
    // Whether to skip empty tokens
    bool SkipEmpty;

public:
    using iterator = Iterator;
    using const_iterator = Iterator;

    // Construct a new splitter
    SplitResult(StringRef str, Delimiter delim, bool skipEmpty = false)
        : Str(str)
        , Delim(std::move(delim))
        , SkipEmpty(skipEmpty)
    {
    }

    // Returns an iterator to the first token
    const_iterator begin() const {
        return { this, Str };
    }

    // Returns the past-the-end iterator
    const_iterator end() const {
        return { nullptr, Str };
    }

    // Construct a container from this range
    template <class T>
    explicit operator T() const
    {
        return T(begin(), end());
    }

    // Returns the current token and the rest of the string.
    std::pair<StringRef, StringRef> operator ()() const
    {
        auto I = begin();
        return { I.Tok, I.Str };
    }

private:
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
        // Construct a new iterator
        Iterator(SplitResult const* split, StringRef str)
            : Split(split)
            , Tok(str.front(0))
            , Str(str)
        {
            if (Split)
                Split->increment(*this, true/*first*/);
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

            Split->increment(*this);
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

            if (!LHS.Split && !RHS.Split)
                return true;
            if (!LHS.Split || !RHS.Split)
                return false;

            return LHS.Tok.begin() == RHS.Tok.begin();
        }

        // Returns whether the given iterators are inequal
        friend bool operator!=(Iterator const& LHS, Iterator const& RHS) {
            return !(LHS == RHS);
        }
    };

    void inc(Iterator& I, StringRef Sep) const
    {
        assert(I.Str.begin() <= Sep.begin() && Sep.end() <= I.Str.end());

        if (Sep.begin() == I.Str.end())
        {
            // There is no further delimiter.
            // The current string is the last token.
            I.Tok = I.Str;
        }
        else
        {
            // Delimiter found.
            // Split the string at the specified position.
            I.Tok = { I.Str.begin(), Sep.begin() };
            I.Str = { Sep.end(), I.Str.end() };
        }
    }

    void inc(Iterator& I, std::pair<size_t, size_t> Sep) const
    {
        // ?? return inc(I, I.Str.substr(Sep.first, Sep.second)) ??

        /*if (Sep.first == StringRef::npos && Sep.second == StringRef::npos)
        {
            // Stop iteration, now.
            // Set the iterator to the past-the-end iterator.
            I.Split = nullptr;
        }
        else*/ if (Sep.first == StringRef::npos)
        {
            // There is no further delimiter.
            // The current string is the last token.
            I.Tok = I.Str;
        }
        else
        {
            assert(Sep.first <= I.Str.size());
            assert(Sep.second <= I.Str.size() - Sep.first);

            // Delimiter found.
            // Split the string at the specified position.
            I.Tok = I.Str.front(Sep.first);
            I.Str = I.Str.drop_front(Sep.first).drop_front(Sep.second);
        }
    }

    //void inc(Iterator& I, std::pair<StringRef, StringRef> const& Next) const
    //{
    //    I.Tok = Next.first;
    //    I.Str = Next.second;
    //}

    void increment0(Iterator& I, bool first) const
    {
        assert(I.Split);

        if (!first && I.Tok.end() == I.Str.end())
        {
            assert(I.Tok.begin() == I.Str.begin());

            // The current string is the last token.
            // Set the iterator to the past-the-end iterator.
            I.Split = nullptr;
        }
        else
        {
            // Find the next token and adjust the iterator.
            inc(I, Delim(I.Str));
        }
    }

    void increment(Iterator& I, bool first = false) const
    {
        // Find the next separator and adjust the iterator.
        increment0(I, first);

        // Skip empty tokens if required.
        if (SkipEmpty)
        {
            while (I.Split && I.Tok.empty())
                increment0(I, false/*first*/);
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
    SplitAnyOf(StringRef Chars) : Chars(Chars)
    {
    }

    std::pair<size_t, size_t> operator()(StringRef Str) const {
        return { Str.find_first_of(Chars), 1 };
    }
};

inline SplitAnyOf any_of(StringRef Chars) {
    return { Chars };
}

//--------------------------------------------------------------------------------------------------
// SplitLiteral
//

class SplitLiteral
{
    StringRef Needle;

public:
    SplitLiteral(StringRef Needle) : Needle(Needle)
    {
    }

    std::pair<size_t, size_t> operator()(StringRef Str) const
    {
        if (Needle.empty())
        {
#if !SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL
            // Return the whole string as a token.
            // Makes literal("") behave exactly as any_of("").
            return { StringRef::npos, 0 };
#else
            return { Str.size() <= 1 ? StringRef::npos : 1, 0 };
#endif
        }

        return { Str.find(Needle), Needle.size() };
    }
};

inline SplitLiteral literal(StringRef Str) {
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

    std::pair<size_t, size_t> operator()(StringRef Str) const
    {
        // If the string fits into the current line,
        // just return this last line.
        if (Str.size() <= Len)
            return { StringRef::npos, 0 };

        // Search for the first space preceding the line length.
        auto pos = Str.find_last_of(Spaces, Len);

        if (pos != StringRef::npos)
            return { pos, 1 }; // There is a space.

        // No space in current line, break at Len.
        return { Len, 0 };
    }
};

inline SplitWrap wrap(size_t Width, StringRef Spaces = " ") {
    return { Width, Spaces };
}

//--------------------------------------------------------------------------------------------------
// split
//

template <class Delimiter>
inline SplitResult<Delimiter> split(StringRef Str, Delimiter D, bool SkipEmpty = false)
{
    return { Str, std::move(D), SkipEmpty };
}

inline SplitResult<SplitLiteral> split(StringRef Str, std::string const& Chars, bool SkipEmpty = false) {
    return split(Str, literal(Chars), SkipEmpty);
}

inline SplitResult<SplitLiteral> split(StringRef Str, StringRef Chars, bool SkipEmpty = false) {
    return split(Str, literal(Chars), SkipEmpty);
}

inline SplitResult<SplitLiteral> split(StringRef Str, char const* Chars, bool SkipEmpty = false) {
    return split(Str, literal(Chars), SkipEmpty);
}

} // namespace strings
} // namespace support
