// This file is distributed under the MIT license.
// See the LICENSE file for details.


#include "Support/CmdLine.h"
#include "Support/PrettyPrint.h"
#include "Support/StringSplit.h"

#include <iostream>
#include <set>

#include <gtest/gtest.h>


using namespace support;


namespace support {
namespace cl {

    template<class T, class U>
    struct Parser<std::pair<T, U>>
    {
        bool operator ()(StringRef value, size_t i, std::pair<T, U>& result) const
        {
            auto p = strings::split(value, ":")();

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


#if 1


typedef std::vector<std::string> Argv;


bool parse(cl::CmdLine& cmd, Argv argv)
{
    if (!cmd.parse(std::move(argv)))
    {
        for (auto const& s : cmd.getErrors())
            std::cout << "NOTE: error : " << s << "\n";
        return false;
    }

    return true;
}


TEST(CmdLineTest, Flags1)
{
    auto test = [](Argv argv, bool val_a, bool val_b, bool val_c) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<bool>(cmd, "a");
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "c", cl::Grouping);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), val_a);
        if (b.getCount()) EXPECT_EQ(b.get(), val_b);
        if (c.getCount()) EXPECT_EQ(c.get(), val_c);
        return true;
    };

    EXPECT_TRUE ( test({ "-a"                   }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a=1"                 }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a=true"              }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a=0"                 }, 0, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a=false"             }, 0, 1, 1 ) );
    EXPECT_FALSE( test({ "-a0"                  }, 1, 1, 1 ) );
    EXPECT_FALSE( test({ "-a1"                  }, 1, 1, 1 ) );
    EXPECT_FALSE( test({ "-ax"                  }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a", "-b"             }, 1, 1, 0 ) );
    EXPECT_TRUE ( test({ "-a", "-b", "-c"       }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a", "-bc"            }, 1, 1, 1 ) );
    EXPECT_TRUE ( test({ "-a", "-cb"            }, 1, 1, 1 ) );
    EXPECT_FALSE( test({ "-a", "-bcbc"          }, 1, 1, 1 ) );
}


TEST(CmdLineTest, Grouping1)
{
    auto test = [](Argv argv, int cnt_a, int cnt_b, int cnt_c) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<bool>(cmd, "a", cl::Grouping, cl::ZeroOrMore);
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "ab", cl::Prefix);

        if (!parse(cmd, std::move(argv)))
            return false;

        EXPECT_EQ(a.getCount(), cnt_a);
        EXPECT_EQ(b.getCount(), cnt_b);
        EXPECT_EQ(c.getCount(), cnt_c);
        return true;
    };

    EXPECT_TRUE ( test({ "-a"                   }, 1, 0, 0 ) );
    EXPECT_FALSE( test({ "-a=1"                 }, 1, 0, 0 ) );
    EXPECT_FALSE( test({ "-a=true"              }, 1, 0, 0 ) );
    EXPECT_FALSE( test({ "-a=0"                 }, 1, 0, 0 ) );
    EXPECT_FALSE( test({ "-a=false"             }, 1, 0, 0 ) );
    EXPECT_FALSE( test({ "-a0"                  }, 0, 0, 0 ) );
    EXPECT_FALSE( test({ "-a1"                  }, 0, 0, 0 ) );
    EXPECT_FALSE( test({ "-ax"                  }, 0, 0, 0 ) );
    EXPECT_FALSE( test({ "-ab"                  }, 0, 0, 0 ) );
    EXPECT_FALSE( test({ "-abb"                 }, 0, 0, 0 ) );
    EXPECT_TRUE ( test({ "-abtrue"              }, 0, 0, 1 ) );
    EXPECT_TRUE ( test({ "-abfalse"             }, 0, 0, 1 ) );
    EXPECT_TRUE ( test({ "-ba"                  }, 1, 1, 0 ) );
    EXPECT_TRUE ( test({ "-baa"                 }, 2, 1, 0 ) );
    EXPECT_TRUE ( test({ "-ba", "-a"            }, 2, 1, 0 ) );
    EXPECT_TRUE ( test({ "-ab", "1", "-ba"      }, 1, 1, 1 ) );
}


TEST(CmdLineTest, Prefix1)
{
    auto test = [](Argv argv, std::string const& a_val) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a", cl::Prefix);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), a_val);
        return true;
    };

    EXPECT_FALSE( test({ "-a"                   }, ""       ) );
    EXPECT_TRUE ( test({ "-a", ""               }, ""       ) );
    EXPECT_TRUE ( test({ "-axxx"                }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-a=xxx"               }, "=xxx"   ) );
    EXPECT_TRUE ( test({ "-a", "xxx"            }, "xxx"    ) );
}


TEST(CmdLineTest, Prefix2)
{
    auto test = [](Argv argv, std::string const& a_val, std::string const& b_val) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a", cl::StrictPrefix);
        auto b = cl::makeOption<std::string>(cmd, "b", cl::StrictPrefix, cl::ArgRequired);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), a_val);
        if (b.getCount()) EXPECT_EQ(b.get(), b_val);
        return true;
    };

    EXPECT_TRUE ( test({ "-a"                   }, ""       , ""        ) );
    EXPECT_TRUE ( test({ "-ax"                  }, "x"      , ""        ) );
    EXPECT_TRUE ( test({ "-a=x"                 }, "=x"     , ""        ) );
    EXPECT_FALSE( test({ "-a", "x"              }, ""       , ""        ) );
    EXPECT_FALSE( test({ "-a", "-b"             }, ""       , ""        ) );
    EXPECT_TRUE ( test({ "-a", "-bx"            }, ""       , "x"       ) );
    EXPECT_TRUE ( test({ "-a", "-b=x"           }, ""       , "=x"      ) );
    EXPECT_FALSE( test({ "-a", "x", "-b=x"      }, ""       , ""        ) );
    EXPECT_TRUE ( test({ "-ax", "-b=x"          }, "x"      , "=x"      ) );
    EXPECT_TRUE ( test({ "-a=x", "-bx"          }, "=x"     , "x"       ) );
}


