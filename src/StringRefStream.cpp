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

//--------------------------------------------------------------------------------------------------
// Formatted output
//

std::ostream& support::operator <<(std::ostream& Stream, StringRef Str)
{
    //
    // 27.7.3.6.1 Common requirements [ostream.formatted.reqmts]
    //
    // 1    Each formatted output function begins execution by constructing an
    //      object of class sentry. If this object returns true when converted
    //      to a value of type bool, the function endeavors to generate the
    //      requested output. If the generation fails, then the formatted output
    //      function does setstate(ios_base::failbit), which might throw an
    //      exception. If an exception is thrown during output, then ios::badbit
    //      is turned on in *this's error state. If (exceptions()&badbit) != 0
    //      then the exception is rethrown. Whether or not an exception is
    //      thrown, the sentry object is destroyed before leaving the formatted
    //      output function. If no exception is thrown, the result of the
    //      formatted output function is *this.
    // 2    [...]
    // 3    If a formatted output function of a stream os determines padding, it
    //      does so as follows. Given a charT character sequence seq where charT
    //      is the character type of the stream, if the length of seq is less
    //      than os.width(), then enough copies of os.fill() are added to this
    //      sequence as necessary to pad to a width of os.width() characters.
    //      If (os.flags() & ios_base::adjustfield) == ios_base::left is true,
    //      the fill characters are placed after the character sequence;
    //      otherwise, they are placed before the character sequence.
    //
    // 27.7.3.6.4 Character inserter function templates [ostream.inserters.character]
    //
    // 4    Effects: Behaves like a formatted inserter (as described in 27.7.3.6.1)
    //      of out. [...] Creates a character sequence seq of n characters
    //      starting at s [...]. Determines padding for seq as described in
    //      27.7.3.6.1. Inserts seq into out. Calls width(0).
    // 5    Returns: out.
    //

    using traits = StringRef::traits_type;

    std::ostream::sentry ok(Stream);
    if (ok)
    {
        bool failed = false;
        try
        {
            std::streamsize size = Str.size();
            std::streamsize fill = Stream.width() <= 0 || Stream.width() <= size
                    ? 0
                    : Stream.width() - size;

            bool left = (Stream.flags() & std::ios_base::adjustfield) == std::ios_base::left;

            if (left)
            {
                failed = Stream.rdbuf()->sputn(Str.data(), size) != size;
            }

            for ( ; !failed && fill > 0; --fill)
            {
                failed = Stream.rdbuf()->sputc(Stream.fill()) == traits::eof();
            }

            if (!failed && !left)
            {
                failed = Stream.rdbuf()->sputn(Str.data(), size) != size;
            }
        }
        catch (...)
        {
            failed = true;
        }

        if (failed)
            Stream.setstate(std::ios_base::badbit | std::ios_base::failbit);
    }

    Stream.width(0);

    return Stream;
}
