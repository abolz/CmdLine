// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLineToArgv.h"

#include "ConvertUTF.h"
#include "PrettyPrint.h"

#include <vector>
#include <iostream>

#include <gtest/gtest.h>
#ifdef _WIN32
#include <windows.h>
#endif

using namespace support;

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
#ifdef _WIN32

static std::string toUTF8(std::wstring const& str)
{
    std::string result;

    auto I = str.begin();
    auto E = str.end();

    utf::convertUTF16ToUTF8(I, E, std::back_inserter(result));

    return result;
}

static std::wstring toUTF16(std::string const& str)
{
    std::wstring result;

    auto I = str.begin();
    auto E = str.end();

    utf::convertUTF8ToUTF16(I, E, std::back_inserter(result));

    return result;
}

static std::vector<std::string> stringToArgvWin(std::wstring const& wargs)
{
    int argc = 0;
    auto argv = ::CommandLineToArgvW(wargs.c_str(), &argc);

    std::vector<std::string> result;

    for (int i = 0; i < argc; ++i)
    {
        result.push_back(toUTF8(argv[i]));
    }

    LocalFree(argv);

    return result;
}

static std::vector<std::string> stringToArgvCL(std::string const& args)
{
    std::vector<std::string> argv;

    cl::tokenizeCommandLineWin(args.begin(), args.end(), std::back_inserter(argv));

    return argv;
}

static void compare(std::string const& args)
{
    SCOPED_TRACE("command-line: \"( " + args + " )\"");

    auto x = stringToArgvWin(toUTF16(args));
    auto y = stringToArgvCL(args);

    EXPECT_EQ(x, y);
}

TEST(CmdLineToArgvTest, Win)
{
//  #define P R"("c:\path to\test program.exe" )"
//  #define P R"("c:\path to\test program.exe )"
    #define P R"(test )"

    const std::string tests[] = {
        P R"()",
        P R"( )",
        P R"( ")",
        P R"( " )",
        P R"(foo""""""""""""bar)",
        P R"(foo"X""X""X""X"bar)",
        P R"(   "this is a string")",
        P R"("  "this is a string")",
        P R"( " "this is a string")",
        P R"("this is a string")",
        P R"("this is a string)",
        P R"("""this" is a string)",
        P R"("hello\"there")",
        P R"("hello\\")",
        P R"(abc)",
        P R"("a b c")",
        P R"(a"b c d"e)",
        P R"(a\"b c)",
        P R"("a\"b c")",
        P R"("a b c\\")",
        P R"("a\\\"b")",
        P R"(a\\\b)",
        P R"("a\\\b")",
        P R"("\"a b c\"")",
        P R"("a b c" d e)",
        P R"("ab\"c" "\\" d)",
        P R"(a\\\b d"e f"g h)",
        P R"(a\\\"b c d)",
        P R"(a\\\\"b c" d e)",
        P R"("a b c""   )",
        P R"("""a""" b c)",
        P R"("""a b c""")",
        P R"(""""a b c"" d e)",
        P R"("c:\file x.txt")",
        P R"("c:\dir x\\")",
        P R"("\"c:\dir x\\\"")",
        P R"("a b c")",
        P R"("a b c"")",
        P R"("a b c""")",
    };

    for (auto const& s : tests)
    {
        EXPECT_NO_FATAL_FAILURE( compare(s) );
    }
}

#endif

//--------------------------------------------------------------------------------------------------
//
//--------------------------------------------------------------------------------------------------
TEST(CmdLineToArgvTest, Unix)
{
    // FIXME:
    // Add some tests here...
}
