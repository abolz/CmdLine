// This file is distributed under the MIT license.
// See the LICENSE file for details.


#pragma once


#include <cassert>
#include <functional>
#include <ostream>
#include <string>
#include <utility>


namespace support
{


class StringRef
{
public:
    using char_type = char;

    using const_reference   = char_type const&;
    using const_pointer     = char_type const*;
    using const_iterator    = char_type const*;

    using traits_type = std::char_traits<char_type>;

private:
    // The string data - an external buffer
    const_pointer Data;
    // The length of the string
    size_t Length;

    // Workaround: traits_type::compare is undefined for null pointers even if MaxCount is 0.
    static int Compare(const_pointer LHS, const_pointer RHS, size_t MaxCount) {
        return MaxCount == 0 ? 0 : traits_type::compare(LHS, RHS, MaxCount);
    }

    static size_t Min(size_t x, size_t y) { return x < y ? x : y; }
    static size_t Max(size_t x, size_t y) { return x > y ? x : y; }

public:
    static size_t const npos = static_cast<size_t>(-1);

public:
    // Construct an empty StringRef.
    StringRef()
        : Data(nullptr)
        , Length(0)
    {
    }

    // Construct a StringRef from a pointer and a length.
    StringRef(const_pointer Data, size_t Length)
        : Data(Data)
        , Length(Length)
    {
        assert( (Data || Length == 0) && "constructing from a nullptr and a non-zero length" );
    }

    // Construct a StringRef from a C-string.
    StringRef(const_pointer Str)
        : Data(Str)
        , Length(Str ? traits_type::length(Str) : 0)
    {
    }

    // Construct from two iterators
    StringRef(const_iterator Begin, const_iterator End)
        : Data(Begin)
        , Length(End - Begin)
    {
        assert( (Begin ? Begin <= End : !End) && "invalid iterators" );
    }

    // Construct a StringRef from a std::string.
    StringRef(std::string const& Str)
        : Data(Str.data())
        , Length(Str.size())
    {
    }

    // Returns a pointer to the start of the string.
    // Note: The string may not be null-terminated.
    const_pointer data() const {
        return Data;
    }

    // Returns the length of the string.
    size_t size() const {
        return Length;
    }

    // Returns whether this string is null or empty.
    bool empty() const {
        return Length == 0;
    }

    // Returns whether this string is null.
    bool null() const {
        return Data == nullptr;
    }

    // Returns an iterator to the first element of the string.
    const_iterator begin() const {
        return Data;
    }

    // Returns an iterator to one element past the last element of the string.
    const_iterator end() const {
        return Data + Length;
    }

    // Array access.
    const_reference operator [](size_t Index) const
    {
        assert( Index < size() && "index out of range" );
        return Data[Index];
    }

    // Returns the first character of the string.
    const_reference front() const
    {
        assert( !empty() && "index out of range" );
        return data()[0];
    }

    // Returns the last character of the string.
    const_reference back() const
    {
        assert( !empty() && "index out of range" );
        return data()[size() - 1];
    }

    // Returns the first N characters of the string.
    StringRef front(size_t N) const
    {
        N = Min(N, size());
        return {data(), N};
    }

    // Removes the first N characters from the string.
    StringRef dropFront(size_t N) const
    {
		N = Min(N, size());
        return {data() + N, size() - N};
    }

    // Returns the last N characters of the string.
    StringRef back(size_t N) const
    {
		N = Min(N, size());
        return {data() + (size() - N), N};
    }

    // Removes the last N characters from the string.
    StringRef dropBack(size_t N) const
    {
		N = Min(N, size());
        return {data(), size() - N};
    }

    // Returns the substring [First, Last).
    StringRef slice(size_t First, size_t Last = npos) const {
        return front(Last).dropFront(First);
    }

    // Returns the sub-string [First, First + Count).
    StringRef substr(size_t First, size_t Count = npos) const {
        return dropFront(First).front(Count);
    }

    // Returns whether this string is equal to another.
    bool equals(StringRef RHS) const {
        return size() == RHS.size() && 0 == Compare(data(), RHS.data(), RHS.size());
    }

    // Lexicographically compare this string with another.
    bool less(StringRef RHS) const
    {
		int c = Compare(data(), RHS.data(), Min(size(), RHS.size()));
        return c < 0 || (c == 0 && size() < RHS.size());
    }

    // Returns whether the string starts with Prefix
    bool startsWith(StringRef Prefix) const
    {
        return size() >= Prefix.size()
            && 0 == Compare(data(), Prefix.data(), Prefix.size());
    }

