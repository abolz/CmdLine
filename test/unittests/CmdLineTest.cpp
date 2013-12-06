// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLine.h"
#include "Support/StringSplit.h"

#include "PrettyPrint.h"

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
        //for (auto const& s : cmd.getErrors())
        //    std::cout << "NOTE : " << s << "\n";
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
    using Pair = std::pair<unsigned, int>;

    auto test = [](bool result, Argv argv, Pair const& a_val, Pair const& b_val, Pair const& c_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto a = cl::makeOption<bool>(cmd, "a");
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "c", cl::Grouping, cl::ZeroOrMore);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(a_val.first, a.getCount());
        EXPECT_EQ(b_val.first, b.getCount());
        EXPECT_EQ(c_val.first, c.getCount());

        if (a.getCount())
            EXPECT_EQ(a_val.second, +a.get());
        if (b.getCount())
            EXPECT_EQ(b_val.second, +b.get());
        if (c.getCount())
            EXPECT_EQ(c_val.second, +c.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a"                   }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=1"                 }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=true"              }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=0"                 }, {1,0}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=false"             }, {1,0}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a0"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -a0
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a1"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -a1
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ax"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -ax
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a", "-b"             }, {1,1}, {1,1}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a", "-b", "-c"       }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a", "-bc"            }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a", "--bc"           }, {1,1}, {0,0}, {0,0} ) ); // unknown option --bc
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a", "-cb"            }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a", "-bcb"           }, {1,1}, {1,1}, {1,1} ) ); // -b only allowed once
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a", "-bcc"           }, {1,1}, {1,1}, {2,1} ) ); // -b only allowed once
}

TEST(CmdLineTest, Grouping1)
{
    using Pair = std::pair<unsigned, int>;

    auto test = [](bool result, Argv argv, Pair const& a_val, Pair const& b_val, Pair const& c_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto a = cl::makeOption<bool>(cmd, "a", cl::Grouping, cl::ZeroOrMore);
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "ab", cl::Prefix);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(a_val.first, a.getCount());
        EXPECT_EQ(b_val.first, b.getCount());
        EXPECT_EQ(c_val.first, c.getCount());

        if (a.getCount())
            EXPECT_EQ(a_val.second, +a.get());
        if (b.getCount())
            EXPECT_EQ(b_val.second, +b.get());
        if (c.getCount())
            EXPECT_EQ(c_val.second, +c.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a"                   }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a=1"                 }, {0,0}, {0,0}, {0,0} ) ); // group => arg disallowed
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a=true"              }, {0,0}, {0,0}, {0,0} ) ); // group => arg disallowed
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a=0"                 }, {0,0}, {0,0}, {0,0} ) ); // group => arg disallowed
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a=false"             }, {0,0}, {0,0}, {0,0} ) ); // group => arg disallowed
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a0"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -a0
    EXPECT_NO_FATAL_FAILURE( test(false, { "-a1"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -a1
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ax"                  }, {0,0}, {0,0}, {0,0} ) ); // unknown option -ax
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ab"                  }, {0,0}, {0,0}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-abb"                 }, {0,0}, {0,0}, {0,0} ) ); // invalid value for -ab
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-abtrue"              }, {0,0}, {0,0}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-abfalse"             }, {0,0}, {0,0}, {1,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ba"                  }, {1,1}, {1,1}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "--ba"                 }, {0,0}, {0,0}, {0,0} ) ); // no check for option group
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-baa"                 }, {2,1}, {1,1}, {0,0} ) ); // check for option group
    EXPECT_NO_FATAL_FAILURE( test(false, { "--baa"                }, {0,0}, {0,0}, {0,0} ) ); // no check for option group
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ba", "-a"            }, {2,1}, {1,1}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "--ba", "-a"           }, {1,1}, {0,0}, {0,0} ) ); // no check for option group
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ab", "-ba"           }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ab1", "-ba"          }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ab=1", "-ba"         }, {1,1}, {1,1}, {0,0} ) ); // invalid value for -ab
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ab", "1", "-ba"      }, {1,1}, {1,1}, {1,1} ) ); // unhandled positional
}

TEST(CmdLineTest, Prefix)
{
    using Pair = std::pair<unsigned, std::string>;

    auto test = [](bool result, Argv argv, Pair const& r_val, Pair const& o_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto r = cl::makeOption<std::string>(cmd, "r", cl::Prefix, cl::ArgRequired);
        auto o = cl::makeOption<std::string>(cmd, "o", cl::Prefix, cl::ArgOptional);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(r_val.first, r.getCount());
        EXPECT_EQ(o_val.first, o.getCount());

        if (r.getCount())
            EXPECT_EQ(r_val.second, r.get());
        if (o.getCount())
            EXPECT_EQ(o_val.second, o.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  {              }, {0,""        }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r"          }, {0,""        }, {0,""        }) ); // missing argument for r
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "x"     }, {0,""        }, {0,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-rx"         }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r=x"        }, {1,"=x"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r-o"        }, {1,"-o"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-o"    }, {0,""        }, {1,""        }) ); // -o is a valid option
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-ox"   }, {0,""        }, {1,"x"       }) ); // -o is a valid option
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o"          }, {0,""        }, {1,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-o", "x"     }, {0,""        }, {1,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-ox"         }, {0,""        }, {1,"x"       }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o=x"        }, {0,""        }, {1,"=x"      }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o-r"        }, {0,""        }, {1,"-r"      }) );
}

