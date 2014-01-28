// This file is distributed under the MIT license.
// See the LICENSE file for details.

// http://isocpp.org/files/papers/n3593.html

#pragma once

#include "Support/StringRef.h"

#include <cstddef>
#include <iterator>
#include <utility>

namespace support
{
namespace strings
{

//--------------------------------------------------------------------------------------------------
// SplitIterator
//

template <class SplitRangeT>
class SplitIterator
{
    SplitRangeT* R;

public:
    using iterator_category     = std::input_iterator_tag;
    using value_type            = StringRef;
    using reference             = StringRef const&;
    using pointer               = StringRef const*;
    using difference_type       = ptrdiff_t;

public:
    SplitIterator(SplitRangeT* R_ = nullptr)
        : R(R_)
    {
    }

    auto operator *() -> reference
    {
        assert(R && "dereferencing end() iterator");
        return R->Tok;
    }

    auto operator ->() -> pointer
    {
        assert(R && "dereferencing end() iterator");
        return &R->Tok;
    }

    auto operator ++() -> SplitIterator&
    {
        assert(R && "incrementing end() iterator");

        if (R->next() == false)
            R = nullptr;

        return *this;
    }

    auto operator ++(int) -> SplitIterator
    {
        auto t = *this;
        operator ++();
        return t;
    }

    auto operator ==(SplitIterator const& RHS) const -> bool {
        return R == RHS.R;
    }

    auto operator !=(SplitIterator const& RHS) const -> bool {
        return R != RHS.R;
    }
};

//--------------------------------------------------------------------------------------------------
// SplitRange
//

template <class StringT, class DelimiterT>
class SplitRange
{
    template <class> friend class SplitIterator;

    // The string to split
    StringT Str;
    // The delimiters
    DelimiterT Delim;
    // The current token
    StringRef Tok;
    // The start of the rest of the string
    size_t Pos;

public:
    using iterator = SplitIterator<SplitRange>;
    using const_iterator = SplitIterator<SplitRange>;

public:
    SplitRange(StringT Str_, DelimiterT Delim_)
        : Str(std::move(Str_))
        , Delim(std::move(Delim_))
        , Tok(Str)
        , Pos(0)
    {
        increment();
    }

    auto begin() -> iterator {
        return iterator(this);
    }

    auto end() -> iterator {
        return iterator();
    }

//#if !STRINGS_N3593
    template <class T> explicit operator T() { return T(begin(), end()); }
//#endif

    // Returns the current token and the rest of the string.
    auto operator ()() const -> std::pair<StringRef, StringRef> {
        return { Tok, StringRef(Str).substr(Pos) };
    }

private:
    //
    // From N3593:
    //
    // The result of a Delimiter's find() member function must be a std::StringRef referring to
    // one of the following:
    //
    // -    A substring of find()'s argument text referring to the delimiter/separator that was
    //      found.
    // -    An empty std::StringRef referring to find()'s argument's end iterator, (e.g.,
    //      std::StringRef(input_text.end(), 0)). This indicates that the delimiter/separator was
    //      not found.
    //
    // [Footnote: An alternative to having a Delimiter's find() function return a std::StringRef
    // is to instead have it return a std::pair<size_t, size_t> where the pair's first member is the
    // position of the found delimiter, and the second member is the length of the found delimiter.
    // In this case, Not Found could be prepresented as std::make_pair(std::StringRef::npos, 0).
    // ---end footnote]
    //

    void increment(std::pair<size_t, size_t> Sep)
    {
        auto S = StringRef(Str).substr(Pos);

        if (Sep.first == StringRef::npos)
        {
            // There is no further delimiter.
            // The current string is the last token.
            Tok = S;
            Pos = StringRef::npos;
        }
        else
        {
            assert(Sep.first + Sep.second >= Sep.first);
            assert(Pos + Sep.first + Sep.second > Pos);

            // Delimiter found.
            Tok = S.substr(0, Sep.first);
            Pos = Pos + Sep.first + Sep.second;
        }
    }

    bool increment()
    {
        increment(Delim(StringRef(Str).substr(Pos)));
        return true;
    }

