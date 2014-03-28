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

bool parse(cl::CmdLine& cmd, Argv const& argv)
{
    try
    {
        cmd.parse(argv);
        return true;
    }
    catch (std::exception& e)
    {
        std::cout << "ERROR: " << e.what() << std::endl;
        return false;
    }
}

template <class T>
static std::string to_pretty_string(T const& object)
{
    std::ostringstream str;
    str << pretty(object);
    return str.str();
}

TEST(CmdLineTest, Flags1)
{
    using Pair = std::pair<unsigned, int>;

    auto test = [](bool result, Argv const& argv, Pair const& a_val, Pair const& b_val, Pair const& c_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<bool>(cmd, "a");
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "c", cl::Grouping, cl::ZeroOrMore);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(a_val.first, a->count());
        EXPECT_EQ(b_val.first, b->count());
        EXPECT_EQ(c_val.first, c->count());

        if (a->count())
            EXPECT_EQ(a_val.second, +a->value());
        if (b->count())
            EXPECT_EQ(b_val.second, +b->value());
        if (c->count())
            EXPECT_EQ(c_val.second, +c->value());
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

    auto test = [](bool result, Argv const& argv, Pair const& a_val, Pair const& b_val, Pair const& c_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<bool>(cmd, "a", cl::Grouping, cl::ZeroOrMore);
        auto b = cl::makeOption<bool>(cmd, "b", cl::Grouping);
        auto c = cl::makeOption<bool>(cmd, "ab", cl::Prefix);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(a_val.first, a->count());
        EXPECT_EQ(b_val.first, b->count());
        EXPECT_EQ(c_val.first, c->count());

        if (a->count())
            EXPECT_EQ(a_val.second, +a->value());
        if (b->count())
            EXPECT_EQ(b_val.second, +b->value());
        if (c->count())
            EXPECT_EQ(c_val.second, +c->value());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a"                   }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=1"                 }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=true"              }, {1,1}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=0"                 }, {1,0}, {0,0}, {0,0} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-a=false"             }, {1,0}, {0,0}, {0,0} ) );
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
    EXPECT_NO_FATAL_FAILURE( test(false, { "--ba", "-a"           }, {0,0}, {0,0}, {0,0} ) ); // no check for option group
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ab", "-ba"           }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-ab1", "-ba"          }, {1,1}, {1,1}, {1,1} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ab=1", "-ba"         }, {0,0}, {0,0}, {0,0} ) ); // invalid value for -ab
    EXPECT_NO_FATAL_FAILURE( test(false, { "-ab", "1", "-ba"      }, {0,0}, {0,0}, {1,1} ) ); // unhandled positional
}