TEST(CmdLineTest, MayPrefix)
{
    using Pair = std::pair<unsigned, std::string>;

    auto test = [](bool result, Argv argv, Pair const& r_val, Pair const& o_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto r = cl::makeOption<std::string>(cmd, "r", cl::MayPrefix, cl::ArgRequired);
        auto o = cl::makeOption<std::string>(cmd, "o", cl::MayPrefix, cl::ArgOptional);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(r_val.first, r.getCount());
        EXPECT_EQ(o_val.first, o.getCount());

        if (r.getCount())
            EXPECT_EQ(r_val.second, r.get());
        if (o.getCount())
            EXPECT_EQ(o_val.second, o.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  {              }, {0,""        }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r"          }, {0,""        }, {0,""        }) ); // missing argument for r
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r", "x"     }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-rx"         }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r=x"        }, {1,"=x"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r-o"        }, {1,"-o"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-o"    }, {0,""        }, {1,""        }) ); // -o is a valid option
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r", "-ox"   }, {1,"-ox"     }, {0,""        }) ); // -ox is *NOT* a valid option (quick test)
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o"          }, {0,""        }, {1,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-o", "x"     }, {0,""        }, {1,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-ox"         }, {0,""        }, {1,"x"       }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o=x"        }, {0,""        }, {1,"=x"      }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o-r"        }, {0,""        }, {1,"-r"      }) );
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
    EXPECT_TRUE ( test({ "-b=xxx"               }, "=xxx"   ) );
    EXPECT_FALSE( test({ "-c"                   }, ""       ) ); // -c expects an argument
    EXPECT_TRUE ( test({ "-c", "xxx"            }, "xxx"    ) );
    EXPECT_FALSE( test({ "-cxxx"                }, ""       ) ); // unknown option -cxxx
    EXPECT_TRUE ( test({ "-c=xxx"               }, "xxx"    ) );
    EXPECT_TRUE ( test({ "-d"                   }, ""       ) );
    EXPECT_FALSE( test({ "-d", "xxx"            }, "xxx"    ) ); // unhandled positional xxx
    EXPECT_FALSE( test({ "-dxxx"                }, ""       ) ); // unknown option -dxxx
    EXPECT_TRUE ( test({ "-d=xxx"               }, "xxx"    ) );
}

TEST(CmdLineTest, Consume1)
{
    auto test = [](Argv argv, std::string const& s_val, std::vector<std::string> const& x_val) -> bool
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a");
        auto s = cl::makeOption<std::string>(cmd, "script", cl::Positional, cl::Required, cl::ConsumeAfter);
        auto x = cl::makeOption<std::vector<std::string>>(cmd, "arguments", cl::Positional);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (s.getCount())
            EXPECT_EQ(s.get(), s_val);

        if (x.getCount())
            EXPECT_EQ(x.get(), x_val);
        else
            EXPECT_TRUE(x_val.empty());

        return true;
    };

    EXPECT_FALSE( test( { "-a"                      }, "script",    {           } ) ); // script name missing
    EXPECT_TRUE ( test( { "script"                  }, "script",    {           } ) );
    EXPECT_TRUE ( test( { "script", "x"             }, "script",    {"x"        } ) );
    EXPECT_TRUE ( test( { "x", "script"             }, "x",         {"script"   } ) );
    EXPECT_TRUE ( test( { "script", "-a"            }, "script",    {"-a"       } ) );
    EXPECT_TRUE ( test( { "-a", "script"            }, "script",    {           } ) ); // -a is an argument for <program>
    EXPECT_TRUE ( test( { "-a", "script", "-a"      }, "script",    {"-a"       } ) ); // the second -a does not match the "consume-option"
    EXPECT_TRUE ( test( { "-a", "script", "x", "-a" }, "script",    {"x", "-a"  } ) ); // the first -a is an argument for <program>
    EXPECT_TRUE ( test( { "script", "-a", "x"       }, "script",    {"-a", "x"  } ) );
    EXPECT_TRUE ( test( { "script", "x", "-a"       }, "script",    {"x", "-a"  } ) ); // -a is an argument for <s>
}

