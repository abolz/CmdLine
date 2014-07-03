// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <vector>

namespace support
{
namespace cl
{

//--------------------------------------------------------------------------------------------------
// Recursively expand response files.
//
template <class Tokenizer>
void expandResponseFiles(std::vector<std::string>& args, Tokenizer tokenize)
{
    // Expand the response file at position i
    auto expand1 = [&](size_t i)
    {
        std::ifstream file;

        file.exceptions(std::ios::failbit);
        file.open(args[i].substr(1));

        // Erase the i-th argument (@file)
        args.erase(args.begin() + i);

        // Parse the file while inserting new command line arguments before the erased argument
        auto I = std::istreambuf_iterator<char>(file.rdbuf());
        auto E = std::istreambuf_iterator<char>();

        tokenize(I, E, std::inserter(args, args.begin() + i));
    };

    // Response file counter to prevent infinite recursion...
    size_t responseFilesLeft = 100;

    // Recursively expand respond files...
    for (size_t i = 0; i < args.size(); /**/)
    {
        if (args[i][0] != '@')
        {
            i++;
            continue;
        }

        expand1(i);

        if (--responseFilesLeft == 0)
            throw std::runtime_error("too many response files encountered");
    }
}

//--------------------------------------------------------------------------------------------------
// Expand wild cards.
// This is only supported for Windows.
//
void expandWildcards(std::vector<std::string>& args);

} // namespace cl
} // namespace support
