// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/StringRefStream.h"

#include <algorithm>

using namespace support;

//--------------------------------------------------------------------------------------------------
// StringRefStreamBuffer
//

StringRefStreamBuffer::StringRefStreamBuffer(StringRef Str)
{
    auto First = const_cast<char*>(Str.data());
    setg(First, First, First + Str.size());
}

StringRefStreamBuffer::~StringRefStreamBuffer()
{
}

// Sets the position indicator of the inputsequence relative to some other position.
StringRefStreamBuffer::pos_type StringRefStreamBuffer::seekoff(
        off_type Off,
        std::ios_base::seekdir Way,
        std::ios_base::openmode Which)
{
    if (gptr() && (Which & std::ios_base::in))
    {
        switch (Way)
        {
        case std::ios_base::beg:
            break;
        case std::ios_base::cur:
            Off += gptr() - eback();
            break;
        case std::ios_base::end:
            Off += egptr() - eback();
            break;
        default:
            assert(!"not handled");
            return pos_type(-1);
        }

        if (0 <= Off && Off <= egptr() - eback())
        {
            setg(eback(), eback() + Off, egptr());
            return pos_type(Off);
        }
    }

    return pos_type(-1);
}

// Sets the position indicator of the input sequence to an absolute position.
StringRefStreamBuffer::pos_type StringRefStreamBuffer::seekpos(
        pos_type Pos,
        std::ios_base::openmode Which)
{
    if (gptr() && (Which & std::ios_base::in))
    {
        off_type Off(Pos);

        if (0 <= Off && Off <= egptr() - eback())
        {
            setg(eback(), eback() + Off, egptr());
            return Pos;
        }
    }

    return pos_type(-1);
}

// Reads count characters from the input sequence and stores them into a character array.
// The characters are read as if by repeated calls to sbumpc(). That is, if less than count
// characters are immediately available, the function calls uflow() to provide more until
// traits::eof() is returned.
std::streamsize StringRefStreamBuffer::xsgetn(char_type* Dest, std::streamsize Count)
{
    if (gptr() == nullptr)
        return 0;

    Count = std::min(Count, in_avail());

    if (Count > 0)
    {
        // Copy the characters
        traits_type::copy(Dest, gptr(), static_cast<size_t>(Count));

        // Advance the get pointer
        gbump(static_cast<int>(Count));
    }

    return Count;
}

//--------------------------------------------------------------------------------------------------
// StringRefStream
//

StringRefStream::StringRefStream(StringRef Str)
    : BaseType(nullptr)
    , Buffer(Str)
{
    init(&Buffer);
}

StringRefStream::~StringRefStream()
{
}
