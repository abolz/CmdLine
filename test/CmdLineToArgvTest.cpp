// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/CmdLineToArgv.h"
#include "Support/PrettyPrint.h"

#include "ConvertUTF.h"

#include <vector>
#include <iostream>

#include <windows.h>

using namespace support;

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

static void compare(std::string const& args, std::wstring const& wargs)
{
    auto argvWin = stringToArgvWin(wargs);
    auto argvCL = stringToArgvCL(args);

    if (argvWin != argvCL)
    {
        std::cout << "FAILED" << std::endl;
        std::cout << "    command-line : " << pretty(args) << std::endl;
        std::cout << "    actual       : " << pretty(argvCL) << std::endl;
        std::cout << "    expected     : " << pretty(argvWin) << std::endl;
    }
}

int main()
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

    for (auto&& s : tests)
        compare( s, toUTF16(s) );

    return 0;
}