#endif


#if 0

int main(int argc, char* argv[])
{
    //----------------------------------------------------------------------------------------------

    cl::CmdLine cmd(argv[0], "A short description.\n\nA long description.");

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

    std::initializer_list<std::string> Iinit = {
        "eins", "zwei", "drei", "vier", "funf"
    };

    auto I = cl::makeOption<std::vector<std::string>>(cmd, "I",
        cl::ArgName("dir"),
        cl::Desc("Add the directory dir to the list of directories to be searched for header files."),
        cl::StrictPrefix,
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
        { "O0", OL_None        },
        { "O1", OL_Trivial     },
        { "O2", OL_Default     },
        { "O3", OL_Expensive   }
    });

    auto opt = cl::makeOptionWithParser<OptimizationLevel>(
        optParser,
        cmd,
        cl::ArgDisallowed,
        cl::Desc("Choose an optimization level"),
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

    auto simpson = cl::makeOptionWithParser<Simpson>(
        simpsonParser,
        cmd, "simpson",
        cl::Desc("Choose a Simpson"),
        cl::ArgRequired,
        cl::init(SideshowBob),
        cl::Prefix
        );

                    //------------------------------------------------------------------------------

    auto bf = cl::makeOptionWithParser<unsigned>(
        cl::BinaryOpParser<support::BitOr>(),
        cmd, "bf",
        cl::CommaSeparated,
        cl::ArgRequired,
        cl::ZeroOrMore
        );

                    //------------------------------------------------------------------------------

    auto f = cl::makeOption<std::map<std::string, int>>(
        cmd, "f",
        cl::CommaSeparated,
        cl::ArgRequired
        );

                    //------------------------------------------------------------------------------

    auto helpParser = [&](StringRef /*value*/, size_t /*i*/, bool& /*result*/) -> bool
    {
        cmd.help();
        exit(0);
    };

    auto help = cl::makeOptionWithParser<bool>(
        helpParser,
        cmd, "help",
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
    std::cout << pretty(gh) << std::endl;
    std::cout << pretty(z) << std::endl;
    std::cout << pretty(I) << std::endl;
    std::cout << pretty(opt) << std::endl;
    std::cout << pretty(simpson) << std::endl;
    std::cout << pretty(help) << std::endl;
//    std::cout << pretty(regex) << std::endl;
    std::cout << "bf = 0x" << std::hex << bf.get() << std::endl;
    std::cout << pretty(f) << std::endl;

    std::cout << "files:\n";
    for (auto& s : files.get())
        std::cout << "  \"" << s << "\"" << std::endl;

    //----------------------------------------------------------------------------------------------

    return 0;
}

#endif
