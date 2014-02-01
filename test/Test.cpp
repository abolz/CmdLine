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

    template <class T, template <class> class TraitsT, class ParserT>
    void prettyPrint(std::ostream& stream, Option<T, TraitsT, ParserT> const& option)
    {
        stream << option.name() << ":\n";
        stream << "  count = " << option.count() << "\n";
        stream << "  value = " << pretty(option.value());
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
    void operator ()(StringRef name, StringRef /*arg*/, size_t /*i*/, bool& value) const
    {
        value = !name.starts_with("Wno-");
    }
};

template <class... Args>
inline auto makeWFlag(Args&&... args)
    -> decltype( cl::makeOption<bool>(cl::InitParser, WFlagParser()) )
{
    return cl::makeOption<bool>(cl::InitParser, WFlagParser(),
                    std::forward<Args>(args)..., cl::ArgDisallowed, cl::ZeroOrMore);
}

int main(int argc, char* argv[])
{
    //----------------------------------------------------------------------------------------------

    cl::CmdLine cmd;

                    //------------------------------------------------------------------------------

    auto help = cl::makeOption<std::string>(
        cmd, "help",
        cl::ArgName("option"),
        cl::ArgOptional
        );

                    //------------------------------------------------------------------------------

    double y = -1.0;

    auto y_ref = cl::makeOption<double&>(cmd, "y",
        cl::ArgName("float"),
        cl::ArgRequired,
        cl::init(y)
        );

                    //------------------------------------------------------------------------------

    auto g  = cl::makeOption<bool>(cmd, "g", cl::Grouping, cl::ArgDisallowed, cl::ZeroOrMore);
    auto h  = cl::makeOption<bool>(cmd, "h", cl::Grouping, cl::ArgDisallowed, cl::ZeroOrMore);
    auto gh = cl::makeOption<bool>(cmd, "gh", cl::Prefix, cl::ArgRequired);

                    //------------------------------------------------------------------------------

    auto z = cl::makeOption<std::set<int>>(cmd, "z",
        cl::ArgName("int"),
        cl::ArgRequired,
        cl::CommaSeparated,
        cl::ZeroOrMore
        );

                    //------------------------------------------------------------------------------

    std::initializer_list<cl::WithIndex<std::string>> Iinit {
        "eins", "zwei", "drei", "vier", "funf"
    };

    auto I = cl::makeOption<std::vector<cl::WithIndex<std::string>>>(cmd, "I",
        cl::ArgName("dir"),
        cl::ArgRequired,
        cl::init(Iinit),
        cl::Prefix,
        cl::ZeroOrMore
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
        { "O0", OL_None      },
        { "O1", OL_Trivial   },
        { "O2", OL_Default   },
        { "O3", OL_Expensive },
    });

    auto opt = cl::makeOption<OptimizationLevel>(
        cl::InitParser, std::ref(optParser),
        cmd,
        cl::ArgDisallowed,
        cl::ArgName("optimization level"),
        cl::init(OL_None),
        cl::Required
        );

                    //------------------------------------------------------------------------------

    enum Simpson {
        Homer, Marge, Bart, Lisa, Maggie, SideshowBob
    };

    auto simpsonParser = cl::MapParser<Simpson>({
        { "homer",        Homer       },
        { "marge",        Marge       },
        { "bart",         Bart        },
        { "el barto",     Bart        },
        { "lisa",         Lisa        },
        { "maggie",       Maggie      },
//      { "sideshow bob", SideshowBob },
    });

    auto simpson = cl::makeOption<Simpson>(
        cl::InitParser, simpsonParser,
        cmd, "simpson",
        cl::ArgRequired,
        cl::init(SideshowBob)
        );

                    //------------------------------------------------------------------------------

    auto f = cl::makeOption<std::map<std::string, int>, cl::Traits>(
        cl::InitParser, [](StringRef name, StringRef arg, size_t i, std::pair<std::string, int>& value)
        {
            auto p = strings::split(arg, ":")();

            cl::Parser<std::string>()(name, p.first, i, value.first);
            cl::Parser<int>()(name, p.second, i, value.second);
        },
        cmd, "f",
        cl::ArgName("string:int"),
        cl::ArgRequired,
        cl::CommaSeparated
        );

                    //------------------------------------------------------------------------------

    auto debug_level = cl::makeOption<int>(cmd, "debug-level|d",
        cl::ArgRequired,
        cl::Optional
        );

                    //------------------------------------------------------------------------------

    auto Wsign_conversion = makeWFlag(cmd, "Wsign-conversion|Wno-sign-conversion");

    auto Wsign_compare = makeWFlag(cmd, "Wsign-compare|Wno-sign-compare");

                    //------------------------------------------------------------------------------

    auto targetsParser = [](StringRef name, StringRef arg, size_t /*i*/, std::set<std::string>& value)
    {
        if (name.starts_with("without-"))
            value.erase(arg.str());
        else
            value.insert(arg.str());
    };

    auto targets = cl::makeOption<std::set<std::string>, cl::ScalarType>(
        cl::InitParser, targetsParser,
        cmd, "without-|with-",
        cl::ArgName("target"),
        cl::ArgRequired,
        cl::CommaSeparated,
        cl::Prefix,
        cl::ZeroOrMore
        );

    //----------------------------------------------------------------------------------------------

    try
    {
        cmd.parse({ argv + 1, argv + argc });
    }
    catch (std::exception& e)
    {
        std::cout << "error: " << e.what() << std::endl;
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
    std::cout << pretty(targets) << std::endl;

    std::cout << "Files:\n";
    for (auto& s : files.value())
        std::cout << "  \"" << s << "\"" << std::endl;

    //----------------------------------------------------------------------------------------------

    return 0;
}
