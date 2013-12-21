// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/CmdLine.h"

namespace support
{
namespace cl
{

//--------------------------------------------------------------------------------------------------
// Misc.
//

struct Aligned
{
    // The text to print
    StringRef text;
    // Align to
    size_t indent;

    explicit Aligned(StringRef text, size_t indent)
        : text(text)
        , indent(indent)
    {
    }
};

std::ostream& operator<<(std::ostream& stream, Aligned const& x);

struct Wrapped
{
    // The text to wrap
    StringRef text;
    // Indentation for all but the first line
    size_t indent;
    // The maximum width of a single line of text
    size_t maxWidth;

    explicit Wrapped(StringRef text, size_t indent, size_t maxWidth)
        : text(text)
        , indent(indent)
        , maxWidth(maxWidth)
    {
    }
};

std::ostream& operator<<(std::ostream& stream, Wrapped const& x);

//--------------------------------------------------------------------------------------------------
// Help
//

// Returns a short usage message for the given command line
std::string usage(CmdLine const& cmd);

// Returns a short usage message for the given option
std::string usage(OptionBase const* option);

// Prints the help message for the given command line
void help(std::ostream& stream, CmdLine const& cmd);

// Prints the help message for the given option
void help(std::ostream& stream, OptionBase const* option);

// Prints the help message for the given option group
void help(std::ostream& stream, OptionGroup const* group);

} // namespace cl
} // namespace support
