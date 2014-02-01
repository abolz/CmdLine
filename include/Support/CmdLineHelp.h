// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include "Support/CmdLine.h"

namespace support
{
namespace cl
{

// Returns a short usage message for the given command line
std::string usage(CmdLine const& cmd);

// Returns a short usage message for the given option
std::string usage(OptionBase const* option);

// Prints the help message for the given command line
void help(std::ostream& stream, CmdLine const& cmd, StringRef overview = "");

// Prints the help message for the given option
void help(std::ostream& stream, OptionBase const* option);

// Prints the help message for the given option group
void help(std::ostream& stream, OptionGroup const* group);

} // namespace cl
} // namespace support
