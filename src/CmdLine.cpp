// This file is distributed under the MIT license.
// See the LICENSE file for details.


#include "Support/CmdLine.hh"

#include <algorithm>
#include <cctype>
#include <fstream>
#include <iostream>


namespace cl = support::cl;

using cl::CmdLine;
using cl::OptionBase;

using support::StringRef;


//--------------------------------------------------------------------------------------------------
// CmdLine
//--------------------------------------------------------------------------------------------------


bool CmdLine::add(OptionBase* opt)
{
    if (opt->formatting == Positional)
    {
        positionals.push_back(opt);
    }
    else
    {
        if (opt->name.empty())
        {
            for (auto const& s : opt->getValueNames())
                if (!options.insert({s, opt}).second)
                    return false;
        }
        else
        {
            if (!options.insert({opt->name, opt}).second)
                return false;
        }
    }

    return true;
}


bool CmdLine::parse(StringVector argv, bool ignoreUnknowns)
{
    // Expand response files
    if (!expandResponseFiles(argv))
        return false;

    bool success = true;
    bool ok = true;

    // "--" seen?
    bool dashdash = false;

    // The current positional argument - if any
    auto pos = positionals.begin();

    for (size_t i = 0, e = argv.size(); i != e; ++i, success = success && ok)
    {
        StringRef arg = argv[i];

        // Stop parsing if "--" has been found
        if (arg == "--" && !dashdash)
        {
            dashdash = true;
            continue;
        }

        // This argument is considered to be positional if it doesn't start with '-', if it is "-"
        // itself, or if we have seen "--" already.
        if (arg[0] != '-' || arg == "-" || dashdash)
        {
            handlePositional(ok, arg, i, pos);
            continue;
        }

        // Starts with a dash, must be an argument.
        auto name = arg.dropFront(1);

        if (name.size() > 0 && name[0] == '-')
            name = name.dropFront(1);

        assert( !name.empty() );

        // Try to process this argument as a standard option.
        if (handleOption(ok, name, i, argv))
            continue;

        // If it's not a standard option and there is no equals sign,
        // check for a prefix option.
        if (handlePrefix(ok, name, i))
            continue;

        // If it's not standard or prefix option, check for an option group
        if (handleGroup(ok, name, i))
            continue;

        // Unknown option specified...
        ok = ignoreUnknowns;

        if (!ok)
            error("unknown option '" + arg + "'");
    }

    // Check if all required options have been successfully parsed
    success = check() && success;

    return success;
}


void CmdLine::help() const
{
    if (!overview.empty())
        std::cout << "Overview:\n  " << overview << "\n\n";

    std::cout << "Usage:\n  " << programName << " [options]";

    if (!positionals.empty())
    {
        for (auto& I : positionals)
            std::cout << " " << I->usage();
    }

    std::cout << "\n\nOptions:\n";

    for (auto I : getOptions())
        I->help();
}


CmdLine::OptionVector CmdLine::getOptions() const
{
    // Get the list of all options
    auto opts = OptionVector(mapSecondIterator(options.begin()),
                             mapSecondIterator(options.end())
                             );

    // Sort by name
    std::stable_sort(opts.begin(), opts.end(), [](OptionBase* LHS, OptionBase* RHS) {
        return LHS->name < RHS->name;
    });

    // Remove duplicates
    auto E = std::unique(opts.begin(), opts.end());

    // Remove hidden options
    E = std::remove_if(opts.begin(), E, [](OptionBase* opt) {
        return opt->miscFlags & Hidden;
    });

    // Actually erase unused options
    opts.erase(E, opts.end());

    return opts;
}


OptionBase* CmdLine::findOption(StringRef name) const
{
    auto I = options.find(name);
    return I == options.end() ? 0 : I->second;
}


// Parses a command line string and returns an array of command line arguments.
template<class InputIterator, class OutputIterator>
static void StringToArgv(InputIterator first, InputIterator last, OutputIterator out)
{
    using StringType = std::string;

    StringType arg;
    StringType::value_type quoteChar = 0;

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
            if (!arg.empty())
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
    if (!arg.empty())
        *out++ = std::move(arg);
}


