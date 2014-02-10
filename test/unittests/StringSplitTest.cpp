// This file is distributed under the MIT license.
// See the LICENSE file for details.

#define SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL 0

#include "Support/StringSplit.h"

#include <array>
#include <forward_list>
#include <list>
#include <vector>

#include <gtest/gtest.h>

using namespace support;
using namespace support::strings;

TEST(StringSplitTest, EmptyString1)
{
    auto vec = std::vector<StringRef>(split(StringRef(), ","));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, EmptyString2)
{
    auto vec = std::vector<StringRef>(split("", ","));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, EmptyString3)
{
    auto vec = std::vector<StringRef>(split(StringRef(), AnyOfDelimiter(",")));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, EmptyString4)
{
    auto vec = std::vector<StringRef>(split("", AnyOfDelimiter(",")));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, X1)
{
    auto vec = std::vector<StringRef>(split(",", ","));

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "");
}

TEST(StringSplitTest, X2)
{
    auto vec = std::vector<StringRef>(split(", ", ","));

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], " ");
}

TEST(StringSplitTest, Z1)
{
    auto vec = std::vector<StringRef>(split("a", ","));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "a");
}

TEST(StringSplitTest, Z2)
{
    auto vec = std::vector<StringRef>(split("a,", ","));

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "");
}

TEST(StringSplitTest, Z3)
{
    auto vec = std::vector<StringRef>(split("a,b", ","));

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
}

TEST(StringSplitTest, Test6)
{
    auto vec = std::vector<StringRef>(split("a.b-c,. d, e .f-", AnyOfDelimiter(".,-")));

    ASSERT_EQ(vec.size(), 8);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
    EXPECT_EQ(vec[2], "c");
    EXPECT_EQ(vec[3], "");
    EXPECT_EQ(vec[4], " d");
    EXPECT_EQ(vec[5], " e ");
    EXPECT_EQ(vec[6], "f");
    EXPECT_EQ(vec[7], "");
}

TEST(StringSplitTest, Test6_1)
{
    auto vec = std::vector<StringRef>(split("-a-b-c-", "-"));

    ASSERT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "a");
    EXPECT_EQ(vec[2], "b");
    EXPECT_EQ(vec[3], "c");
    EXPECT_EQ(vec[4], "");
}

TEST(StringSplitTest, Test7)
{
    auto vec = std::vector<StringRef>(split("-a-b-c----d", "--"));

    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "-a-b-c");
    EXPECT_EQ(vec[1], "");
    EXPECT_EQ(vec[2], "d");
}

TEST(StringSplitTest, Test7_1)
{
    auto vec = std::vector<StringRef>(split("-a-b-c----d", "-"));

    ASSERT_EQ(vec.size(), 8);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "a");
    EXPECT_EQ(vec[2], "b");
    EXPECT_EQ(vec[3], "c");
    EXPECT_EQ(vec[4], "");
    EXPECT_EQ(vec[5], "");
    EXPECT_EQ(vec[6], "");
    EXPECT_EQ(vec[7], "d");
}

TEST(StringSplitTest, EmptySepLiteral)
{
    {
        auto vec = std::vector<StringRef>(split(StringRef(), ""));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "");
    }
    {
        auto vec = std::vector<StringRef>(split(StringRef(), AnyOfDelimiter("")));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "");
    }
    {
        auto vec = std::vector<StringRef>(split("", ""));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "");
    }
    {
        auto vec = std::vector<StringRef>(split("", AnyOfDelimiter("")));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "");
    }
    {
        auto vec = std::vector<StringRef>(split("x", ""));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "x");
    }
    {
        auto vec = std::vector<StringRef>(split("x", AnyOfDelimiter("")));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "x");
    }
    {
        auto vec = std::vector<StringRef>(split("abc", ""));

#if SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL
        ASSERT_EQ(vec.size(), 3);
        EXPECT_EQ(vec[0], "a");
        EXPECT_EQ(vec[1], "b");
        EXPECT_EQ(vec[2], "c");
#else
        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "abc");
#endif
    }
    {
        auto vec = std::vector<StringRef>(split("abc", AnyOfDelimiter("")));

        ASSERT_EQ(vec.size(), 1);
        EXPECT_EQ(vec[0], "abc");
    }
}

