// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"

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
    // The current token
    StringRef Tok;
    // The delimiters
    Splitter Split;
    // The number of tokens this iterator might produce
    int Count;

public:
    using size_type             = size_t;
    using difference_type       = ptrdiff_t;

    using value_type            = StringRef;
    using reference             = value_type const&;
    using pointer               = value_type const*;
    using const_reference       = value_type const&;
    using const_pointer         = value_type const*;
    using const_iterator        = SplitResult;

    using iterator_category     = std::forward_iterator_tag;

public:
    // Construct the past-the-end iterator
    SplitResult()
    {
    }

    // Construct a new splitter
    SplitResult(StringRef S, Splitter D, int MaxCount = -1)
        : Str(S)
        , Split(std::move(D))
        , Count(MaxCount)
    {
        // Make Str non-null!
        // NOTE: assert(StringRef() == "")
        if (Str.data() == nullptr)
            Str = "";

        this->operator ++(); // grab the first token
    }

    //----------------------------------------------------------------------------------------------
    // Iterator implementation
    //

    // Returns the current token
    const_reference operator *() const {
        return Tok;
    }

    // Returns a pointer to the current token
    const_pointer operator ->() const {
        return &Tok;
    }

    // Grab the next token
    SplitResult& operator ++()
    {
        if (Count == 0)
        {
            assign({}, {});
        }
        else if (Str.empty())
        {
            assign(Str, {});
        }
        else
        {
            assign(Split(Str));

            if (Count > 0)
                Count--;
        }

        return *this;
    }

    // Grab the next token
    SplitResult operator ++(int)
    {
        auto I = *this;
        this->operator ++();
        return I;
    }

    // Returns whether the given iterators are equal
    friend bool operator ==(SplitResult const& LHS, SplitResult const& RHS) {
        return LHS.Tok.data() == RHS.Tok.data();
    }

    // Returns whether the given iterators are inequal
    friend bool operator !=(SplitResult const& LHS, SplitResult const& RHS) {
        return !(LHS == RHS);
    }

    //----------------------------------------------------------------------------------------------
    // Range implementation
    //

    // Returns an iterator to the first token
    const_iterator begin() const {
        return *this;
    }

    // Returns the past-the-end iterator
    const_iterator end() const {
        return {};
    }

    // Returns whether this range is empty
    bool empty() const {
        return Tok.data() == nullptr;
    }

    // Construct a container from this range
    template <class T>
    explicit operator T() const {
        return T(begin(), end());
    }

    //----------------------------------------------------------------------------------------------
    // Generator implementation
    //

    // Returns the current "state" of the splitter
    std::pair<StringRef, StringRef> curr() const {
        return { Tok, Str };
    }

    // Returns the current "state" of the splitter
    std::pair<StringRef, StringRef> operator ()() const
    {
        auto p = curr();
        //this->operator ++();
        return p;
    }

private:
    void assign(std::pair<size_t, size_t> p)
    {
        if (p.first == StringRef::npos)
            assign(Str, {});
        else
            assign(Str.front(p.first), Str.drop_front(p.first + p.second));
    }

    void assign(std::pair<StringRef, StringRef> p) {
        assign(p.first, p.second);
    }

    void assign(StringRef tok, StringRef str)
    {
#ifndef NDEBUG
        auto len = Str.size();
#endif

        Tok = tok;
        Str = str;

#ifndef NDEBUG
        // Tok == null implies Str == null
        assert(Tok.data() || Str.data() == 0);
        // Str must get smaller...
        assert(len == 0 || Str.size() < len);
#endif
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

    std::pair<size_t, size_t> operator ()(StringRef Str) const {
        return { Str.find_first_of(Chars), 1 };
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

    std::pair<size_t, size_t> operator ()(StringRef Str) const {
        return { Str.find(Needle), Needle.size() };
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
inline SplitResult<Splitter> split(StringRef Str, Splitter D, int MaxCount = -1)
{
    return { Str, std::move(D), MaxCount };
}

inline SplitResult<SplitLiteral> split(StringRef Str, std::string const& Chars, int MaxCount = -1)
{
    return split(Str, literal(Chars), MaxCount);
}

inline SplitResult<SplitLiteral> split(StringRef Str, StringRef Chars, int MaxCount = -1)
{
    return split(Str, literal(Chars), MaxCount);
}

inline SplitResult<SplitLiteral> split(StringRef Str, char const* Chars, int MaxCount = -1)
{
    return split(Str, literal(Chars), MaxCount);
}

} // namespace strings
} // namespace support
