// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"

#include <cstdint>

namespace support
{
namespace utf
{

namespace details
{
    inline bool isValidCodepoint(uint32_t U)
    {
        // Characters with values greater than 0x10FFFF cannot be encoded in UTF-16.
        if (U > 0x10FFFF)
            return false;

        // Values between 0xD800 and 0xDFFF are specifically reserved for
        // use with UTF-16, and don't have any characters assigned to them.
        if (0xD800 <= U && U <= 0xDFFF)
            return false;

        return true;
    }

    inline bool isContinuationByte(uint8_t b)
    {
        return 0x80 == (b & 0xC0);
    }

    inline uint32_t getUTF8SequenceLength(uint32_t U)
    {
        assert(isValidCodepoint(U) && "invalid code-point");

        if (U <= 0x7F)
            return 1;
        if (U <= 0x7FF)
            return 2;
        if (U <= 0xFFFF)
            return 3;
        if (U <= 0x10FFFF)
            return 4;

        return 0;
    }
}

template <class OutputIterator>
bool encodeUTF8Sequence(uint32_t U, OutputIterator& out)
{
    if (!details::isValidCodepoint(U))
        return false;

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Encoding a character to UTF-8 proceeds as follows:
    //
    // 1.  Determine the number of octets required from the character number
    // and the first column of the table above.  It is important to note
    // that the rows of the table are mutually exclusive, i.e., there is
    // only one valid way to encode a given character.
    //
    // 2.  Prepare the high-order bits of the octets as per the second
    // column of the table.
    //
    // 3.  Fill in the bits marked x from the bits of the character number,
    // expressed in binary.  Start by putting the lowest-order bit of
    // the character number in the lowest-order position of the last
    // octet of the sequence, then put the next higher-order bit of the
    // character number in the next higher-order position of that octet,
    // etc.  When the x bits of the last octet are filled in, move on to
    // the next to last octet, then to the preceding one, etc. until all
    // x bits are filled in.
    //

    switch (details::getUTF8SequenceLength(U))
    {
    case 1:
        *out++ = static_cast<uint8_t>(U & 0x7F);
        return true;

    case 2:
        *out++ = static_cast<uint8_t>(0xC0 | ((U >> 6) & 0x1F));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 0) & 0x3F));
        return true;

    case 3:
        *out++ = static_cast<uint8_t>(0xE0 | ((U >> 12) & 0x0F));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 6) & 0x3F));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 0) & 0x3F));
        return true;

    case 4:
        *out++ = static_cast<uint8_t>(0xF0 | ((U >> 18) & 0x07));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 12) & 0x3F));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 6) & 0x3F));
        *out++ = static_cast<uint8_t>(0x80 | ((U >> 0) & 0x3F));
        return true;

    default:
        return false;
    }
}

template <class Iterator>
bool decodeUTF8Sequence(Iterator& begin, Iterator end, uint32_t& U)
{
    static const uint32_t kSequenceLength[256] = {
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, // 0xxxxxxx
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1, 1,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0,
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, // 110xxxxx
        2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2, 2,
        3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, 3, // 1110xxxx
        4, 4, 4, 4, 4, 4, 4, 4, 0, 0, 0, 0, 0, 0, 0, 0, // 11110xxx
    };

    static const uint32_t kMask[] = { 0 /*unused*/, 0x7F, 0x1F, 0x0F, 0x07 };

    if (begin == end)
        return false; // EOS

    //
    // Char. number range  |        UTF-8 octet sequence
    //    (hexadecimal)    |              (binary)
    // --------------------+---------------------------------------------
    // 0000 0000-0000 007F | 0xxxxxxx
    // 0000 0080-0000 07FF | 110xxxxx 10xxxxxx
    // 0000 0800-0000 FFFF | 1110xxxx 10xxxxxx 10xxxxxx
    // 0001 0000-0010 FFFF | 11110xxx 10xxxxxx 10xxxxxx 10xxxxxx
    //
    // Decoding a UTF-8 character proceeds as follows:
    //
    // 1.  Initialize a binary number with all bits set to 0.  Up to 21 bits
    // may be needed.
    //
    // 2.  Determine which bits encode the character number from the number
    // of octets in the sequence and the second column of the table
    // above (the bits marked x).
    //

    auto Len = kSequenceLength[static_cast<uint32_t>(*begin) & 0xFF];

    if (Len == 0)
        return false; // Invalid UTF-8 sequence

    //
    // 3.  Distribute the bits from the sequence to the binary number, first
    // the lower-order bits from the last octet of the sequence and
    // proceeding to the left until no x bits are left.  The binary
    // number is now equal to the character number.
    //

    U = static_cast<uint32_t>(*begin++) & kMask[Len];

    for (auto Remaining = Len - 1; Remaining != 0; --Remaining)
    {
        if (begin == end)
            return false; // Insufficient data

        if (!details::isContinuationByte(*begin))
            return false; // Invalid UTF-8 sequence

        U = (U << 6) | (static_cast<uint32_t>(*begin++) & 0x3F);
    }

    //
    // Implementations of the decoding algorithm above MUST protect against
    // decoding invalid sequences.  For instance, a naive implementation may
    // decode the overlong UTF-8 sequence C0 80 into the character U+0000,
    // or the surrogate pair ED A1 8C ED BE B4 into U+233B4.  Decoding
    // invalid sequences may have security consequences or cause other
    // problems.
    //

    // Check for invalid codepoint
    if (!details::isValidCodepoint(U))
        return false; // Invalid codepoint

    // Check for overlong sequence
    if (Len != details::getUTF8SequenceLength(U))
        return false; // Invalid UTF-8 sequence

    return true;
}

