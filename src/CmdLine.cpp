// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/CmdLineToArgv.h"
#include "Support/StringSplit.h"

#include <algorithm>
#include <fstream>
#include <iostream>
#include <sstream>

using namespace support;
using namespace support::cl;

//--------------------------------------------------------------------------------------------------
// Misc.
//

namespace
{

const size_t kMaxWidth = 80;
const size_t kIndentDescription = 20;

// Returns a string consisting of N spaces.
StringRef Spaces(size_t N)
{
    static std::vector<char> str(kMaxWidth, ' ');

    if (str.size() < N)
        str.resize(N, ' ');

    return { &str[0], N };
}

struct Wrapped
{
    // The text to wrap
    StringRef Text;
    // Indentation for all but the first line
    // FIXME: Make this a string?
    size_t Indent;
    // The maximum width of a single line of text
    size_t MaxWidth;

    explicit Wrapped(StringRef Text, size_t Indent = 0, size_t MaxWidth = kMaxWidth)
        : Text(Text)
        , Indent(Indent)
        , MaxWidth(MaxWidth)
    {
    }
};

std::ostream& operator<<(std::ostream& stream, Wrapped const& x)
{
    bool first = true;

    for (auto par : strings::split(x.Text, "\n"))
    {
        for (auto line : strings::split(par, strings::wrap(x.MaxWidth - x.Indent)))
        {
            if (first)
                first = false;
            else
                stream << "\n" << Spaces(x.Indent);

            stream << line;
        }
    }

    return stream;
}

struct Aligned
{
    // The text to print
    StringRef text;
    // Align to
    size_t indent;

    explicit Aligned(StringRef text, size_t indent = kIndentDescription)
        : text(text)
        , indent(indent)
    {
    }
};

std::ostream& operator<<(std::ostream& stream, Aligned const& x)
{
    if (x.text.size() < x.indent)
    {
        // Fits.
        // Print the text, then skip to indentation width
        return stream << x.text << Spaces(x.indent - x.text.size());
    }

    // Does not fit.
    // Print the text, move to next line, then move to indentation width
    return stream << x.text << "\n" << Spaces(x.indent);
}

template <class Range>
std::string Join(Range const& range, StringRef sep)
{
    std::ostringstream result;

    bool first = true;

    for (auto const& s : range)
    {
        if (first)
            first = false;
        else
            result << sep;

        result << s;
    }

    return result.str();
}

} // namespace

//--------------------------------------------------------------------------------------------------
// CmdLine
//

CmdLine::CmdLine(std::string program, std::string overview)
    : program(std::move(program))
    , overview(std::move(overview))
{
}

bool CmdLine::add(OptionBase* opt)
{
    if (opt->formatting == Positional)
    {
        positionals.push_back(opt);
        return true;
    }

    if (opt->name.empty())
    {
        std::vector<StringRef> values;

        opt->getValues(values);

        if (values.empty())
            return false;

        for (auto&& s : values)
        {
            if (!options.insert({ s, opt }).second)
                return false;
        }

        return true;
    }

    return options.insert({ opt->name, opt }).second;
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

            // If the current positional argument has the ConsumeAfter flag set, parse
            // all following command-line arguments as positional options.
            if (ok && ((*pos)->miscFlags & ConsumeAfter) != 0)
                dashdash = true;

            continue;
        }

        // Starts with a dash, must be an argument.
        auto name = arg.drop_front(1);

        if (name.size() > 0 && name[0] == '-')
            name = name.drop_front(1);

        assert(!name.empty());

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

void CmdLine::help(bool showPositionals) const
{
    if (!overview.empty())
        std::cout << "Overview:\n  " << Wrapped(overview, 2) << "\n\n";

    std::cout << "Usage:\n  " << program << " [options]";

    for (auto I : positionals)
    {
        if (I->isOptional())
            std::cout << " [" << I->usage() << "]";
        else
            std::cout << " " << I->usage();
    }

    std::cout << "\n\nOptions:\n";

    for (auto I : getOptions())
        I->help();

    if (showPositionals && !positionals.empty())
    {
        std::cout << "\nPositional options:\n";

        for (auto I : positionals)
            I->help();
    }
}

CmdLine::OptionVector CmdLine::getOptions(bool SkipHidden) const
{
    OptionVector opts;

    // Get the list of all (visible) options
    for (auto const& I : options)
    {
        if (!SkipHidden || (I.second->miscFlags & Hidden) == 0)
            opts.emplace_back(I.second);
    }

    // Sort by name
    std::stable_sort(opts.begin(), opts.end(), [](OptionBase* LHS, OptionBase* RHS) {
        return LHS->name < RHS->name;
    });

    // Remove duplicates
    opts.erase(std::unique(opts.begin(), opts.end()), opts.end());

    return opts;
}

OptionBase* CmdLine::findOption(StringRef name) const
{
    auto I = options.find(name);
    return I == options.end() ? 0 : I->second;
}

bool CmdLine::isPossibleOption(StringRef name) const
{
    if (name.size() < 2)
        return false;

    // If name does not start with a dash, it's not an option
    if (name[0] != '-')
        return false;

    // If the name does start with two (or more...) dashes it might be an option
    if (name[1] == '-')
        return true;

    // If name starts with a single dash, check if there is an option
    // with the given name.
    return findOption(name.drop_front(1)) != nullptr;
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

    tokenizeCommandLineUnix(I, E, std::inserter(argv, argv.begin() + i));

    return true;
}