TEST(StringSplitTest, Iterator)
{
    {
        auto R = split("a,b,c,d", ",");

        std::vector<StringRef> vec;

        for (auto I = R.begin(), E = R.end(); I != E; ++I)
        {
            vec.push_back(*I);
        }

        ASSERT_EQ(vec.size(), 4);
        EXPECT_EQ(vec[0], "a");
        EXPECT_EQ(vec[1], "b");
        EXPECT_EQ(vec[2], "c");
        EXPECT_EQ(vec[3], "d");
    }
    {
        auto R = split("a,b,c,d", ",");

        EXPECT_EQ(*R.begin(), "a");
        EXPECT_EQ(*R.begin(), "a");
        EXPECT_EQ(*R.begin(), "a");

        ++R.begin();

        EXPECT_EQ(*R.begin(), "b");
        EXPECT_EQ(*R.begin(), "b");
        EXPECT_EQ(*R.begin(), "b");
    }
    {
        auto R = split("a,b,c,d", ",");

        auto I = R.begin();
        auto J = R.begin();

        ++I;

        EXPECT_EQ(I, I);
        EXPECT_EQ(*I, *I);
        EXPECT_EQ(I, J);
        EXPECT_EQ(*I, *J);
        EXPECT_EQ(J, J);
        EXPECT_EQ(*J, *J);

        I = ++J;

        ++J;

        EXPECT_EQ(I, J);
        EXPECT_EQ(*I, *J);
    }
}

template <class T> using IsStringRef = typename std::is_same<T, StringRef>::type;
template <class T> using IsStdString = typename std::is_same<T, std::string>::type;

TEST(StringSplitTest, StringType)
{
    // Test Split_string type deduction...

    static_assert( IsStringRef< Split_string::type<char*                >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char*&               >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char*&&              >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char (&)[1]          >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char const*          >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char const*&         >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char const*&&        >>::value, "" );
    static_assert( IsStringRef< Split_string::type<char const (&)[1]    >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef            >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef&           >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef&&          >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef const      >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef const&     >>::value, "" );
    static_assert( IsStringRef< Split_string::type<StringRef const&&    >>::value, "" );
    static_assert( IsStringRef< Split_string::type<std::string&         >>::value, "" );
    static_assert( IsStringRef< Split_string::type<std::string const&   >>::value, "" );

    static_assert( IsStdString< Split_string::type<std::string          >>::value, "" );
    static_assert( IsStdString< Split_string::type<std::string&&        >>::value, "" );
    static_assert( IsStdString< Split_string::type<std::string const    >>::value, "" );
    static_assert( IsStdString< Split_string::type<std::string const&&  >>::value, "" );
}

static std::string make_string()
{
    return
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,"
        "abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc,abc"
        ;
}

TEST(StringSplitTest, Temp)
{
    using result_type = decltype(split(make_string(), ","));

    std::cout << "sizeof(void*)       = " << sizeof(void*) << std::endl;
    std::cout << "sizeof(std::string) = " << sizeof(std::string) << std::endl;
    std::cout << "sizeof(S)           = " << sizeof(result_type) << std::endl;
    std::cout << "sizeof(S::iterator) = " << sizeof(result_type::iterator) << std::endl;

    std::vector<std::string> vec;

    {
        auto S = split(make_string(), ",");

        auto T = S;

        auto U = std::move(T);

        auto V = U;

        vec = std::vector<std::string>(V);
    }

    for (auto&& s : vec)
    {
        EXPECT_EQ(s, "abc");
    }
}

TEST(StringSplitTest, KeepEmpty)
{
    std::vector<StringRef> vec(split(", a ,b , c,,  ,d", ",", KeepEmpty()));

    ASSERT_EQ(vec.size(), 7);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], " a ");
    EXPECT_EQ(vec[2], "b ");
    EXPECT_EQ(vec[3], " c");
    EXPECT_EQ(vec[4], "");
    EXPECT_EQ(vec[5], "  ");
    EXPECT_EQ(vec[6], "d");
}

TEST(StringSplitTest, SkipEmpty)
{
    std::vector<StringRef> vec(split(", a ,b , c,,  ,d", ",", SkipEmpty()));

    ASSERT_EQ(vec.size(), 5);
    EXPECT_EQ(vec[0], " a ");
    EXPECT_EQ(vec[1], "b ");
    EXPECT_EQ(vec[2], " c");
    EXPECT_EQ(vec[3], "  ");
    EXPECT_EQ(vec[4], "d");
}

TEST(StringSplitTest, SkipSpace)
{
    std::vector<StringRef> vec(split(", a ,b , c,,  ,d", ",", SkipSpace()));

    ASSERT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], " a ");
    EXPECT_EQ(vec[1], "b ");
    EXPECT_EQ(vec[2], " c");
    EXPECT_EQ(vec[3], "d");
}

TEST(StringSplitTest, Trim)
{
    std::vector<StringRef> vec(split(", a ,b , c,,  ,d", ",", Trim()));

    ASSERT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
    EXPECT_EQ(vec[2], "c");
    EXPECT_EQ(vec[3], "d");
}