template <class OutputIterator>
bool encodeUTF16Sequence(uint32_t U, OutputIterator& out)
{
    if (!details::isValidCodepoint(U))
        return false;

    //
    // Encoding of a single character from an ISO 10646 character value to
    // UTF-16 proceeds as follows. Let U be the character number, no greater
    // than 0x10FFFF.
    //
    // 1) If U < 0x10000, encode U as a 16-bit unsigned integer and terminate.
    //

    if (U < 0x10000)
    {
        *out++ = static_cast<uint16_t>(U);
        return true;
    }

    //
    // 2) Let U' = U - 0x10000. Because U is less than or equal to 0x10FFFF,
    // U' must be less than or equal to 0xFFFFF. That is, U' can be
    // represented in 20 bits.
    //
    // 3) Initialize two 16-bit unsigned integers, W1 and W2, to 0xD800 and
    // 0xDC00, respectively. These integers each have 10 bits free to
    // encode the character value, for a total of 20 bits.
    //
    // 4) Assign the 10 high-order bits of the 20-bit U' to the 10 low-order
    // bits of W1 and the 10 low-order bits of U' to the 10 low-order
    // bits of W2. Terminate.
    //

    auto Up = U - 0x10000;

    *out++ = static_cast<uint16_t>(0xD800 + ((Up >> 10) & 0x3FF));
    *out++ = static_cast<uint16_t>(0xDC00 + ((Up >> 0) & 0x3FF));

    return true;
}

template <class Iterator>
bool decodeUTF16Sequence(Iterator& begin, Iterator end, uint32_t& U)
{
    if (begin == end)
        return false; // EOS

    //
    // Decoding of a single character from UTF-16 to an ISO 10646 character
    // value proceeds as follows. Let W1 be the next 16-bit integer in the
    // sequence of integers representing the text. Let W2 be the (eventual)
    // next integer following W1.
    //

    uint32_t W1 = static_cast<uint32_t>(*begin++);

    //
    // 1) If W1 < 0xD800 or W1 > 0xDFFF, the character value U is the value
    // of W1. Terminate.
    //

    if (W1 < 0xD800 || W1 > 0xDFFF)
    {
        U = W1;
        return true;
    }

    // POST: W1 >= 0xD800 && W1 <= 0xDFFF

    //
    // 2) Determine if W1 is between 0xD800 and 0xDBFF. If not, the sequence
    // is in error and no valid character can be obtained using W1.
    // Terminate.
    //

    if (W1 > 0xDBFF)
        return false; // Invalid UTF16 sequence

    //
    // 3) If there is no W2 (that is, the sequence ends with W1), or if W2
    // is not between 0xDC00 and 0xDFFF, the sequence is in error.
    // Terminate.
    //

    if (begin == end)
        return false; // Insufficient data

    uint32_t W2 = static_cast<uint32_t>(*begin++);

    if (W2 < 0xDC00 || W2 > 0xDFFF)
        return false; // Invalid UTF16 sequence

    //
    // 4) Construct a 20-bit unsigned integer U', taking the 10 low-order
    // bits of W1 as its 10 high-order bits and the 10 low-order bits of
    // W2 as its 10 low-order bits.
    //

    uint32_t Up = ((W1 & 0x3FF) << 10) | (W2 & 0x3FF);

    //
    // 5) Add 0x10000 to U' to obtain the character value U. Terminate.
    //

    U = Up + 0x10000;

    return true;
}

template <class InputIterator, class OutputIterator>
bool convertUTF16ToUTF8(InputIterator& begin, InputIterator end, OutputIterator out)
{
    while (begin != end)
    {
        uint32_t U = 0;

        if (!decodeUTF16Sequence(begin, end, U))
            return false;

        if (!encodeUTF8Sequence(U, out))
            return false;
    }

    return true;
}

template <class InputIterator, class OutputIterator>
bool convertUTF8ToUTF16(InputIterator& begin, InputIterator end, OutputIterator out)
{
    while (begin != end)
    {
        uint32_t U = 0;

        if (!decodeUTF8Sequence(begin, end, U))
            return false;

        if (!encodeUTF16Sequence(U, out))
            return false;
    }

    return true;
}

} // namespace utf
} // namespace support
