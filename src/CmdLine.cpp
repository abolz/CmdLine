// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/CmdLineToArgv.h"
#include "Support/StringSplit.h"

#include <algorithm>
#include <fstream>
#include <iostream>

using namespace support;
using namespace support::cl;

//--------------------------------------------------------------------------------------------------
// CmdLine
//

CmdLine::CmdLine(unsigned flags)
    : flags_(flags)
    , maxPrefixLength_(0)
{
}

bool CmdLine::add(OptionBase* opt)
{
    if (opt->formatting() == Positional)
    {
        positionals_.push_back(opt);
        return true;
    }

    auto insert = [&](StringRef s, OptionBase* opt) -> bool
    {
        // Save the length of the longest prefix option
        if (opt->isPrefix())
        {
            if (maxPrefixLength_ < s.size())
                maxPrefixLength_ = s.size();
        }

        // Insert the option into the map
        return options_.insert({ s, opt }).second;
    };

    if (opt->name().empty())
    {
        std::vector<StringRef> values;

        opt->allowedValues(values);

        if (values.empty())
            return false;

        for (auto const& s : values)
        {
            if (!insert(s, opt))
                return false;
        }
    }
    else
    {
        for (auto const& s : strings::split(opt->name(), "|"))
        {
            if (!insert(s, opt))
                return false;
        }
    }

    return true;
}

bool CmdLine::parse(StringVector const& argv)
{
    bool success = true;
    bool dashdash = false; // "--" seen?

    // The current positional argument - if any
    auto pos = positionals_.begin();

    // Handle all arguments...
    for (size_t N = 0; N < argv.size(); ++N)
    {
        if (!handleArg(argv, N, pos, dashdash))
        {
            if (flags_ & StopOnFirstError)
                return false;
            else
                success = false;
        }
    }

    if (flags_ & IgnoreMissingOptions)
        return true;

    // Check if all required options have been successfully parsed
    return check() && success;
}

bool CmdLine::expandAndParse(StringVector argv)
{
    if (!expandResponseFiles(argv))
        return false;

    return parse(argv);
}

CmdLine::ConstOptionVector CmdLine::options() const
{
    ConstOptionVector opts;

    // Get the list of all (visible) options
    for (auto const& I : options_)
        opts.emplace_back(I.second);

    // Sort by name
    std::stable_sort(opts.begin(), opts.end(),
        [](OptionBase const* LHS, OptionBase const* RHS) {
            return LHS->name() < RHS->name();
        });

    // Remove duplicates
    opts.erase(std::unique(opts.begin(), opts.end()), opts.end());

    return opts;
}

CmdLine::ConstOptionVector CmdLine::positionals() const
{
    return ConstOptionVector(positionals_.begin(), positionals_.end());
}

OptionBase* CmdLine::findOption(StringRef name) const
{
    auto I = options_.find(name);
    return I == options_.end() ? 0 : I->second;
}

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

// Recursively expand response files.
// Returns true on success, false otherwise.
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

// Process a single command line argument.
// Returns true on success, false otherwise.
bool CmdLine::handleArg(StringVector const& argv, size_t& i, OptionVector::iterator& pos, bool& dashdash)
{
    StringRef arg = argv[i];

    // Stop parsing if "--" has been found
    if (arg == "--" && !dashdash)
    {
        dashdash = true;
        return true;
    }

    bool ignoreUnknowns = (flags_ & IgnoreUnknownOptions) != 0;

    // This argument is considered to be positional if it doesn't start with '-', if it is "-"
    // itself, or if we have seen "--" already.
    if (arg[0] != '-' || arg == "-" || dashdash)
    {
        if (handlePositional(argv, i, pos, arg))
        {
            // If the current positional argument has the ConsumeAfter flag set, parse
            // all following command-line arguments as positional options.
            if ((*pos)->flags() & ConsumeAfter)
                dashdash = true;

            return true;
        }

        // Unhandled positional argument...
        unknowns_.push_back(arg.str());

        return ignoreUnknowns || error("unknown positional option: '" + arg + "'");
    }

    // Starts with a dash, must be an argument.
    auto name = arg.drop_front(1);

    bool short_option = name[0] != '-';

    if (!short_option)
        name = name.drop_front(1);

    bool success = true;

    // Try to process this argument as a standard option.
    if (handleOption(argv, i, success, name))
    {
        return success;
    }

    // If it's not a standard option and there is no equals sign,
    // check for a prefix option.
    if (handlePrefix(argv, i, success, name))
    {
        return success;
    }

    // If it's not standard or prefix option, check for an option group
    if (short_option && handleGroup(argv, i, success, name))
    {
        return success;
    }

    // Unknown option specified...
    unknowns_.push_back(arg.str());

    return ignoreUnknowns || error("unknown option '" + arg + "'");
}