    // Returns whether the string ends with Suffix
    bool endsWith(StringRef Suffix) const
    {
        return size() >= Suffix.size()
            && 0 == Compare(data() + (size() - Suffix.size()), Suffix.data(), Suffix.size());
    }

    // Constructs a std::string from this StringRef.
    std::string str() const {
        return empty() ? std::string() : std::string(data(), size());
    }

    // Explicitly convert to a std::string
    explicit operator std::string() const { return str(); }

    // Write this string into the given stream
    void write(std::ostream& Stream) const;

    // Search for the first character Ch in the sub-string [From, Length)
    size_t find(char_type Ch, size_t From = 0) const;

    // Search for the first character in the sub-string [From, Length)
    // which matches any of the characters in Chars.
    size_t findFirstOf(StringRef Chars, size_t From = 0) const;

    // Search for the first character in the sub-string [From, Length)
    // which does not match any of the characters in Chars.
    size_t findFirstNotOf(StringRef Chars, size_t From = 0) const;

    // Search for the last character in the sub-string [From, Length)
    // which matches any of the characters in Chars.
    size_t findLastOf(StringRef Chars, size_t From = npos) const;

    // Search for the last character in the sub-string [From, Length)
    // which does not match any of the characters in Chars.
    size_t findLastNotOf(StringRef Chars, size_t From = npos) const;

    // Split into two substrings around the first occurrence of the separator character.
    std::pair<StringRef, StringRef> split(char_type Ch, size_t From = 0) const
    {
        auto I = find(Ch, From);

        if (I == npos)
            return {*this, StringRef()};

        return {front(I), dropFront(I + 1)};
    }

    // Split into two substrings around the first occurrence of any of the separator characters.
    std::pair<StringRef, StringRef> split(StringRef Chars, size_t From = 0) const
    {
        auto I = findFirstOf(Chars, From);

        if (I == npos)
            return {*this, StringRef()};

        return {front(I), dropFront(I + 1)};
    }

    // Split into two substrings at the given position
    std::pair<StringRef, StringRef> splitAt(size_t Pos) const {
        return {front(Pos), dropFront(Pos)};
    }

    // Return string with consecutive characters in Chars starting from the left removed.
    StringRef trimLeft(StringRef Chars = " \t\n\v\f\r") const;

    // Return string with consecutive characters in Chars starting from the right removed.
    StringRef trimRight(StringRef Chars = " \t\n\v\f\r") const;

    // Return string with consecutive characters in Chars starting from the left and right removed.
    StringRef trim(StringRef Chars = " \t\n\v\f\r") const;

    // Split into substrings around the occurrences of a separator string.
    // Each substring is passed to the given function.
    template<class Function>
    bool tokenize(StringRef Separators, Function out) const
    {
        auto rest = *this;

        while (!rest.null())
        {
            auto p = rest.split(Separators);

            if (!out(p.first))
                return false;

            rest = p.second;
        }

        return true;
    }
};


inline bool operator ==(StringRef LHS, StringRef RHS) {
    return LHS.equals(RHS);
}

inline bool operator !=(StringRef LHS, StringRef RHS) {
    return !(LHS == RHS);
}

inline bool operator <(StringRef LHS, StringRef RHS) {
    return LHS.less(RHS);
}

inline bool operator <=(StringRef LHS, StringRef RHS) {
    return !(RHS < LHS);
}

inline bool operator >(StringRef LHS, StringRef RHS) {
    return RHS < LHS;
}

inline bool operator >=(StringRef LHS, StringRef RHS) {
    return !(LHS < RHS);
}


inline std::ostream& operator <<(std::ostream& Stream, StringRef Str) {
    return Stream.write(Str.data(), Str.size());
}


inline std::string& operator +=(std::string& LHS, StringRef RHS) {
    return LHS.append(RHS.data(), RHS.size());
}

inline std::string operator +(StringRef LHS, std::string RHS)
{
    RHS.insert(0, LHS.data(), LHS.size());
    return std::move(RHS);
}

inline std::string operator +(std::string LHS, StringRef RHS)
{
    LHS.append(RHS.data(), RHS.size());
    return std::move(LHS);
}


// Modified Bernstein hash
inline size_t hashValue(StringRef Str, size_t H = 5381)
{
    for (size_t I = 0, E = Str.size(); I != E; ++I)
        H = 33 * H ^ static_cast<unsigned char>(Str[I]);

    return H;
}


} // namespace support


namespace std
{
template<>
struct hash< ::support::StringRef>
{
    size_t operator ()(::support::StringRef Str) const {
        return hashValue(Str);
    }
};
}
