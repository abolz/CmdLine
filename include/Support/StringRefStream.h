// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/StringRef.h"

#include <istream>
#include <ostream>
#include <streambuf>

namespace support
{

//--------------------------------------------------------------------------------------------------
// StringRefStreamBuffer
//

class StringRefStreamBuffer : public std::streambuf
{
    using BaseType = std::streambuf;

public:
    // Construct from a StringRef
    explicit StringRefStreamBuffer(StringRef Str);

    // Destructor
    virtual ~StringRefStreamBuffer();

    // Returns the current input buffer
    StringRef strref() const { return { gptr(), egptr() }; }

protected:
    // Sets the position indicator of the inputsequence relative to some other position.
    virtual pos_type seekoff(off_type Off,
                             std::ios_base::seekdir Way,
                             std::ios_base::openmode Which = std::ios_base::in) override;

    // Sets the position indicator of the input sequence to an absolute position.
    virtual pos_type seekpos(pos_type Pos,
                             std::ios_base::openmode Which = std::ios_base::in) override;

    // Reads characters from the input sequence and stores them into a character array
    virtual std::streamsize xsgetn(char_type* Dest, std::streamsize Count) override;
};

//--------------------------------------------------------------------------------------------------
// StringRefStream
//

class StringRefStream : public std::istream
{
    using BaseType = std::istream;
    using BufferType = StringRefStreamBuffer;

    BufferType Buffer;

public:
    // Construct from a StringRef
    StringRefStream(StringRef Str);

    // Destructor
    virtual ~StringRefStream();

    // Returns the current input buffer
    StringRef strref() const { return Buffer.strref(); }
};

} // namespace support