bool CmdLine::handlePositional(StringVector const& argv, size_t& i, OptionVector::iterator& pos, StringRef arg)
{
    if (pos == positionals_.end())
    {
        return false;
    }

    auto opt = *pos;

    // If the current positional argument does not allow any further
    // occurrence try the next.
    if (!opt->isOccurrenceAllowed())
    {
        return handlePositional(argv, i, ++pos, arg);
    }

    // The value of a positional option is the argument itself.
    return addOccurrence(argv, i, opt, (*pos)->displayName(), arg);
}

// If 'arg' is the name of an option, process the option immediately.
// Otherwise looks for an equal sign and try again.
bool CmdLine::handleOption(StringVector const& argv, size_t& i, bool& success, StringRef arg)
{
    if (auto opt = findOption(arg)) // Standard option?
    {
        success = addOccurrence(argv, i, opt, arg, {});
        return true;
    }

    // Get the index of the first equals sign
    auto I = arg.find('=');

    if (I == StringRef::npos)
    {
        return false;
    }

    // Get the option name
    auto name = arg.front(I);

    if (auto opt = findOption(name))
    {
        // Include the equals sign in the value if this option is a prefix option.
        // Discard otherwise.
        arg = arg.drop_front(opt->isPrefix() ? I : I + 1);

        success = addOccurrence(argv, i, opt, name, arg);
        return true;
    }

    return false;
}

// Handles prefix options which have an argument specified without an equals sign.
bool CmdLine::handlePrefix(StringVector const& argv, size_t& i, bool& success, StringRef arg)
{
    assert(!arg.empty());

    for (auto n = std::min(maxPrefixLength_, arg.size()); n != 0; --n)
    {
        auto name = arg.front(n);
        auto opt = findOption(name);

        if (opt && opt->isPrefix())
        {
            success = addOccurrence(argv, i, opt, name, arg.drop_front(n));
            return true;
        }
    }

    return false;
}

// Process an option group.
bool CmdLine::handleGroup(StringVector const& argv, size_t& i, bool& success, StringRef name)
{
    OptionVector group;

    group.reserve(name.size());

    // First check if the name consists of single letter options
    for (size_t n = 0; n != name.size(); ++n)
    {
        auto opt = findOption(name.substr(n, 1));

        if (opt == 0 || opt->formatting() != Grouping)
            return false;
        else
            group.push_back(opt);
    }

    assert(!group.empty());

    // The first N - 1 options must not require an argument.
    // The last option might allow an argument.
    for (size_t n = 0; n != group.size() - 1; ++n)
    {
        if (group[n]->numArgs() == ArgRequired)
        {
            success = error("option '" + group[n]->displayName()
                + "' requires an argument (must be last in '" + name + "')");
            return true;
        }
    }

    success = true;

    // Then handle all the options
    for (auto opt : group)
    {
        success = addOccurrence(argv, i, opt, opt->name(), {})
                  && success;
    }

    return true;
}

bool CmdLine::parse(OptionBase* opt, StringRef name, StringRef arg, size_t i)
{
    if (opt->parse(name, arg, i))
    {
        opt->count_++;
        return true;
    }

    return error("invalid argument '" + arg + "' for option '" + name + "'");
}

