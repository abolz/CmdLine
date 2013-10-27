// This file is distributed under the MIT license.
// See the LICENSE file for details.


#pragma once


#include <cctype>
#include <iterator>
#include <string>
#include <utility>


namespace support
{
namespace cl
{


// Parses a command line string and returns a list of command line arguments.
// Using Unix-style escaping.
template<class InputIterator, class OutputIterator>
void tokenizeCommandLineUnix(InputIterator first, InputIterator last, OutputIterator out)
{
    using CharType = typename std::iterator_traits<InputIterator>::value_type;
    using StringType = std::basic_string<CharType>;

    StringType arg;

    CharType quoteChar = 0;

    for ( ; first != last; ++first)
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


// Parses a command line string and returns a list of command line arguments.
// Using Windows-style escaping.
template<class InputIterator, class OutputIterator>
void tokenizeCommandLineWin(InputIterator first, InputIterator last, OutputIterator out)
{
    using CharType = typename std::iterator_traits<InputIterator>::value_type;
    using StringType = std::basic_string<CharType>;

    StringType arg;

    unsigned numBackslashes = 0;

    bool quoting = false;
    bool recentlyClosed = false;

    for ( ; first != last; ++first)
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


} // namespace cl
} // namespace support
