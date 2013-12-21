// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/StringSplit.h"

#include "CmdLineQt.h"
#include "CmdLineWithIndex.h"
#include "PrettyPrint.h"

#include <functional>
#include <iostream>
#include <set>

using namespace support;

namespace support
{
namespace cl
{

    template <class T, class U>
    struct Parser<std::pair<T, U>>
    {
        bool operator ()(StringRef spec, StringRef value, size_t i, std::pair<T, U>& result) const
        {
            auto p = strings::split(value, ":")();

            return Parser<T>()(spec, p.first, i, result.first)
                && Parser<U>()(spec, p.second, i, result.second);
        }
    };

    template <class T, class P>
    void prettyPrint(std::ostream& stream, Option<T, P> const& option)
    {
        stream << option.getName() << ":\n";
        stream << "  count = " << option.getCount() << "\n";
        stream << "  value = " << pretty(option.get());
    }

    template <class T>
    void prettyPrint(std::ostream& stream, WithIndex<T> const& x)
    {
        stream << "(" << x.index << ": " << pretty(x.value) << ")";
    }

} // namespace cl
} // namespace support

struct WFlagParser
{
    bool operator ()(StringRef spec, StringRef /*value*/, size_t /*i*/, bool& result) const
    {
        result = !spec.starts_with("Wno-");
        return true;
    }
};

template <class... A>
inline cl::Option<bool, WFlagParser> makeWFlag(A&&... args)
{
    return cl::makeOptionWithParser<bool>(WFlagParser(), std::forward<A>(args)..., cl::ArgDisallowed, cl::ZeroOrMore);
}

