solution "Support"

    configurations { "Debug", "Release" }

    platforms { "x64", "x32" }

    location    ("build/" .. _ACTION)
    objdir      ("build/" .. _ACTION .. "/obj")
    targetdir   ("build/" .. _ACTION .. "/bin")

    configuration { "Debug" }

        defines { "_DEBUG" }
        flags { "ExtraWarnings", "Symbols" }

    configuration { "Release" }

        defines { "NDEBUG" }
        flags { "ExtraWarnings", "Optimize" }

    configuration { "gmake" }

        buildoptions {
            "-std=c++11",
            "-Wall",
            "-Wextra",
            "-pedantic",
--            "-save-temps",
        }

    configuration { "windows" }

        flags { "Unicode" }


----------------------------------------------------------------------------------------------------
project "CmdLine"

    kind "StaticLib"

    language "C++"

    includedirs { "include/" }

    files {
        "include/**.*",
        "src/**.*",
    }

    configuration { "Debug" }
        targetsuffix "d"


----------------------------------------------------------------------------------------------------
project "Test"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine" }

    includedirs { "include/" }

    files {
        "test/Test.cpp",
        "test/CmdLineQt.h",
    }


----------------------------------------------------------------------------------------------------
project "CmdLineTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    files {
        "test/CmdLineTest.cpp",
    }


----------------------------------------------------------------------------------------------------
project "CmdLineToArgvTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine" }

    includedirs { "include/" }

    files {
        "test/CmdLineToArgvTest.cpp",
        "test/ConvertUTF.h",
    }


----------------------------------------------------------------------------------------------------
project "StringRefTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    files {
        "test/StringRefTest.cpp",
    }


----------------------------------------------------------------------------------------------------
project "StringSplitTest"

    kind "ConsoleApp"

    language "C++"

    links { "CmdLine", "gtest", "gtest_main" }

    includedirs { "include/" }

    files {
        "test/StringSplitTest.cpp",
    }


----------------------------------------------------------------------------------------------------
if _ACTION == "clean" then
    os.rmdir("build")
end
