// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/StringSplit.h"

#include <algorithm>
#include <iostream>

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

} // namespace

//--------------------------------------------------------------------------------------------------
// CmdLine
//

CmdLine::CmdLine(std::string program, std::string overview)
    : program(std::move(program))
    , overview(std::move(overview))
    , maxPrefixLength(0)
{
}

bool CmdLine::add(OptionBase* opt)
{
    if (opt->formatting == Positional)
    {
        positionals.push_back(opt);
        return true;
    }

    auto insert = [&](StringRef s, OptionBase* opt) -> bool
    {
        // Save the length of the longest prefix option
        if (opt->isPrefix()) {
            if (maxPrefixLength < s.size())
                maxPrefixLength = s.size();
        }

        // Insert the option into the map
        return options.insert({ s, opt }).second;
    };

    if (opt->name.empty())
    {
        std::vector<StringRef> values;

        opt->allowedValues(values);

        if (values.empty())
            return false;

        for (auto&& s : values)
        {
            if (!insert(s, opt))
                return false;
        }

        return true;
    }

    // Option name is not empty.
    // Might be a list of different names.
    for (auto const& s : strings::split(opt->name, "|"))
    {
        if (!insert(s, opt))
            return false;
    }

    return true;
}

bool CmdLine::parse(StringVector const& argv, bool ignoreUnknowns)
{
    bool success = true;
    bool dashdash = false; // "--" seen?

    // The current positional argument - if any
    auto pos = positionals.begin();

    // Handle all arguments...
    for (size_t N = 0; N < argv.size(); ++N)
    {
        if (!handleArg(argv, N, pos, dashdash, ignoreUnknowns))
        {
            success = false;
        }
    }

    // Check if all required options have been successfully parsed
    return check() && success;
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

CmdLine::ConstOptionVector CmdLine::getOptions(bool SkipHidden) const
{
    ConstOptionVector opts;

    // Get the list of all (visible) options
    for (auto const& I : options)
    {
        if (!SkipHidden || (I.second->miscFlags & Hidden) == 0)
            opts.emplace_back(I.second);
    }

    // Sort by name
    std::stable_sort(opts.begin(), opts.end(),
        [](OptionBase const* LHS, OptionBase const* RHS)
        {
            return LHS->name < RHS->name;
        }
    );

    // Remove duplicates
    opts.erase(std::unique(opts.begin(), opts.end()), opts.end());

    return opts;
}

CmdLine::ConstOptionVector CmdLine::getPositionalOptions() const
{
    return ConstOptionVector(positionals.begin(), positionals.end());
}

OptionBase* CmdLine::findOption(StringRef name) const
{
    auto I = options.find(name);
    return I == options.end() ? 0 : I->second;
}

//
// Process a single command line argument.
// Returns true on success, false otherwise.
//
bool CmdLine::handleArg(StringVector const& argv,
                        size_t& i,
                        OptionVector::iterator& pos,
                        bool& dashdash,
                        bool ignoreUnknowns
                        )
{
    StringRef arg = argv[i];

    // Stop parsing if "--" has been found
    if (arg == "--" && !dashdash)
    {
        dashdash = true;
        return true;
    }

    // This argument is considered to be positional if it doesn't start with '-', if it is "-"
    // itself, or if we have seen "--" already.
    if (arg[0] != '-' || arg == "-" || dashdash)
    {
        if (handlePositional(arg, i, pos))
        {
            // If the current positional argument has the ConsumeAfter flag set, parse
            // all following command-line arguments as positional options.
            if ((*pos)->miscFlags & ConsumeAfter)
                dashdash = true;

            return true;
        }

        // Unhandled positional argument...
        unknowns.emplace_back(arg.str());

        return ignoreUnknowns || error("unhandled positional argument: '" + arg + "'");
    }

    // Starts with a dash, must be an argument.
    auto name = arg.drop_front(1);

    bool short_option = name[0] != '-';

    if (!short_option)
        name = name.drop_front(1);

    bool success = true;

    // Try to process this argument as a standard option.
    if (handleOption(success, name, i, argv))
        return success;

    // If it's not a standard option and there is no equals sign,
    // check for a prefix option.
    // FIXME: Allow prefix only for short options?
    if (handlePrefix(success, name, i))
        return success;

    // If it's not standard or prefix option, check for an option group
    if (short_option && handleGroup(success, name, i))
        return success;

    // Unknown option specified...
    unknowns.emplace_back(arg);

    return ignoreUnknowns || error("unknown option '" + arg + "'");
}

//
// Process a positional argument.
// Returns true on success, false otherwise.
//
bool CmdLine::handlePositional(StringRef arg, size_t i, OptionVector::iterator& pos)
{
    if (pos == positionals.end())
        return false;

    auto opt = *pos;

    // If the current positional argument does not allow any further
    // occurrence try the next.
    if (!opt->isOccurrenceAllowed())
        return handlePositional(arg, i, ++pos);

    // The value of a positional option is the argument itself.
    return addOccurrence(opt, arg, arg, i);
}

//
// If 'arg' is the name of an option, process the option immediately.
// Otherwise looks for an equal sign and try again.
//
// Returns true if the option was handled, returns in 'success' whether the option was
// successfully handled.
//
bool CmdLine::handleOption(bool& success, StringRef arg, size_t& i, StringVector const& argv)
{
    if (auto opt = findOption(arg)) // Standard option?
    {
        StringRef value;

        // If the option requires an argument, "steal" the next argument from the
        // command line, so that "-o file" is possible instead of "-o=file"
        if (opt->numArgs == ArgRequired)
        {
            if (opt->formatting == Prefix || i + 1 >= argv.size())
            {
                success = error("option '" + opt->displayName() + "' expects an argument");
                return true;
            }

            value = argv[++i];
        }

        success = addOccurrence(opt, arg, value, i);
        return true;
    }

    // Get the index of the first equals sign
    auto I = arg.find('=');

    if (I == StringRef::npos)
        return false;

    // Get the option name
    auto spec = arg.front(I);

    if (auto opt = findOption(spec))
    {
        if (opt->numArgs == ArgDisallowed)
        {
            // An argument was specified, but this is not allowed
            success = error("option '" + opt->displayName() + "' does not allow an argument");
        }
        else
        {
            // Include the equals sign in the value if this option is a prefix option.
            // Discard otherwise.
            if (!opt->isPrefix())
                I++;

            success = addOccurrence(opt, spec, arg.drop_front(I), i);
        }

        return true;
    }

    return false;
}

//
// Handles prefix options which have an argument specified without an equals sign.
//
// Returns true if the option was handled, returns in 'success' whether the option was
// successfully handled.
//
bool CmdLine::handlePrefix(bool& success, StringRef arg, size_t i)
{
    assert(!arg.empty());

    for (auto n = std::min(maxPrefixLength, arg.size()); n != 0; --n)
    {
        auto spec = arg.front(n);
        auto opt = findOption(spec);

        if (opt && opt->isPrefix())
        {
            success = addOccurrence(opt, spec, arg.drop_front(n), i);
            return true;
        }
    }

    return false;
}

//
// Process an option group.
//
// Returns true if the option was handled, returns in 'success' whether the option was
// successfully handled.
//
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

bool CmdLine::addOccurrence(OptionBase* opt, StringRef spec, StringRef value, size_t i)
{
    StringRef name = opt->displayName();

    if (!opt->isOccurrenceAllowed())
    {
        if (opt->numOccurrences == Optional)
            return error("option '" + name + "' must occur at most once");
        else
            return error("option '" + name + "' must occur exactly once");
    }

    auto parse = [&](StringRef spec, StringRef value) -> bool
    {
        if (!opt->parse(spec, value, i))
            return error("invalid argument '" + value + "' for option '" + name + "'");

        opt->count++;

        return true;
    };

    if (opt->miscFlags & CommaSeparated)
    {
        for (auto v : strings::split(value, ","))
            if (!parse(spec, v))
                return false;
    }
    else
    {
        if (!parse(spec, value))
            return false;
    }

    return true;
}

bool CmdLine::check()
{
    bool success = true;

    for (auto& I : getOptions(false/*SkipHidden*/))
        success = check(I) && success;

    for (auto& I : positionals)
        success = check(I) && success;

    for (auto& I : groups)
        success = check(I.second) && success;

    return success;
}

bool CmdLine::check(OptionBase const* opt)
{
    if (!opt->isOccurrenceRequired())
        return true;

    return error("option '" + opt->displayName() + "' missing");
}

bool CmdLine::check(OptionGroup const* g)
{
    if (g->check())
        return true;

    return error(g->desc());
}

// Adds an error message. Returns false.
bool CmdLine::error(std::string str)
{
    errors.push_back(std::move(str));
    return false;
}

//--------------------------------------------------------------------------------------------------
// OptionGroup
//

OptionGroup::OptionGroup(CmdLine& cmd, std::string name, Type type)
    : name(std::move(name))
    , type(type)
{
    if (!cmd.groups.insert({this->name, this}).second)
    {
    }
}

bool OptionGroup::check() const
{
    if (type == OptionGroup::Default)
        return true;

    // Count the number of options in this group which have been specified on the command-line
    size_t N = std::count_if(options.begin(), options.end(),
        [](OptionBase const* x)
        {
            return x->getCount() > 0;
        }
    );

    switch (type)
    {
    case OptionGroup::Default: // prevent warning
        return true;
    case OptionGroup::Zero:
        return N == 0;
    case OptionGroup::ZeroOrOne:
        return N == 0 || N == 1;
    case OptionGroup::One:
        return N == 1;
    case OptionGroup::OneOrMore:
        return N >= 1;
    case OptionGroup::All:
        return N == options.size();
    case OptionGroup::ZeroOrAll:
        return N == 0 || N == options.size();
    }

    return true; // prevent warning
}

std::string OptionGroup::desc() const
{
    switch (type)
    {
    case OptionGroup::Default:
        return "any number of options in group '" + name + "' may be specified";
    case OptionGroup::Zero:
        return "no options in group '" + name + "' may be specified";
    case OptionGroup::ZeroOrOne:
        return "at most one option in group '" + name + "' may be specified";
    case OptionGroup::One:
        return "exactly one option in group '" + name + "' must be specified";
    case OptionGroup::OneOrMore:
        return "at least on option in group '" + name + "' must be specified";
    case OptionGroup::All:
        return "all options in group '" + name + "' must be specified";
    case OptionGroup::ZeroOrAll:
        return "none or all options in group '" + name + "' must be specified";
    }

    assert(!"internal error");
    return "xxx";
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

OptionBase::~OptionBase()
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
        str = "[=" + str + "]";

    if (numArgs == ArgRequired && !isPrefix())
        str = " " + str;

    return name + str;
}

void OptionBase::help() const
{
    std::vector<StringRef> values;
    std::vector<StringRef> descr;

    allowedValues(values);

    if (values.empty())
    {
        auto prefix = formatting == Positional ? "  " : "  -";
        std::cout << Aligned(prefix + usage()) << Wrapped(desc, kIndentDescription) << "\n";
        return;
    }

    // This option only allows a limited set of values.
    // Show all valid values.

    // Get the description for each value
    descriptions(descr);

    assert(descr.size() == values.size() && "not supported");

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
                  << Wrapped("- " + descr[I], kIndentDescription + 2) << "\n";
    }
}

StringRef OptionBase::displayName() const
{
    if (name.empty())
        return argName;

    return name;
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

bool OptionBase::isRequired() const
{
    return numOccurrences == Required || numOccurrences == OneOrMore;
}

bool OptionBase::isPrefix() const
{
    return formatting == Prefix || formatting == MayPrefix;
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
        argName = "arg";
}