TEST(CmdLineTest, Grouping2)
{
    using PairI = std::pair<unsigned, int>;
    using PairS = std::pair<unsigned, std::string>;

    auto test = [](bool result, Argv const& argv, PairI const& a_val, PairI const& b_val, PairS const& c_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<bool>(cmd, "x", cl::Grouping, cl::ArgDisallowed, cl::ZeroOrMore);
        auto b = cl::makeOption<bool>(cmd, "v", cl::Grouping, cl::ArgDisallowed);
        auto c = cl::makeOption<std::string>(cmd, "f", cl::Grouping, cl::ArgRequired);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(a_val.first, a->count());
        EXPECT_EQ(b_val.first, b->count());
        EXPECT_EQ(c_val.first, c->count());

        if (a->count())
            EXPECT_EQ(a_val.second, +a->value());
        if (b->count())
            EXPECT_EQ(b_val.second, +b->value());
        if (c->count())
            EXPECT_EQ(c_val.second, c->value());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  { "-xvf", "test.tar"       }, {1,1}, {1,1}, {1,"test.tar"} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-xv", "-f", "test.tar"  }, {1,1}, {1,1}, {1,"test.tar"} ) );
    EXPECT_NO_FATAL_FAILURE( test(true,  { "-xv", "-f=test.tar"     }, {1,1}, {1,1}, {1,"test.tar"} ) );
    EXPECT_NO_FATAL_FAILURE( test(false, { "-xfv", "test.tar"       }, {0,0}, {0,0}, {0,""        } ) );
}

TEST(CmdLineTest, Prefix)
{
    using Pair = std::pair<unsigned, std::string>;

    auto test = [](bool result, Argv const& argv, Pair const& r_val, Pair const& o_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto r = cl::makeOption<std::string>(cmd, "r", cl::Prefix, cl::ArgRequired);
        auto o = cl::makeOption<std::string>(cmd, "o", cl::Prefix, cl::ArgOptional);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(r_val.first, r->count());
        EXPECT_EQ(o_val.first, o->count());

        if (r->count())
            EXPECT_EQ(r_val.second, r->value());
        if (o->count())
            EXPECT_EQ(o_val.second, o->value());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  {              }, {0,""        }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r"          }, {0,""        }, {0,""        }) ); // missing argument for r
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "x"     }, {0,""        }, {0,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-rx"         }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r=x"        }, {1,"=x"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r-o"        }, {1,"-o"      }, {0,""        }) );
//  EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-o"    }, {0,""        }, {1,""        }) ); // -o is a valid option
//  EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-ox"   }, {0,""        }, {1,"x"       }) ); // -o is a valid option
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o"          }, {0,""        }, {1,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-o", "x"     }, {0,""        }, {1,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-ox"         }, {0,""        }, {1,"x"       }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o=x"        }, {0,""        }, {1,"=x"      }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o-r"        }, {0,""        }, {1,"-r"      }) );
}

TEST(CmdLineTest, MayPrefix)
{
    using Pair = std::pair<unsigned, std::string>;

    auto test = [](bool result, Argv const& argv, Pair const& r_val, Pair const& o_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto r = cl::makeOption<std::string>(cmd, "r", cl::MayPrefix, cl::ArgRequired);
        auto o = cl::makeOption<std::string>(cmd, "o", cl::MayPrefix, cl::ArgOptional);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(r_val.first, r->count());
        EXPECT_EQ(o_val.first, o->count());

        if (r->count())
            EXPECT_EQ(r_val.second, r->value());
        if (o->count())
            EXPECT_EQ(o_val.second, o->value());
    };

    EXPECT_NO_FATAL_FAILURE( test(true,  {              }, {0,""        }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-r"          }, {0,""        }, {0,""        }) ); // missing argument for r
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r", "x"     }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-rx"         }, {1,"x"       }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r=x"        }, {1,"=x"      }, {0,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r-o"        }, {1,"-o"      }, {0,""        }) );
//  EXPECT_NO_FATAL_FAILURE( test(false, {"-r", "-o"    }, {0,""        }, {1,""        }) ); // -o is a valid option
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-r", "-ox"   }, {1,"-ox"     }, {0,""        }) ); // -ox is *NOT* a valid option (quick test)
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o"          }, {0,""        }, {1,""        }) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-o", "x"     }, {0,""        }, {1,""        }) ); // unhandled positional arg
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-ox"         }, {0,""        }, {1,"x"       }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o=x"        }, {0,""        }, {1,"=x"      }) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-o-r"        }, {0,""        }, {1,"-r"      }) );
}