bool CmdLine::addOccurrence(StringVector const& argv,
                            size_t& i,
                            OptionBase* opt,
                            StringRef name,
                            StringRef arg)
{
    StringRef dname = opt->displayName();

    // Check if an occurrence is allowed
    if (!opt->isOccurrenceAllowed())
    {
        return error("option '" + name + "' already specified");
    }

    size_t index = i;

    if (opt->formatting() != Positional)
    {
        // If an argument was specified, check if this is allowed.
        if (arg.data() && opt->numArgs() == ArgDisallowed)
        {
            return error("option '" + dname + "' doesn't allow an argument");
        }

        // If the option requires an argument, "steal" the next argument from the
        // command line, so that "-o file" is possible instead of "-o=file"
        if (!arg.data() && opt->numArgs() == ArgRequired)
        {
            if (opt->formatting() == Prefix || i + 1 >= argv.size())
            {
                return error("option '" + dname + "' requires an argument");
            }

            arg = argv[++i];
        }
    }

    // If this option takes a comma-separated list of arguments, split the string
    // and add all arguments separately.
    if (opt->flags() & CommaSeparated)
    {
        for (auto v : strings::split(arg, ","))
        {
            if (!parse(opt, name, v, index))
                return false;
        }

        return true;
    }

    // Otherwise, just parse the option.
    return parse(opt, name, arg, index);
}

bool CmdLine::check()
{
    bool success = true;

    for (auto& I : options())
        success = check(I) && success;

    for (auto& I : positionals())
        success = check(I) && success;

    for (auto& I : groups_)
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
    if (flags_ & PrintErrors)
        std::cout << "command-line error: " << str << std::endl;

    errors_.push_back(std::move(str));
    return false;
}

//--------------------------------------------------------------------------------------------------
// OptionGroup
//

OptionGroup::OptionGroup(CmdLine& cmd, std::string name, Type type)
    : name_(std::move(name))
    , type_(type)
{
    if (!cmd.groups_.insert({name_, this}).second)
        throw std::runtime_error("failed to register option group '" + name + "'");
}

bool OptionGroup::check() const
{
    if (type_ == OptionGroup::Default)
        return true;

    // Count the number of options in this group which have been specified on the command-line
    size_t N = std::count_if(options_.begin(), options_.end(),
        [](OptionBase const* x) {
            return x->count() > 0;
        });

    switch (type_)
    {
    case Default: // prevent warning
        return true;
    case Zero:
        return N == 0;
    case ZeroOrOne:
        return N == 0 || N == 1;
    case One:
        return N == 1;
    case OneOrMore:
        return N >= 1;
    case All:
        return N == options_.size();
    case ZeroOrAll:
        return N == 0 || N == options_.size();
    }

    return true; // prevent warning
}

std::string OptionGroup::desc() const
{
    switch (type_)
    {
    case Default:
        return "any number of options in group '" + name_ + "' may be specified";
    case Zero:
        return "no options in group '" + name_ + "' may be specified";
    case ZeroOrOne:
        return "at most one option in group '" + name_ + "' may be specified";
    case One:
        return "exactly one option in group '" + name_ + "' must be specified";
    case OneOrMore:
        return "at least one option in group '" + name_ + "' must be specified";
    case All:
        return "all options in group '" + name_ + "' must be specified";
    case ZeroOrAll:
        return "none or all options in group '" + name_ + "' must be specified";
    }

    assert(!"internal error");
    return "xxx";
}

//--------------------------------------------------------------------------------------------------
// OptionBase
//

OptionBase::OptionBase()
    : name_()
    , argName_()
    , desc_("**** Documentation missing ****")
    , numOccurrences_(Optional)
    , numArgs_(ArgOptional)
    , formatting_(DefaultFormatting)
    , miscFlags_(None)
    , count_(0)
{
}

OptionBase::~OptionBase()
{
}

bool OptionBase::isUnbounded() const
{
    return numOccurrences_ == ZeroOrMore || numOccurrences_ == OneOrMore;
}

bool OptionBase::isRequired() const
{
    return numOccurrences_ == Required || numOccurrences_ == OneOrMore;
}

bool OptionBase::isPrefix() const
{
    return formatting_ == Prefix || formatting_ == MayPrefix;
}

StringRef OptionBase::displayName() const
{
    if (name_.empty())
        return argName_;

    return name_;
}

bool OptionBase::isOccurrenceAllowed() const
{
    if (numOccurrences_ == Optional || numOccurrences_ == Required)
        return count_ == 0;

    return true;
}

bool OptionBase::isOccurrenceRequired() const
{
    if (numOccurrences_ == Required || numOccurrences_ == OneOrMore)
        return count_ == 0;

    return false;
}

void OptionBase::applyRec()
{
    assert((formatting_ != Positional || !name_.empty())
        && "positional options need a name");

    if (argName_.empty())
        argName_ = "arg";
}
