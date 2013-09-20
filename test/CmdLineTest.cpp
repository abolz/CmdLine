// This file is distributed under the MIT license.
// See the LICENSE file for details.


#include "Support/CmdLine.hh"
#include "Support/PrettyPrint.hh"

#include <algorithm>
#include <functional>
#include <iostream>
#include <map>
#include <set>
#include <unordered_map>
#include <climits>
#include <type_traits>


namespace cl = support::cl;

using support::StringRef;
using support::pretty;


namespace support {
namespace cl {

    template<class T, class U>
    struct Parser<std::pair<T, U>>
    {
        bool operator ()(StringRef value, size_t i, std::pair<T, U>& result) const
        {
            auto p = value.split(':');

            return Parser<T>()(p.first.trim(), i, result.first)
                && Parser<U>()(p.second.trim(), i, result.second);
        }
    };

    template<class T, class P>
    void prettyPrint(std::ostream& stream, Option<T, P> const& option)
    {
        stream << option.getName() << ":\n";
        stream << "  count = " << option.getCount() << "\n";
        stream << "  value = " << pretty(option.get());
    }

}}


int main(int argc, char* argv[])
{
    //----------------------------------------------------------------------------------------------

    cl::CmdLine cmd(argv[0], "A short description.\n\nA long description.");

                    //------------------------------------------------------------------------------

    double y = -1.0;

    auto y_ref = cl::makeOption<double&>(
        cmd, "y",
        cl::ArgName("float"),
        cl::Desc("Enter a floating-point number"),
        cl::ArgRequired,
        cl::init(y)
        );

                    //------------------------------------------------------------------------------

    auto g = cl::makeOption<bool>(cmd, "g", cl::Grouping, cl::ZeroOrMore);
    auto h = cl::makeOption<bool>(cmd, "h", cl::Grouping, cl::ZeroOrMore);

    //auto gh = cl::makeOption<bool>(cmd, "gh", cl::Prefix);

                    //------------------------------------------------------------------------------

    auto z = cl::makeOption<std::set<int>>(
        cmd, "z",
        cl::ArgName("int"),
        cl::Desc("A list of integers"),
        cl::ZeroOrMore,
        cl::ArgRequired,
        cl::CommaSeparated
        );

                    //------------------------------------------------------------------------------

    std::initializer_list<std::string> Iinit = {
        "eins", "zwei", "drei", "vier", "funf"
    };

    auto I = cl::makeOption<std::vector<std::string>>(
        cmd, "I",
        cl::ArgName("dir"),
        cl::Desc("Add the directory dir to the list of directories to be searched for header files."),
        cl::Prefix,
        cl::ZeroOrMore,
        cl::init(Iinit)
        );

                    //------------------------------------------------------------------------------

////    auto regexParser = [](StringRef value, size_t, std::regex& result) -> bool
////    {
////        result = value.str();
////        return true;
////    };
//
//    std::regex regex_;
//
//    auto regex = cl::makeOption<std::regex&>(
//        cmd,
//        cl::init(regex_), "regex", cl::Positional, cl::Required
//        );

                    //------------------------------------------------------------------------------

    auto files = cl::makeOption<std::vector<std::string>>(cmd, "files", cl::Positional, cl::ZeroOrMore);

                    //------------------------------------------------------------------------------

    enum OptLevel : unsigned {
        None, Trivial, Default, Expensive
    };

    auto optParser = cl::MapParser<OptLevel>({
        { "O0", None        },
        { "O1", Trivial     },
        { "O2", Default     },
        { "O3", Expensive   }
    });

    auto opt = cl::makeOptionPiecewise<OptLevel>(
        optParser,
        cmd,
        cl::ArgDisallowed,
        cl::Desc("Choose an optimization level"),
        cl::init(None)
        );

                    //------------------------------------------------------------------------------

    enum Simpson {
        Homer, Marge, Bart, Lisa, Maggie, SideshowBob
    };

    auto simpsonParser = cl::MapParser<Simpson>({
        { "homer",       Homer  },
        { "marge",       Marge  },
        { "bart",        Bart   },
        { "el barto",    Bart   },
        { "lisa",        Lisa   },
        { "maggie",      Maggie }
    });

    auto simpson = cl::makeOptionPiecewise<Simpson>(
        simpsonParser,
        cmd, "simpson",
        cl::Desc("Choose a Simpson"),
        cl::ArgRequired,
        cl::init(SideshowBob)
        );

                    //------------------------------------------------------------------------------

    auto bf = cl::makeOptionPiecewise<unsigned>(
        cl::BinaryOpParser<std::bit_or<unsigned>>(),
        cmd, "bf",
        cl::CommaSeparated,
        cl::ArgRequired,
        cl::ZeroOrMore
        );

                    //------------------------------------------------------------------------------

    auto f = cl::makeOption<std::map<std::string, int>>(
        cmd, "f", cl::CommaSeparated, cl::ArgRequired
        );

                    //------------------------------------------------------------------------------

    auto helpParser = [&](StringRef /*value*/, size_t /*i*/, bool& /*result*/) -> bool
    {
        cmd.help();
        exit(0);
    };

    auto help = cl::makeOptionPiecewise<bool>(helpParser, cmd, "help", cl::Optional);

    //----------------------------------------------------------------------------------------------

    if (!cmd.parse({argv + 1, argv + argc}, /*ignoreUnknowns*/false))
    {
        for (auto const& s : cmd.getErrors())
            std::cout << "error: " << s << "\n";

        return -1;
    }

    //----------------------------------------------------------------------------------------------

    std::cout << pretty(y_ref) << std::endl;
    std::cout << pretty(g) << std::endl;
    std::cout << pretty(h) << std::endl;
    std::cout << pretty(z) << std::endl;
    std::cout << pretty(I) << std::endl;
    std::cout << pretty(opt) << std::endl;
    std::cout << pretty(simpson) << std::endl;
    std::cout << pretty(help) << std::endl;
//    std::cout << pretty(regex) << std::endl;
    std::cout << "bf = 0x" << std::hex << bf.get() << std::endl;
    std::cout << pretty(f) << std::endl;

    std::cout << "files:\n";
    for (auto& s : files)
        std::cout << "  \"" << s << "\"" << std::endl;

    //----------------------------------------------------------------------------------------------

    return 0;
}