TEST(CmdLineTest, Consume2)
{
    // same as Consume1, but
    // merge script name and arguments...

    auto test = [](Argv argv, std::vector<std::string> const& s_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto a = cl::makeOption<std::string>(cmd, "a");
        auto s = cl::makeOption<std::vector<std::string>>(cmd, "script", cl::Positional, cl::OneOrMore, cl::ConsumeAfter);

        if (!parse(cmd, std::move(argv)))
            return false;

        if (s.getCount())
            EXPECT_EQ(s.get(), s_val);

        return true;
    };

    EXPECT_FALSE( test( { "-a"                      }, {                        } ) ); // script name missing
    EXPECT_TRUE ( test( { "script"                  }, {"script",               } ) );
    EXPECT_TRUE ( test( { "script", "x"             }, {"script", "x"           } ) );
    EXPECT_TRUE ( test( { "x", "script"             }, {"x",      "script"      } ) );
    EXPECT_TRUE ( test( { "script", "-a"            }, {"script", "-a"          } ) );
    EXPECT_TRUE ( test( { "-a", "script"            }, {"script",               } ) ); // -a is an argument for <program>
    EXPECT_TRUE ( test( { "-a", "script", "-a"      }, {"script", "-a"          } ) ); // the second -a does not match the "consume-option"
    EXPECT_TRUE ( test( { "-a", "script", "x", "-a" }, {"script", "x", "-a"     } ) ); // the first -a is an argument for <program>
    EXPECT_TRUE ( test( { "script", "-a", "x"       }, {"script", "-a", "x"     } ) );
    EXPECT_TRUE ( test( { "script", "x", "-a"       }, {"script", "x", "-a"     } ) ); // -a is an argument for <s>
}

TEST(CmdLineTest, Map1)
{
    auto test = [](bool result, Argv argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto x = cl::makeOption<int>(
            {   { "none",          0, "Guess source file type" },
                { "c",             1, "C source file" },
                { "c++",           2, "C++ source file" },
            },
            cmd, "x",
            cl::ArgRequired,
            cl::ArgName("lang"),
            cl::Desc("Specifiy source file type"),
            cl::ZeroOrMore,
            cl::init(0)
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x.getCount());

        if (x.getCount())
            EXPECT_EQ(x_val.second, x.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  {                  }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x"              }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-x", "none"      }, {1,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-x=none"         }, {1,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-x", "c++"       }, {1,2}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-x=c++"          }, {1,2}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x", "cxx"       }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x=cxx"          }, {0,0}) );
}

TEST(CmdLineTest, Map2)
{
    auto test = [](bool result, Argv argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto x = cl::makeOption<int>(
            {   { "O0", 0, "No optimizations"             },
                { "O1", 1, "Enable trivial optimizations" },
                { "O2", 2, "Enable some optimizations"    },
                { "O3", 3, "Enable all optimizations"     }
            },
            cmd,
            cl::Required,
            cl::ArgDisallowed,
            cl::Desc("Choose an optimization level")
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x.getCount());

        if (x.getCount())
            EXPECT_EQ(x_val.second, x.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(false, {                  }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O"              }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-O1"             }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-Ox"             }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O=1"            }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O", "1"         }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O1", "-O1"      }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O2", "-O1"      }, {1,2}) );
}

TEST(CmdLineTest, Map3)
{
    auto test = [](bool result, Argv argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto x = cl::makeOption<int>(
            {   { "O0", 0, "No optimizations"             },
                { "O1", 1, "Enable trivial optimizations" },
                { "O2", 2, "Enable some optimizations"    },
                { "O3", 3, "Enable all optimizations"     }
            },
            cmd,
            cl::Required,
            cl::Prefix,
            cl::ArgOptional,
            cl::Desc("Choose an optimization level")
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x.getCount());

        if (x.getCount())
            EXPECT_EQ(x_val.second, x.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(false, {                  }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O"              }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-O1"             }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O1=O1"          }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-O1O1"           }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-O1O2"           }, {1,2}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O1Ox"           }, {0,0}) );
}

TEST(CmdLineTest, Map4)
{
    auto test = [](bool result, Argv argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd("program");

        auto x = cl::makeOption<int>(
            {   { "0", 0, "No optimizations"             },
                { "1", 1, "Enable trivial optimizations" },
                { "2", 2, "Enable some optimizations"    },
                { "3", 3, "Enable all optimizations"     }
            },
            cmd, "O",
            cl::Required,
            cl::Prefix,
            cl::ArgRequired,
            cl::Desc("Choose an optimization level")
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x.getCount());

        if (x.getCount())
            EXPECT_EQ(x_val.second, x.get());
    };

    EXPECT_NO_FATAL_FAILURE( test(false, {                  }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O"              }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-O1"             }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-Ox"             }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O=1"            }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O", "1"         }, {0,0}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O1", "-O1"      }, {1,1}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-O2", "-O1"      }, {1,2}) );
}