bool CmdLine::expandResponseFiles(StringVector& argv)
{
    // Response file counter to prevent infinite recursion...
    size_t responseFilesLeft = 100;

    for (size_t i = 0; i < argv.size();)
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
    // FIXME:
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
        // If the option name is empty, this option really is a map of option names
        if (opt->name.empty())
        {
            success = addOccurrence(opt, name, name, i);
            return true;
        }

        StringRef value;

        // If the option requires an argument, "steal" the next argument from the
        // command line, so that "-o file" is possible instead of "-o=file"
        if (opt->numArgs == ArgRequired)
        {
            if (opt->formatting == Prefix || (i + 1 >= argv.size() || isPossibleOption(argv[i + 1])))
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
            // Include the equals sign in the value if an argument is required.
            // Discard otherwise.
            if (opt->formatting != Prefix || opt->numArgs != ArgRequired)
                I++;

            success = addOccurrence(opt, opt_name, name.drop_front(I), i);
        }

        return true;
    }

    return false;
}

// Handles prefix options which have an argument specified without an equals sign.
bool CmdLine::handlePrefix(bool& success, StringRef name, size_t i)
{
    assert(name.size() != 0);

    for (auto n = name.size() - 1; n != 0; --n)
    {
        auto opt = findOption(name.front(n));

        if (opt && opt->formatting == Prefix)
        {
            success = addOccurrence(opt, opt->name, name.drop_front(n), i);
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
        if (opt->name.empty())
            return error("option '" + name + "' not allowed here");

        if (opt->numOccurrences == Optional)
            return error("option '" + name + "' must occur at most once");

        return error("option '" + name + "' must occur exactly once");
    }

    auto parse = [&](StringRef v)->bool
    {
        if (!opt->parse(v, i))
            return error("invalid argument '" + v + "' for option '" + name + "'");

        return true;
    };

    if (opt->miscFlags & CommaSeparated)
    {
        for (auto s : strings::split(value, ","))
            if (!parse(s))
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

    for (auto& I : getOptions(false/*SkipHidden*/))
        success = check(I) && success;

    for (auto& I : positionals)
        success = check(I) && success;

    return success;
}

bool CmdLine::check(OptionBase* opt)
{
    if (!opt->isOccurrenceRequired())
        return true;

    std::string name = opt->name.empty() ? opt->argName : opt->name;

    return error("option '" + name + "' missing");
}

// Adds an error message. Returns false.
bool CmdLine::error(std::string str)
{
    errors.push_back(std::move(str));
    return false;
}

//--------------------------------------------------------------------------------------------------
// OptionBase
//

OptionBase::OptionBase()
    : name()
    , argName()
    , desc("**** Documentation missing ****")
    , numOccurrences(Optional)
    , numArgs(ArgOptional)
    , formatting(DefaultFormatting)
    , miscFlags(None)
    , count(0)
{
}

std::string OptionBase::usage() const
{
    if (formatting == Positional)
        return "<" + name + ">" + (isUnbounded() ? "..." : "");

    if (numArgs == ArgDisallowed)
        return name;

    std::string str = "<" + argName + ">";

    if (numArgs == ArgOptional)
        str = "[" + str + "]";

    if (numArgs == ArgRequired && formatting != Prefix)
        str = " " + str;

    return name + str;
}

void OptionBase::help() const
{
    std::vector<StringRef> values;
    std::vector<StringRef> descriptions;

    getValues(values);

    if (values.empty())
    {
        auto prefix = formatting == Positional ? "  " : "  -";
        std::cout << Aligned(prefix + usage()) << Wrapped(desc, kIndentDescription) << "\n";
        return;
    }

    // This option only allows a limited set of values.
    // Show all valid values.

    // Get the description for each value
    getDescriptions(descriptions);

    assert(descriptions.size() == values.size() && "not supported");

    if (formatting == Positional)
    {
        // Print the name of the positional option and the description
        std::cout << Aligned("  <" + name + ">")
                  << Wrapped(desc + ":", kIndentDescription) << "\n";
    }
    else if (name.empty())
    {
        // Print the description
        std::cout << Wrapped("  " + desc + ":", 2) << "\n";
    }
    else
    {
        // Named alternative.
        // Print the name and the description.
        std::cout << Aligned("  -" + name) << Wrapped(desc + ":", kIndentDescription) << "\n";
    }

    std::string prefix = "    ";

    if (formatting != Positional)
        prefix += name.empty() ? "-" : "=";

    // Print all the possible options
    for (size_t I = 0; I < values.size(); ++I)
    {
        std::cout << Aligned(prefix + values[I])
                  << Wrapped("- " + descriptions[I], kIndentDescription + 2) << "\n";
    }
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

void OptionBase::done()
{
    assert((formatting != Positional || !name.empty())
        && "positional options need a name");

    if (formatting == Positional /*|| formatting == Prefix*/)
        numArgs = ArgRequired;

    if (formatting == Grouping)
        numArgs = ArgDisallowed;

    if (argName.empty())
    {
        std::vector<StringRef> values;

        getValues(values);

        argName = Join(values, "|");

        if (argName.empty()) // values was empty...
            argName = "arg";
    }
}
