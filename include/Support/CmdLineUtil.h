// This file is distributed under the MIT license.
// See the LICENSE file for details.

#pragma once

#include <cctype>
#include <fstream>
#include <iterator>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

namespace support
{
namespace cl
{

//--------------------------------------------------------------------------------------------------
// Parses a command line string and returns a list of command line arguments.
// Using Unix-style escaping.
//
template <class InputIterator, class OutputIterator>
void tokenizeCommandLineUnix(InputIterator first, InputIterator last, OutputIterator out)
{
    // See:
    // http://www.gnu.org/software/bash/manual/bashref.html#Quoting
    // http://wiki.bash-hackers.org/syntax/quoting

    using CharType = typename std::iterator_traits<InputIterator>::value_type;
    using StringType = std::basic_string<CharType>;

    StringType arg;

    CharType quoteChar = 0;

    for (; first != last; ++first)
    {
        auto ch = *first;

        // Quoting a single character using the backslash?
        if (quoteChar == '\\')
        {
            arg.push_back(ch);
            quoteChar = 0;
            continue;
        }

        // Currently quoting using ' or "?
        if (quoteChar && ch != quoteChar)
        {
            arg.push_back(ch);
            continue;
        }

        // Toggle quoting?
        if (ch == '\'' || ch == '\"' || ch == '\\')
        {
            quoteChar = quoteChar ? 0 : ch;
            continue;
        }

        // Arguments are separated by whitespace
        if (std::isspace(ch))
        {
            if (arg.size() != 0)
            {
                *out++ = std::move(arg);
                arg = StringType();
            }
            continue;
        }

        // Nothing special...
        arg.push_back(ch);
    }

    // Append the last argument - if any
    if (arg.size() != 0)
    {
        *out++ = std::move(arg);
    }
}

struct TokenizeUnix
{
    template <class InputIterator, class OutputIterator>
    void operator ()(InputIterator first, InputIterator last, OutputIterator out) const
    {
        tokenizeCommandLineUnix(first, last, out);
    }
};

//--------------------------------------------------------------------------------------------------
// Parses a command line string and returns a list of command line arguments.
// Using Windows-style escaping.
//
template <class InputIterator, class OutputIterator>
void tokenizeCommandLineWindows(InputIterator first, InputIterator last, OutputIterator out)
{
    using CharType = typename std::iterator_traits<InputIterator>::value_type;
    using StringType = std::basic_string<CharType>;

    StringType arg;

    unsigned numBackslashes = 0;

    bool quoting = false;
    bool recentlyClosed = false;

    for (; first != last; ++first)
    {
        auto ch = *first;

        if (ch == '\"')
        {
            //
            // If a closing " is followed immediately by another ", the
            // 2nd " is accepted literally and added to the parameter.
            //
            // See:
            // http://www.daviddeley.com/autohotkey/parameters/parameters.htm#WINCRULESDOC
            //
            if (recentlyClosed)
            {
                arg.push_back(ch);

                recentlyClosed = false;
            }
            else
            {
                arg.append(numBackslashes / 2, '\\');

                if (numBackslashes % 2)
                {
                    //
                    // If an odd number of backslashes is followed by a double
                    // quotation mark, one backslash is placed in the argv array
                    // for every pair of backslashes, and the double quotation
                    // mark is "escaped" by the remaining backslash, causing a
                    // literal double quotation mark (") to be placed in argv.
                    //
                    arg.push_back(ch);
                }
                else
                {
                    //
                    // If an even number of backslashes is followed by a double
                    // quotation mark, one backslash is placed in the argv array
                    // for every pair of backslashes, and the double quotation
                    // mark is interpreted as a string delimiter.
                    //
                    quoting = !quoting;

                    // Remember if this is a closing "
                    recentlyClosed = !quoting;
                }

                numBackslashes = 0;
            }
        }
        else
        {
            recentlyClosed = false;

            if (ch == '\\')
            {
                numBackslashes++;
            }
            else
            {
                //
                // Backslashes are interpreted literally, unless they
                // immediately precede a double quotation mark.
                //
                arg.append(numBackslashes, '\\');

                if (!quoting && std::isspace(ch))
                {
                    //
                    // Arguments are delimited by white space, which is either a
                    // space or a tab.
                    //
                    // A string surrounded by double quotation marks ("string") is
                    // interpreted as a single argument, regardless of white space
                    // contained within. A quoted string can be embedded in an
                    // argument.
                    //
                    if (arg.size() != 0)
                    {
                        *out++ = std::move(arg);
                        arg = StringType();
                    }
                }
                else
                {
                    arg.push_back(ch);
                }

                numBackslashes = 0;
            }
        }
    }

    // Append any trailing backslashes
    arg.append(numBackslashes, '\\');

    // Append the last argument - if any
    if (quoting || arg.size() != 0)
    {
        *out++ = std::move(arg);
    }
}

struct TokenizeWindows
{
    template <class InputIterator, class OutputIterator>
    void operator ()(InputIterator first, InputIterator last, OutputIterator out) const
    {
        tokenizeCommandLineWindows(first, last, out);
    }
};

//--------------------------------------------------------------------------------------------------
// Quote a single command line argument. Using Windows-style escaping.
//
// See:
// http://blogs.msdn.com/b/twistylittlepassagesallalike/archive/2011/04/23/everyone-quotes-arguments-the-wrong-way.aspx
//
// This routine appends the given argument to a command line such
// that CommandLineToArgvW will return the argument string unchanged.
// Arguments in a command line should be separated by spaces; this
// function does not add these spaces.
//
template <class InputIterator, class OutputIterator>
void quoteSingleArgWindows(InputIterator first, InputIterator last, OutputIterator out)
{
    //
    // TODO:
    // Only add quotes if the input range contains whitespace?!
    //

    *out++ = '"';

    unsigned numBackslashes = 0;

    for (; first != last; ++first)
    {
        if (*first == '\\')
        {
            ++numBackslashes;
        }
        else if (*first == '"')
        {
            //
            // Escape all backslashes and the following
            // double quotation mark.
            //
            for (++numBackslashes; numBackslashes != 0; --numBackslashes)
            {
                *out++ = '\\';
            }
        }
        else
        {
            //
            // Backslashes aren't special here.
            //
            numBackslashes = 0;
        }

        *out++ = *first;
    }

    //
    // Escape all backslashes, but let the terminating
    // double quotation mark we add below be interpreted
    // as a metacharacter.
    //
    for (; numBackslashes != 0; --numBackslashes)
    {
        *out++ = '\\';
    }

    *out++ = '"';
}

//--------------------------------------------------------------------------------------------------
// Quote command line arguments. Using Windows-style escaping.
//
template <class InputIterator, class OutputIterator>
void quoteArgsWindows(InputIterator first, InputIterator last, OutputIterator out)
{
    using std::begin;
    using std::end;

    for (; first != last; ++first)
    {
        auto I = begin(*first);
        auto E = end(*first);

        if (I == E)
        {
            //
            // If a command line ends with an opening " CommandLineToArgvW will append
            // an empty argument to the list of command line arguments. If there is an
            // empty argument in the input range, add an " and return, since only the
            // last argument might be empty.
            //
            *out++ = '"';
            break;
        }

        //
        // Append the current argument
        //
        quoteSingleArgWindows(I, E, out);

        //
        // Separate arguments with spaces
        //
        *out++ = ' ';
    }
}

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
