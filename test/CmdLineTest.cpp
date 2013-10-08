// This file is distributed under the MIT license.
// See the LICENSE file for details.


#include "Support/CmdLine.h"
#include "Support/PrettyPrint.h"

#include <algorithm>
#include <functional>
#include <iostream>
#include <set>


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
        cmd,
        cl::Name("y"),
        cl::ArgName("float"),
        cl::Desc("Enter a floating-point number"),
        cl::ArgRequired,
        cl::init(y)
        );

                    //------------------------------------------------------------------------------

    auto g = cl::makeOption<bool>(
        cmd,
        cl::Name("g"),
        cl::Grouping,
        cl::ZeroOrMore
        );

    auto h = cl::makeOption<bool>(
        cmd,
        cl::Name("h"),
        cl::Grouping,
        cl::ZeroOrMore
        );

    //auto gh = cl::makeOption<bool>(cmd, "gh", cl::Prefix);

                    //------------------------------------------------------------------------------

    auto z = cl::makeOption<std::set<int>>(
        cmd,
        cl::Name("z"),
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
        cmd,
        cl::Name("I"),
        cl::ArgName("dir"),
        cl::Desc("Add the directory dir to the list of directories to be searched for header files."),
        cl::Prefix,
        cl::ZeroOrMore,
        cl::init(Iinit)
        );

                    //------------------------------------------------------------------------------

    auto files = cl::makeOption<std::vector<std::string>>(
        cmd,
        cl::Name("files"),
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
        { "O0", OL_None        },
        { "O1", OL_Trivial     },
        { "O2", OL_Default     },
        { "O3", OL_Expensive   }
    });

    auto opt = cl::makeOptionPiecewise<OptimizationLevel>(
        optParser,
        cmd,
        cl::ArgDisallowed,
        cl::Desc("Choose an optimization level"),
        cl::init(OL_None),
        cl::init(OL_None)
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
        cmd,
        cl::Name("simpson"),
        cl::Desc("Choose a Simpson"),
        cl::ArgRequired,
        cl::init(SideshowBob)
        );

                    //------------------------------------------------------------------------------

    auto bf = cl::makeOptionPiecewise<unsigned>(
        cl::BinaryOpParser<std::bit_or<unsigned>>(),
        cmd,
        cl::Name("bf"),
        cl::CommaSeparated,
        cl::ArgRequired,
        cl::ZeroOrMore
        );

                    //------------------------------------------------------------------------------

    auto f = cl::makeOption<std::map<std::string, int>>(
        cmd,
        cl::Name("f"),
        cl::CommaSeparated,
        cl::ArgRequired
        );

                    //------------------------------------------------------------------------------

    auto helpParser = [&](StringRef /*value*/, size_t /*i*/, bool& /*result*/) -> bool
    {
        cmd.help();
        exit(0);
    };

    auto help = cl::makeOptionPiecewise<bool>(
        helpParser,
        cmd,
        cl::Name("help"),
        cl::Optional
        );

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