// Recursively expand response files.
// Returns true on success, false otherwise.
bool CmdLine::expandResponseFile(StringVector& argv, size_t i)
{
    std::ifstream file(argv[i].substr(1));

    if (!file)
        return error("no such file or directory: \"" + argv[i] + "\"");

    // Erase the i-th argument (@file)
    argv.erase(argv.begin() + i);

    // Parse the file while inserting new command line arguments before the erased argument
    auto I = std::istreambuf_iterator<char>(file.rdbuf());
    auto E = std::istreambuf_iterator<char>();

    StringToArgv(I, E, std::inserter(argv, argv.begin() + i));

    return true;
}


bool CmdLine::expandResponseFiles(StringVector& argv)
{
    // Response file counter to prevent infinite recursion...
    size_t responseFilesLeft = 100;

    for (size_t i = 0; i < argv.size(); )
    {
        if (argv[i][0] != '@')
        {
            i++;
            continue;
        }

        if (!expandResponseFile(argv, i))
            return false;

        if (--responseFilesLeft == 0)
            return error("too many response files encountered");
    }

    return true;
}


bool CmdLine::handlePositional(bool& success, StringRef name, size_t i, OptionVector::iterator& pos)
{
    if (pos == positionals.end())
    {
        success = error("unhandled positional argument: '" + name + "'");
        return true;
    }

    auto opt = *pos;

    // If the current positional argument does not allow any further
    // occurrence try the next.
    if (!opt->isOccurrenceAllowed())
        return handlePositional(success, name, i, ++pos);

    //
    // TODO:
    //
    // If the current positional is Optional and cannot handle the given value,
    // skip to next positional argument instead of reporting an error?!
    //
    success = addOccurrence(opt, opt->name, name, i);
    return true;
}


// If 'name' is the name of an option, process the option immediately.
// Otherwise looks for an equal sign and try again.
bool CmdLine::handleOption(bool& success, StringRef name, size_t& i, StringVector& argv)
{
    if (auto opt = findOption(name)) // Standard option?
    {
        StringRef value;

        // If the option name is empty, this option really is a map of option names
        if (opt->name.empty())
            value = name;

        // If the option requires an argument, "steal" the next argument from the
        // command line, so that "-o file" is possible instead of "-o=file"
        /*else*/ if (opt->numArgs == ArgRequired)
        {
            if (i + 1 >= argv.size() || argv[i + 1][0] == '-')
            {
                success = error("option '" + name + "' expects an argument");
                return true;
            }

            value = argv[++i];
        }

        success = addOccurrence(opt, name, value, i);
        return true;
    }

    // Get the index of the first equals sign
    auto I = name.find('=');

    if (I == StringRef::npos)
        return false;

    // Get the option name
    auto opt_name = name.front(I);

    if (auto opt = findOption(opt_name))
    {
        if (opt->numArgs == ArgDisallowed)
        {
            // An argument was specified, but this is not allowed
            success = error("option '" + opt_name + "' does not allow an argument");
        }
        else
        {
            // Include the equals sign in the value for prefix options.
            // Discard otherwise.
            success = addOccurrence(opt, opt_name, name.dropFront(opt->formatting == Prefix ? I : I + 1), i);
        }

        return true;
    }

    return false;
}


bool CmdLine::handlePrefix(bool& success, StringRef name, size_t i)
{
    assert( name.size() != 0 );

    for (auto n = name.size() - 1; n != 0; --n)
    {
        auto opt = findOption(name.front(n));

        if (opt && opt->formatting == Prefix)
        {
            success = addOccurrence(opt, opt->name, name.dropFront(n), i);
            return true;
        }
    }

    return false;
}


