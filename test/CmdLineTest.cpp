// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/PrettyPrint.h"
#include "Support/StringSplit.h"

#include <iostream>
#include <set>
#include <sstream>

#include <gtest/gtest.h>

using namespace support;

typedef std::vector<std::string> Argv;

bool parse(cl::CmdLine& cmd, Argv argv)
{
    if (!cmd.parse(std::move(argv)))
    {
        for (auto const& s : cmd.getErrors())
            std::cout << "NOTE : " << s << "\n";
        return false;
    }

    return true;
}

template <class T>
static std::string to_pretty_string(T const& object)
{
    std::ostringstream str;
    str << pretty(object);
    return str.str();
}

TEST(CmdLineTest, ArgOptionalPass1)
{
    cl::CmdLine cmd("program");
    auto a = cl::makeOption<std::string>(cmd, "a", cl::ArgOptional);

    EXPECT_TRUE( cmd.parse({"-a"}) );
    EXPECT_EQ( a.getCount(), 1 );
    EXPECT_EQ( a.get(), "" );
}

TEST(CmdLineTest, ArgOptionalPass2)
{
    cl::CmdLine cmd("program");
    auto a = cl::makeOption<std::string>(cmd, "a", cl::ArgOptional);

    EXPECT_TRUE( cmd.parse({"-a=xxx"}) );
    EXPECT_EQ( a.getCount(), 1 );
    EXPECT_EQ( a.get(), "xxx" );
}

TEST(CmdLineTest, ArgOptionalFail1)
{
    cl::CmdLine cmd("program");
    auto a = cl::makeOption<std::string>(cmd, "a", cl::ArgOptional);

    EXPECT_FALSE( cmd.parse({"-a", "xxx"}) );
    EXPECT_EQ( a.getCount(), 1 );
    EXPECT_EQ( a.get(), "" );

    auto e = cmd.getErrors();

    ASSERT_EQ(e.size(), 1);
    EXPECT_EQ(e[0], "unhandled positional argument: 'xxx'");
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
    EXPECT_TRUE ( test({ "-ab"                  }, 0, 0, 1 ) );
    EXPECT_FALSE( test({ "-abb"                 }, 0, 0, 0 ) );
    EXPECT_TRUE ( test({ "-abtrue"              }, 0, 0, 1 ) );
    EXPECT_TRUE ( test({ "-abfalse"             }, 0, 0, 1 ) );
    EXPECT_TRUE ( test({ "-ba"                  }, 1, 1, 0 ) );
    EXPECT_TRUE ( test({ "-baa"                 }, 2, 1, 0 ) );
    EXPECT_TRUE ( test({ "-ba", "-a"            }, 2, 1, 0 ) );
    EXPECT_TRUE ( test({ "-ab=1", "-ba"         }, 1, 1, 1 ) );
    EXPECT_FALSE( test({ "-ab", "1", "-ba"      }, 1, 1, 1 ) );
}

TEST(CmdLineTest, PrefixOptional)
{
    auto test = [](Argv argv, std::string const& a_val) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a", cl::Prefix, cl::ArgOptional);
        auto b = cl::makeOption<bool>(cmd, "b");

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), a_val);
        return true;
    };

    EXPECT_TRUE ( test({ "-a"                   }, ""       ) );
    EXPECT_TRUE ( test({ "-a", "-b"             }, ""       ) ); // "-b" is a valid option -> is not a valid argument for -a
    EXPECT_TRUE ( test({ "-a-b"                 }, "-b"     ) );
    EXPECT_TRUE ( test({ "-a=-b"                }, "-b"     ) );
    EXPECT_TRUE ( test({ "-axxx"                }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-a=xxx"               }, "xxx"    ) );
    EXPECT_FALSE( test({ "-a", "xxx"            }, ""       ) ); // [unhandled positional]
}

TEST(CmdLineTest, PrefixRequired)
{
    auto test = [](Argv argv, std::string const& a_val) -> bool
    {
        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a", cl::Prefix, cl::ArgRequired);
        auto b = cl::makeOption<bool>(cmd, "b");

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), a_val);
        return true;
    };

    EXPECT_FALSE( test({ "-a"                   }, ""       ) );
    EXPECT_FALSE( test({ "-a", "-b"             }, ""       ) ); // "-b" is a valid option -> is not a valid argument for -a
    EXPECT_TRUE ( test({ "-a-b"                 }, "-b"     ) );
    EXPECT_TRUE ( test({ "-a=-b"                }, "=-b"    ) );
    EXPECT_TRUE ( test({ "-axxx"                }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-a=xxx"               }, "=xxx"   ) );
    EXPECT_FALSE( test({ "-a", "xxx"            }, "xxx"    ) ); // [option 'a' expects and argument, unhandled positional]
}

TEST(CmdLineTest, Equals)
{
    auto test = [](Argv argv, std::string const& val) -> bool
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a", cl::Prefix, cl::ArgRequired);
        auto b = cl::makeOption<std::string>(cmd, "b", cl::Prefix, cl::ArgOptional);
        auto c = cl::makeOption<std::string>(cmd, "c", cl::ArgRequired);
        auto d = cl::makeOption<std::string>(cmd, "d", cl::ArgOptional);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (a.getCount()) EXPECT_EQ(a.get(), val);
        if (b.getCount()) EXPECT_EQ(b.get(), val);
        if (c.getCount()) EXPECT_EQ(c.get(), val);
        if (d.getCount()) EXPECT_EQ(d.get(), val);

        return true;
    };

    EXPECT_FALSE( test({ "-a"                   }, ""       ) ); // -a expects an argument
    EXPECT_FALSE( test({ "-a", "xxx"            }, ""       ) ); // -a expects an argument
    EXPECT_TRUE ( test({ "-axxx"                }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-a=xxx"               }, "=xxx"   ) );
    EXPECT_TRUE ( test({ "-b"                   }, ""       ) );
    EXPECT_FALSE( test({ "-b", "xxx"            }, ""       ) ); // unhandled positional xxx
    EXPECT_TRUE ( test({ "-bxxx"                }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-b=xxx"               }, "xxx"    ) );
    EXPECT_FALSE( test({ "-c"                   }, ""       ) ); // -c expects an argument
    EXPECT_TRUE ( test({ "-c", "xxx"            }, "xxx"    ) );
    EXPECT_FALSE( test({ "-cxxx"                }, ""       ) ); // unknown option -cxxx
    EXPECT_TRUE ( test({ "-c=xxx"               }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-d"                   }, ""       ) );
    EXPECT_FALSE( test({ "-d", "xxx"            }, "xxx"    ) ); // unhandled positional xxx
    EXPECT_FALSE( test({ "-dxxx"                }, ""       ) ); // unknown option -dxxx
    EXPECT_TRUE ( test({ "-d=xxx"               }, "xxx"    ) );
}
