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

    reference operator *()
    {
        assert(R && "dereferencing end() iterator");
        return R->Tok;
    }

    pointer operator ->()
    {
        assert(R && "dereferencing end() iterator");
        return &R->Tok;
    }

    SplitIterator& operator ++()
    {
        assert(R && "incrementing end() iterator");

        if (R->next() == false)
            R = nullptr;

        return *this;
    }

    SplitIterator operator ++(int)
    {
        auto t = *this;
        operator ++();
        return t;
    }

    bool operator ==(SplitIterator const& RHS) const {
        return R == RHS.R;
    }

    bool operator !=(SplitIterator const& RHS) const {
        return R != RHS.R;
    }
};

//--------------------------------------------------------------------------------------------------
// SplitRange
//

template <class StringT, class DelimiterT, class PredicateT>
class SplitRange
{
    template <class> friend class SplitIterator;

    // The string to split
    StringT Str;
    // The delimiter
    DelimiterT Delim;
    // The predicate
    PredicateT Pred;
    // The current token
    StringRef Tok;
    // The start of the rest of the string
    size_t Pos;

public:
    using iterator = SplitIterator<SplitRange>;
    using const_iterator = SplitIterator<SplitRange>;

public:
    SplitRange(StringT Str_, DelimiterT Delim_, PredicateT Pred_)
        : Str(std::move(Str_))
        , Delim(std::move(Delim_))
        , Pred(std::move(Pred_))
        , Tok(Str)
        , Pos(0)
    {
        next();
    }

    iterator begin() {
        return iterator(this);
    }

    iterator end() {
        return iterator();
    }

//#if !STRINGS_N3593
    template <class T> explicit operator T() { return T(begin(), end()); }
//#endif

//#if !STRINGS_N3593
    // Returns the current token and the rest of the string.
    auto operator ()() const -> std::pair<StringRef, StringRef> {
        return { Tok, StringRef(Str).substr(Pos) };
    }
//#endif

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
        if (Sep.first == StringRef::npos)
        {
            // There is no further delimiter.
            // The current string is the last token.
            Tok = StringRef(Str.data() + Pos, Str.size() - Pos);
            Pos = StringRef::npos;
        }
        else
        {
            // Delimiter found.
            Tok = StringRef(Str.data() + Pos, Sep.first);
            Pos = Pos + Sep.first + Sep.second;
        }
    }

    bool increment()
    {
        increment(Delim({ Str.data() + Pos, Str.size() - Pos }));
        return true;
    }

    bool next()
    {
        do {
            if (Pos == StringRef::npos)
            {
                // The current string is the last token.
                // Set the iterator to the past-the-end iterator.
                return false;
            }
            else
            {
                // Find the next token and adjust the iterator.
                increment();
            }
        } while (!Pred(Tok));

        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// KeepEmpty
//

struct KeepEmpty
{
    bool operator ()(StringRef Tok) const {
        return true;
    }
};

//--------------------------------------------------------------------------------------------------
// SkipEmpty
//

struct SkipEmpty
{
    bool operator ()(StringRef Tok) const {
        return !Tok.empty();
    }
};

//--------------------------------------------------------------------------------------------------
// SkipSpace
//

struct SkipSpace
{
    bool operator ()(StringRef Tok) const {
        return !Tok.trim().empty();
    }
};

//--------------------------------------------------------------------------------------------------
// Trim
//

struct Trim
{
    bool operator ()(StringRef& Tok) const
    {
        Tok = Tok.trim();
        return !Tok.empty();
    }
};

//--------------------------------------------------------------------------------------------------
// AnyOfDelimiter
//

struct AnyOfDelimiter
{
    StringRef Chars;

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

struct LiteralDelimiter
{
    StringRef Needle;

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

    //
    // From N3593:
    //
    // The default delimiter when not explicitly specified is std::literal_delimiter
    //

    template <class T>
    inline auto TestDelimiter( T )
        -> T;
    inline auto TestDelimiter( std::string )
        -> LiteralDelimiter;
    inline auto TestDelimiter( StringRef )
        -> LiteralDelimiter;
    inline auto TestDelimiter( char const* )
        -> LiteralDelimiter;

    template <class T>
    using DelimiterType = decltype(TestDelimiter(std::declval<T>()));
}

template <class S, class D, class P = KeepEmpty>
auto split(S&& Str, D Delim, P Pred = P())
    -> SplitRange<details::StoredType<S>, details::DelimiterType<D>, P>
{
    return { std::forward<S>(Str), details::DelimiterType<D>(std::move(Delim)), std::move(Pred) };
}

} // namespace strings
} // namespace support