bool CmdLine::handleGroup(bool& success, StringRef name, size_t i)
{
    OptionVector group;

    group.reserve(name.size());

    // First check if the name consists of single letter options
    for (size_t n = 0; n != name.size(); ++n)
    {
        auto opt = findOption(name.substr(n, 1));

        if (opt == 0 || opt->formatting != Grouping)
            return false;
        else
            group.push_back(opt);
    }

    success = true;

    // Then handle all the options
    for (auto opt : group)
        success = addOccurrence(opt, opt->name, StringRef(), i) && success;

    return true;
}


bool CmdLine::addOccurrence(OptionBase* opt, StringRef name, StringRef value, size_t i)
{
    if (!opt->isOccurrenceAllowed())
    {
        if (opt->numOccurrences == Optional)
            return error("option '" + name + "' must occur at most once");
        else
            return error("option '" + name + "' must occur exactly once");
    }

    auto parse = [&](StringRef v) -> bool
    {
        if (!opt->parse(v, i))
            return error("invalid argument '" + v + "' for option '" + name + "'");

        return true;
    };

    if (opt->miscFlags & CommaSeparated)
    {
        if (!value.tokenize(",", parse))
            return false;
    }
    else
    {
        if (!parse(value))
            return false;
    }

    opt->count++;

    return true;
}


bool CmdLine::check()
{
    bool success = true;

    for (auto& I : options)
        success = check(I.second) && success;

    for (auto& I : positionals)
        success = check(I) && success;

    return success;
}


bool CmdLine::check(OptionBase* opt)
{
    if (opt->isOccurrenceRequired())
        return error("option '" + opt->name + "' missing");

    return true;
}


bool CmdLine::error(std::string str)
{
    errors.push_back(std::move(str));
    return false;
}


//--------------------------------------------------------------------------------------------------
// OptionBase
//--------------------------------------------------------------------------------------------------


// TODO: cache this?!?!
std::string OptionBase::usage() const
{
    std::string str;

    if (formatting == Positional)
    {
        str += "<" + name + ">" + (isUnbounded() ? "..." : "");

        if (isOptional())
            str = "[" + str + "]";
    }
    else
    {
        if (name.empty() || numArgs != ArgDisallowed)
        {
            str = "<" + argName + ">" + ((miscFlags & CommaSeparated) ? ",..." : "");

            if (!name.empty())
            {
                if (numArgs == ArgOptional)
                    str = "[=" + str + "]";
                else /*ArgRequired*/ if (formatting != Prefix)
                    str = " " + str;
            }
        }

        str = name + str;
    }

    return str;
}


// TODO: cache this?!?!
void OptionBase::help() const
{
    static const StringRef kIndent = "                        ";

    auto str = "  -" + usage();

    if (str.size() >= kIndent.size())
        std::cout << str << "\n" << kIndent;
    else
        std::cout << str << kIndent.dropFront(str.size());

    std::cout << desc << "\n";
}


bool OptionBase::isOccurrenceAllowed() const
{
    if (numOccurrences == Optional || numOccurrences == Required)
        return count == 0;

    return true;
}


bool OptionBase::isOccurrenceRequired() const
{
    if (numOccurrences == Required || numOccurrences == OneOrMore)
        return count == 0;

    return false;
}


bool OptionBase::isUnbounded() const
{
    return numOccurrences == ZeroOrMore || numOccurrences == OneOrMore;
}


bool OptionBase::isOptional() const
{
    return numOccurrences == Optional || numOccurrences == ZeroOrMore;
}


template<class Iterator>
static std::string Concat(Iterator first, Iterator last)
{
    std::ostringstream stream;

    if (first != last)
    {
        stream << *first;

        while (++first != last)
            stream << "|" << *first;
    }

    return stream.str();
}


void OptionBase::done()
{
    if (formatting == Positional || formatting == Prefix)
        numArgs = ArgRequired;

    if (formatting == Grouping)
        numArgs = ArgDisallowed;

    if (argName.empty())
    {
        auto names = getValueNames();

        if (names.empty())
            argName = "arg";
        else
            argName = Concat(names.begin(), names.end());
    }
}
