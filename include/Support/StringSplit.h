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
    // The rest of the string to split
    StringRef Str;
    // The delimiters
    Splitter Split;
    // The number of tokens an iterator for this range might produce
    int MaxCount;
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
        // FIXME: use just an offset into Split->Str? saves a char*
        StringRef Str;
        // The number of tokens this iterator might produce
        int Count;

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
            , Count(0)
        {
        }

        // Construct a new iterator
        // SplitResult must be non-null!
        explicit Iterator(SplitResult const* Split, int MaxCount)
            : Split(Split)
            , Tok()
            , Str(Split->Str)
            , Count(MaxCount)
        {
            this->operator ++(); // grab the first token
        }

        // Returns the current token
        reference operator *() const
        {
            assert(Split && "dereferencing past-the-end iterator");
            return Tok;
        }

        // Returns a pointer to the current token
        pointer operator ->() const
        {
            assert(Split && "dereferencing past-the-end iterator");
            return &Tok;
        }

        // Grab the next token
        Iterator& operator ++()
        {
            assert(Split && "incrementing past-the-end iterator");

            if (Count == 0)
            {
                Tok = {};
                Str = {};
            }
            else
            {
                Split->next(Tok, Str);

                if (Count > 0)
                    Count--;
            }

            if (Tok.data() == nullptr)
                Split = nullptr;

            return *this;
        }

        // Grab the next token
        Iterator operator ++(int)
        {
            auto I = *this;
            this->operator ++();
            return I;
        }

        // Returns whether the given iterators are equal
        friend bool operator ==(Iterator const& LHS, Iterator const& RHS)
        {
            assert((!LHS.Split || LHS.Tok.data()) && "internal error");
            assert((!RHS.Split || RHS.Tok.data()) && "internal error");
            assert((!LHS.Split || !RHS.Split || LHS.Split == RHS.Split) && "invalid comparison");

            return LHS.Tok.data() == RHS.Tok.data();
        }

        // Returns whether the given iterators are inequal
        friend bool operator !=(Iterator const& LHS, Iterator const& RHS) {
            return !(LHS == RHS);
        }
    };

public:
    using iterator = Iterator;
    using const_iterator = Iterator;

public:
    // Construct a new splitter
    SplitResult(StringRef S, Splitter D, bool SkipEmpty = false, int MaxCount = -1)
        : Str(S)
        , Split(std::move(D))
        , MaxCount(MaxCount)
        , SkipEmpty(SkipEmpty)
    {
        static const StringRef kEmpty = "";

        // Make Str non-null!
        // NOTE: assert(StringRef() == "")
        if (Str.empty())
            Str = kEmpty;
    }

    //----------------------------------------------------------------------------------------------
    // Range implementation
    //

    // Returns an iterator to the first token
    const_iterator begin() const {
        return iterator(this, MaxCount);
    }

    // Returns the past-the-end iterator
    const_iterator end() const {
        return {};
    }

    // Construct a container from this range
    template <class T>
    explicit operator T() const { return T(begin(), end()); }

    //----------------------------------------------------------------------------------------------
    // Generator implementation
    //

    std::pair<StringRef, StringRef> operator ()() const
    {
        auto I = begin();
        return {I.Tok, I.Str};
    }

private:
    void next(StringRef& T, StringRef& S) const
    {
        do
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
                T = {S.begin(), Sep.begin()};
                S = {Sep.end(), S.end()};
            }
        }
        while (SkipEmpty && (T.empty() && T.data() != nullptr));
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

    StringRef operator ()(StringRef Str) const {
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

    StringRef operator ()(StringRef Str) const
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
// split
//

template <class Splitter>
inline SplitResult<Splitter>
split(StringRef Str, Splitter D, bool SkipEmpty = false, int MaxCount = -1)
{
    return { Str, std::move(D), SkipEmpty, MaxCount };
}

inline SplitResult<SplitLiteral>
split(StringRef Str, std::string const& Chars, bool SkipEmpty = false, int MaxCount = -1)
{
    return split(Str, literal(Chars), SkipEmpty, MaxCount);
}

inline SplitResult<SplitLiteral>
split(StringRef Str, StringRef Chars, bool SkipEmpty = false, int MaxCount = -1)
{
    return split(Str, literal(Chars), SkipEmpty, MaxCount);
}

inline SplitResult<SplitLiteral>
split(StringRef Str, char const* Chars, bool SkipEmpty = false, int MaxCount = -1)
{
    return split(Str, literal(Chars), SkipEmpty, MaxCount);
}

} // namespace strings
} // namespace support
