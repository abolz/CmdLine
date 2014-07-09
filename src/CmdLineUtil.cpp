// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLineUtil.h"
#include "Support/StringRef.h"

#include <algorithm>

#ifdef _WIN32
#include <windows.h>
#endif

using support::StringRef;

#ifdef _WIN32

static std::vector<std::string> EnumerateFiles(StringRef pattern, size_t wildpos)
{
    assert(wildpos != StringRef::npos);

    // Find first slash or colon before wildcard
    size_t patternpos = pattern.find_last_of("\\/:", wildpos);

    std::string prefix = "";

    if (patternpos == StringRef::npos)
    {
        // Use current directory.
    }
    else
    {
        // Path specified.
        // Prefix each filename with the path.
        prefix.assign(pattern.data(), pattern.data() + patternpos + 1);
    }

    // Enumerate files

    std::vector<std::string> files;

    //
    // FIXME:
    // Use the Unicode version and convert strings to UTF-8?!?!
    //

    WIN32_FIND_DATAA info;
    HANDLE hFind = FindFirstFileExA(pattern.data(), FindExInfoStandard, &info, FindExSearchNameMatch, nullptr, 0);

    if (hFind != INVALID_HANDLE_VALUE)
    {
        do
        {
            StringRef arg = info.cFileName;

            if (arg[0] != '.' && !arg.starts_with(".."))
            {
                files.push_back(prefix + arg);
            }
        }
        while (FindNextFileA(hFind, &info));

        FindClose(hFind);
    }

    // Sort the list of files
    // Ignore case!

    std::sort(files.begin(), files.end(),
        [](StringRef LHS, StringRef RHS) { return LHS.compare_no_case(RHS) < 0; });

    return files;
}

#endif

void support::cl::expandWildcards(std::vector<std::string>& args)
{
#ifdef _WIN32

    size_t i = 0;
    size_t wildpos = StringRef::npos;

    // Find the first argument containing a '*' or a '?'
    for (; i != args.size(); ++i)
    {
        wildpos = args[i].find_first_of("*?");

        if (wildpos != std::string::npos)
            break;
    }

    // If there is no '*' or '?' there is nothing to expand...
    if (wildpos == StringRef::npos)
        return;

    // Enumerate the files matching the pattern
    auto files = EnumerateFiles(args[i], wildpos);

    // If there are no matches, leave the pattern in args_!
    if (files.empty())
        return;

    // Erase the i-th argument (pattern)
    args.erase(args.begin() + i);

    // Insert the new arguments
    auto I = std::make_move_iterator(files.begin());
    auto E = std::make_move_iterator(files.end());

    std::copy(I, E, std::inserter(args, args.begin() + i));

#else

    static_cast<void>(args);

#endif
}
