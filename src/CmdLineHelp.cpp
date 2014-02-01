// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLineHelp.h"
#include "Support/StringSplit.h"

#include <algorithm>
#include <ostream>

using namespace support;
using namespace support::cl;

//--------------------------------------------------------------------------------------------------
// Misc.
//

namespace
{

const size_t kMaxWidth   = 78;
const size_t kIndent     = 2;
const size_t kOffset     = kMaxWidth / 3;

// Returns a string consisting of N spaces.
StringRef Spaces(size_t N)
{
    static std::vector<char> str(100, ' ');

    if (str.size() < N)
        str.resize(N, ' ');

    return { &str[0], N };
}

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

std::ostream& operator<<(std::ostream& stream, Aligned const& x)
{
    if (x.text.size() < x.indent)
        return stream << x.text << Spaces(x.indent - x.text.size());

    return stream << x.text << "\n" << Spaces(x.indent);
}

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

std::ostream& operator<<(std::ostream& stream, Wrapped const& x)
{
    bool first = true;

    // Break the string into paragraphs
    for (auto par : strings::split(x.text, "\n"))
    {
        // Break the string at the maximum width into lines
        for (auto line : strings::split(par, strings::WrapDelimiter(x.maxWidth - x.indent)))
        {
            if (first)
                first = false;
            else
                stream << "\n" << Spaces(x.indent);

            stream << line;
        }
    }

    return stream;
}

template <class Iterator>
void PrintHelp(std::ostream& stream, StringRef text, Iterator first, Iterator last)
{
    if (first == last)
        return;

    stream << text << "\n";

    for (; first != last; ++first)
        help(stream, *first);

    stream << "\n";
}

template <class Range>
void PrintHelp(std::ostream& stream, StringRef text, Range const& range)
{
    using std::begin;
    using std::end;

    PrintHelp(stream, text, begin(range), end(range));
}

} // namespace

//--------------------------------------------------------------------------------------------------
// Help
//

std::string cl::usage(CmdLine const& cmd)
{
    std::string str = "[options]";

    for (auto opt : cmd.positionals())
    {
        str += " " + usage(opt);
    }

    return str;
}

std::string cl::usage(OptionBase const* o)
{
    if (o->formatting() == Positional)
        return "<" + o->name() + (o->isUnbounded() ? ">..." : ">");

    switch (o->numArgs())
    {
    case ArgDisallowed:
        return "-" + o->name();
    case ArgOptional:
        return "-" + o->name() + "[=<" + o->argName() + ">]";
    case ArgRequired:
        return "-" + o->name() + (o->isPrefix() ? "<" : " <") + o->argName() + ">";
    }

    assert(!"internal error");
    return {};
}

void cl::help(std::ostream& stream, CmdLine const& cmd, StringRef overview)
{
    if (!overview.empty())
    {
        stream << "Overview:\n  " << Wrapped(overview, kIndent, kMaxWidth) << "\n\n";
    }

    stream << "Usage:\n  " << usage(cmd) << "\n\n";

    auto opts = cmd.options();

    // Print positional options
    PrintHelp(stream, "Positional options:", cmd.positionals());

    // Print required options first.
    auto E = std::stable_partition(opts.begin(), opts.end(),
        [](OptionBase const* x)
        {
            return x->isRequired();
        }
    );

    // Print all the required options - if any
    PrintHelp(stream, "Required options:", opts.begin(), E);

    // Print all other options
    PrintHelp(stream, "Options:", E, opts.end());
}

void cl::help(std::ostream& stream, OptionBase const* o)
{
    // Do not show hidden options
    if (o->flags() & Hidden)
        return;

//    // Do not show options without a description
//    if (o->desc.empty())
//        return;

    // Get the list of allowed values for this option.
    auto values = o->allowedValues();

    // If the option does not have a restricted set of allowed values,
    // just print the short usage and the description for the option.
    if (values.empty())
    {
        stream << Aligned(Spaces(kIndent) + usage(o), kOffset)
               << Wrapped(o->desc(), kOffset, kMaxWidth)
               << "\n";
        return;
    }

    // This option only allows a limited set of input values.
    // Show all valid values and their descriptions.

    // Get the list of descriptions for the allowed values.
    auto descr = o->descriptions();

    assert(descr.size() == values.size() && "not supported");

    if (o->name().empty())
    {
        // The option name is empty, i.e. this option is actually a group of
        // options. Print the description only.
        stream << Wrapped(Spaces(kIndent) + o->desc() + ":", kIndent, kMaxWidth)
               << "\n";
    }
    else
    {
        // Named alternative. Print the name and the description.
        stream << Aligned(Spaces(kIndent) + usage(o), kOffset)
               << Wrapped(o->desc() + ":", kOffset, kMaxWidth)
               << "\n";
    }

    auto prefix = Spaces(kIndent * 2) + (o->name().empty() ? "-" : "=");

    // Finally, print all the possible values with their descriptions.
    for (size_t I = 0; I < values.size(); ++I)
    {
        stream << Aligned(prefix + values[I], kOffset)
               << Wrapped("- " + descr[I], kOffset + 2, kMaxWidth)
               << "\n";
    }
}

void cl::help(std::ostream& /*stream*/, OptionGroup const* /*g*/)
{
    assert(!"not implemented");
}