int main(int argc, char* argv[])
{
    //----------------------------------------------------------------------------------------------

    cl::CmdLine cmd;

                    //------------------------------------------------------------------------------

    double y = -1.0;

    auto y_ref = cl::makeOption<double&>(cmd, "y",
        cl::ArgName("float"),
        cl::Desc("Enter a floating-point number"),
        cl::ArgRequired,
        cl::init(y)
        );

                    //------------------------------------------------------------------------------

    auto g  = cl::makeOption<bool>(cmd, "g", cl::Grouping, cl::ZeroOrMore);
    auto h  = cl::makeOption<bool>(cmd, "h", cl::Grouping, cl::ZeroOrMore);
    auto gh = cl::makeOption<bool>(cmd, "gh", cl::Prefix, cl::ArgRequired);

                    //------------------------------------------------------------------------------

    auto z = cl::makeOption<std::set<int>>(cmd, "z",
        cl::ArgName("int"),
        cl::Desc("A list of integers"),
        cl::ZeroOrMore,
        cl::ArgRequired,
        cl::CommaSeparated
        );

                    //------------------------------------------------------------------------------

    std::initializer_list<cl::WithIndex<std::string>> Iinit {
        "eins", "zwei", "drei", "vier", "funf"
    };

    auto I = cl::makeOption<std::vector<cl::WithIndex<std::string>>>(cmd, "I",
        cl::ArgName("dir"),
        cl::Desc(
            // Test the word wrap algorithm...
            "Add the directory dir to the list of directories to be searched for header files. "
            "Directories named by -I are searched before the standard system include directories. "
            "If the directory dir is a standard system include directory, the option is ignored to "
            "ensure that the default search order for system directories and the special treatment "
            "of system headers are not defeated . If dir begins with =, then the = will be replaced "
            "by the sysroot prefix; see --sysroot and -isysroot."
            ),
        cl::Prefix,
        cl::ArgRequired,
        cl::ZeroOrMore,
        cl::init(Iinit)
        );

                    //------------------------------------------------------------------------------

    auto files = cl::makeOption<std::vector<std::string>>(cmd, "files",
        cl::Positional,
        cl::ZeroOrMore
        );

                    //------------------------------------------------------------------------------

    enum OptimizationLevel {
        OL_None,
        OL_Trivial,
        OL_Default,
        OL_Expensive
    };

    auto optParser = cl::MapParser<OptimizationLevel>({
        { "O0", OL_None,      "No optimizations"             },
        { "O1", OL_Trivial,   "Enable trivial optimizations" },
        { "O2", OL_Default,   "Enable some optimizations"    },
        { "O3", OL_Expensive, "Enable all optimizations"     }
    });

    auto opt = cl::makeOptionWithParser<OptimizationLevel>(
        std::ref(optParser),
        cmd,
        cl::ArgName("optimization level"),
        cl::Required,
        cl::ArgDisallowed,
        cl::Desc("Choose an optimization level"),
        cl::init(OL_None)
        );

                    //------------------------------------------------------------------------------

    enum Simpson {
        Homer, Marge, Bart, Lisa, Maggie, SideshowBob
    };

    auto simpsonParser = cl::MapParser<Simpson>({
        { "homer",        Homer,       "Homer Jay Simpson"            },
        { "marge",        Marge,       "Marjorie Simpson"             },
        { "bart",         Bart,        "Bartholomew JoJo Simpson"     },
        { "el barto",     Bart,        "?"                            },
        { "lisa",         Lisa,        "Lisa Marie Simpson"           },
        { "maggie",       Maggie,      "Margaret Simpson"             },
//      { "sideshow bob", SideshowBob, "Robert Underdunk Terwilliger" },
    });

    auto simpson = cl::makeOptionWithParser<Simpson>(
        simpsonParser,
        cmd, "simpson",
        cl::Desc("Choose a Simpson"),
        cl::ArgRequired,
        cl::init(SideshowBob)
        );

                    //------------------------------------------------------------------------------

    auto f = cl::makeOption<std::map<std::string, int>>(
        cmd, "f",
        cl::ArgName("string:int"),
        cl::CommaSeparated,
        cl::ArgRequired
        );

                    //------------------------------------------------------------------------------

    auto debug_level = cl::makeOption<int>(cmd, "debug-level|d",
        cl::Desc("Specify a debug-level"),
        cl::Optional,
        cl::ArgRequired
        );

                    //------------------------------------------------------------------------------

    auto Wsign_conversion = makeWFlag(
        cmd, "Wsign-conversion|Wno-sign-conversion",
        cl::Desc("Warn for implicit conversions that may change the sign of an integer value.")
        );

    auto Wsign_compare = makeWFlag(
        cmd, "Wsign-compare|Wno-sign-compare",
        cl::Desc("Warn when a comparison between signed and unsigned values could produce "
                 "an incorrect result when the signed value is converted to unsigned.")
        );

                    //------------------------------------------------------------------------------

    struct Targets
    {
        std::set<std::string> list{{"x", "y", "z"}};
    };

    auto targetsParser = [](StringRef name, StringRef arg, size_t /*i*/, Targets& value)
    {
        if (name.starts_with("without-"))
            value.list.erase(arg.str());
        else
            value.list.insert(arg.str());

        return true;
    };

    auto targets = cl::makeOptionWithParser<Targets>(
        targetsParser,
        cmd, "without-|with-",
        cl::Desc("Specifiy which targets to build"),
        cl::ArgName("target"),
        cl::Prefix,
        cl::ArgRequired,
        cl::CommaSeparated,
        cl::ZeroOrMore
        );

    //----------------------------------------------------------------------------------------------

    bool success = cmd.parse({ argv + 1, argv + argc }, /*ignoreUnknowns*/ false);

    if (!success)
    {
        for (auto const& s : cmd.getErrors())
            std::cout << "error: " << s << "\n";

        return -1;
    }

    //----------------------------------------------------------------------------------------------

    std::cout << pretty(f) << std::endl;
    std::cout << pretty(g) << std::endl;
    std::cout << pretty(gh) << std::endl;
    std::cout << pretty(h) << std::endl;
    std::cout << pretty(I) << std::endl;
    std::cout << pretty(opt) << std::endl;
    std::cout << pretty(simpson) << std::endl;
    std::cout << pretty(y_ref) << std::endl;
    std::cout << pretty(z) << std::endl;
    std::cout << pretty(debug_level) << std::endl;
    std::cout << pretty(Wsign_conversion) << std::endl;
    std::cout << pretty(Wsign_compare) << std::endl;

    std::cout << "Targets: " << pretty(targets.get().list) << std::endl;

    std::cout << "Files:\n";
    for (auto& s : files.get())
        std::cout << "  \"" << s << "\"" << std::endl;

    //----------------------------------------------------------------------------------------------

    return 0;
}