    bool next()
    {
        if (Pos == StringRef::npos)
        {
            // The current string is the last token.
            // Set the iterator to the past-the-end iterator.
            return false;
        }
        else
        {
            // Find the next token and adjust the iterator.
            return increment();
        }
    }
};

//--------------------------------------------------------------------------------------------------
// AnyOfDelimiter
//

class AnyOfDelimiter
{
    StringRef Chars;

public:
    explicit AnyOfDelimiter(StringRef Chars_)
        : Chars(Chars_)
    {
    }

    auto operator ()(StringRef Str) const -> std::pair<size_t, size_t> {
        return { Str.find_first_of(Chars), 1 };
    }
};

//--------------------------------------------------------------------------------------------------
// LiteralDelimiter
//

class LiteralDelimiter
{
    StringRef Needle;

public:
    explicit LiteralDelimiter(StringRef Needle_)
        : Needle(Needle_)
    {
    }

    auto operator ()(StringRef Str) const -> std::pair<size_t, size_t>
    {
        if (Needle.empty())
        {
#if SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL
            //
            // From N3593:
            //
            // A delimiter of the empty string results in each character in the input string
            // becoming one element in the output collection. This is a special case. It is done to
            // match the behavior of splitting using the empty string in other programming languages
            // (e.g., perl).
            //
            return { Str.size() <= 1 ? StringRef::npos : 1, 0 };
#else
            //
            // Return the whole string as a token.
            // Makes LiteralDelimiter("") behave exactly as AnyOfDelimiter("").
            //
            return { StringRef::npos, 0 };
#endif
        }

        return { Str.find(Needle), Needle.size() };
    }
};

//--------------------------------------------------------------------------------------------------
// WrapDelimiter
//

class WrapDelimiter
{
    // The maximum length of a single line
    size_t Len;
    // The list of separators
    StringRef Spaces;

public:
    explicit WrapDelimiter(size_t Len_, StringRef Spaces_ = " ")
        : Len(Len_)
        , Spaces(Spaces_)
    {
        assert(Len > 0);
    }

    auto operator()(StringRef Str) const -> std::pair<size_t, size_t>
    {
        // If the string fits into the current line, just return this last line.
        if (Str.size() <= Len)
            return { StringRef::npos, 0 };

        // Otherwise, search for the first space preceding the line length.
        auto Pos = Str.find_last_of(Spaces, Len);

        if (Pos != StringRef::npos)
            return { Pos, 1 }; // There is a space.
        else
            return { Len, 0 }; // No space in current line, break at Len.
    }
};

//--------------------------------------------------------------------------------------------------
// split
//

namespace details
{
    //
    // From N3593:
    //
    // Rvalue support
    //
    // As described so far, std::split() may not work correctly if splitting a std::StringRef that
    // refers to a temporary string. In particular, the following will not work:
    //
    //      for (std::StringRef s : std::split(GetTemporaryString(), "-")) {
    //          s now refers to a temporary string that is no longer valid.
    //      }
    //
    // To address this, std::split() will move ownership of rvalues into the Range object that is
    // returned from std::split().
    //

    inline auto TestStoredType( std::string&& )
        -> std::string;
    inline auto TestStoredType( std::string const& )
        -> StringRef;
    inline auto TestStoredType( StringRef )
        -> StringRef;
    inline auto TestStoredType( char const* )
        -> StringRef;

    template <class T>
    using StoredType = decltype(TestStoredType(std::declval<T>()));
}

template <class StringT, class DelimiterT>
auto split(StringT&& S, DelimiterT&& D)
    -> SplitRange<details::StoredType<StringT>, typename std::decay<DelimiterT>::type>
{
    return { std::forward<StringT>(S), std::forward<DelimiterT>(D) };
}

template <class StringT>
auto split(StringT&& S, std::string D)
    -> SplitRange<details::StoredType<StringT>, LiteralDelimiter>
{
    return split(std::forward<StringT>(S), LiteralDelimiter(std::move(D)));
}

template <class StringT>
auto split(StringT&& S, StringRef D)
    -> SplitRange<details::StoredType<StringT>, LiteralDelimiter>
{
    return split(std::forward<StringT>(S), LiteralDelimiter(D));
}

template <class StringT>
auto split(StringT&& S, char const* D)
    -> SplitRange<details::StoredType<StringT>, LiteralDelimiter>
{
    return split(std::forward<StringT>(S), LiteralDelimiter(D));
}

} // namespace strings
} // namespace support