TEST(CmdLineTest, Equals)
{
    auto test = [](Argv const& argv, std::string const& val) -> bool
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<std::string>(cmd, "a", cl::Prefix, cl::ArgRequired);
        auto b = cl::makeOption<std::string>(cmd, "b", cl::Prefix, cl::ArgOptional);
        auto c = cl::makeOption<std::string>(cmd, "c", cl::ArgRequired);
        auto d = cl::makeOption<std::string>(cmd, "d", cl::ArgOptional);

        if (!parse(cmd, argv))
            return false;

        if (a->count()) EXPECT_EQ(a->value(), val);
        if (b->count()) EXPECT_EQ(b->value(), val);
        if (c->count()) EXPECT_EQ(c->value(), val);
        if (d->count()) EXPECT_EQ(d->value(), val);

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
    auto test = [](Argv const& argv, std::string const& s_val, std::vector<std::string> const& x_val) -> bool
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<std::string>("a");
        cmd.add(a);
        auto s = cl::makeOption<std::string>("script", cl::Positional, cl::Required, cl::ConsumeAfter);
        cmd.add(s);
        auto x = cl::makeOption<std::vector<std::string>>("arguments", cl::Positional);
        cmd.add(x);

        if (!parse(cmd, argv))
            return false;

        if (s->count())
            EXPECT_EQ(s->value(), s_val);

        if (x->count())
            EXPECT_EQ(x->value(), x_val);
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

    auto test = [](Argv const& argv, std::vector<std::string> const& s_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto a = cl::makeOption<std::string>("a");
        cmd.add(a);
        auto s = cl::makeOption<std::vector<std::string>>("script", cl::Positional, cl::OneOrMore, cl::ConsumeAfter);
        cmd.add(s);

        if (!parse(cmd, argv))
            return false;

        if (s->count())
            EXPECT_EQ(s->value(), s_val);

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
    auto test = [](bool result, Argv const& argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto xParser = cl::MapParser<int>({
            { "none", 0 },
            { "c",    1 },
            { "c++",  2 },
            { "01234567890123456789012345678901234567890123456789012345678901234567890123456789012345678901234567890123456789", 3}
        });

        auto x = cl::makeOptionWithParser<int>(
            xParser,
            cmd, "x",
            cl::ArgRequired,
            cl::ArgName("lang"),
            cl::ZeroOrMore,
            cl::init(0)
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x->count());

        if (x->count())
            EXPECT_EQ(x_val.second, x->value());
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
    auto test = [](bool result, Argv const& argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto xParser = cl::MapParser<int>({
            { "O0", 0 },
            { "O1", 1 },
            { "O2", 2 },
            { "O3", 3 },
        });

        auto x = cl::makeOptionWithParser<int>(
            xParser,
            cl::Required,
            cl::ArgDisallowed
            );
        cmd.add(x);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x->count());

        if (x->count())
            EXPECT_EQ(x_val.second, x->value());
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
    auto test = [](bool result, Argv const& argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto xParser = cl::MapParser<int>({
            { "O0", 0 },
            { "O1", 1 },
            { "O2", 2 },
            { "O3", 3 },
        });

        auto x = cl::makeOptionWithParser<int>(
            xParser,
            cl::Required,
            cl::Prefix,
            cl::ArgOptional
            );
        cmd.add(x);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x->count());

        if (x->count())
            EXPECT_EQ(x_val.second, x->value());
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
    auto test = [](bool result, Argv const& argv, std::pair<unsigned, int> x_val)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto xParser = cl::MapParser<int>({
            { "0", 0 },
            { "1", 1 },
            { "2", 2 },
            { "3", 3 },
        });

        auto x = cl::makeOptionWithParser<int>(
            xParser,
            cmd, "O",
            cl::Required,
            cl::Prefix,
            cl::ArgRequired
            );

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);

        EXPECT_EQ(x_val.first, x->count());

        if (x->count())
            EXPECT_EQ(x_val.second, x->value());
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

TEST(CmdLineTest, OptionGroup1)
{
    auto test = [](bool result, Argv const& argv)
    {
        SCOPED_TRACE("parsing: " + to_pretty_string(argv));

        cl::CmdLine cmd;

        auto gr1 = cl::OptionGroup("gr1");
        cmd.add(gr1);
        auto gr2 = cl::OptionGroup("gr2", cl::OptionGroup::ZeroOrAll);
        cmd.add(gr2);
        auto gr3 = cl::OptionGroup("gr3", cl::OptionGroup::One);
        cmd.add(gr3);

        auto x = cl::makeOption<bool>("x");
        cmd.add(x);
        gr1.add(x);
        auto y = cl::makeOption<bool>("y");
        cmd.add(y);
        gr2.add(y);
        auto z = cl::makeOption<bool>("z");
        cmd.add(z);
        gr2.add(z);
        gr3.add(z);

        bool actual_result = parse(cmd, argv);
        EXPECT_EQ(result, actual_result);
    };

    EXPECT_NO_FATAL_FAILURE( test(false, {}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-z"}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-y", "-z"}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x"}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x", "-z"}) );
    EXPECT_NO_FATAL_FAILURE( test(false, {"-x", "-y"}) );
    EXPECT_NO_FATAL_FAILURE( test(true,  {"-x", "-y", "-z"}) );
}

TEST(CmdLineTest, MapRef)
{
    int opt = 0;

    cl::CmdLine cmd;

    auto x = cl::makeOption<int&>({{"0",0}, {"1",1}, {"2",2}}, cl::init(opt), "opt");
    cmd.add(x);

    bool ok = parse(cmd, {"-opt=1"});

    EXPECT_EQ(true, ok);
    EXPECT_EQ(1, opt);
}
