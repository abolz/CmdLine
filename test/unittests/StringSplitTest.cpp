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

template <class Range>
class RangeConverter
{
    Range const& R;

public:
    RangeConverter(Range const& R)
        : R(R)
    {
    }

    template <class Container>
    operator Container() const
    {
        return Container(R.begin(), R.end());
    }
};

template <class Range>
RangeConverter<Range> convert(Range const& R)
{
    return RangeConverter<Range>(R);
}

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

TEST(StringSplitTest, EmptySepLiteral1)
{
    auto vec = std::vector<StringRef>(split("abc", ""));

#if !SUPPORT_STRINGSPLIT_EMPTY_LITERAL_IS_SPECIAL
    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "abc");
#else
    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
    EXPECT_EQ(vec[2], "c");
#endif
}

TEST(StringSplitTest, EmptySepLiteral2)
{
    auto vec = std::vector<StringRef>(split("x", ""));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "x");
}

TEST(StringSplitTest, EmptySepLiteral3)
{
    auto vec = std::vector<StringRef>(split("", ""));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, EmptySepAnyOf)
{
    auto vec = std::vector<StringRef>(split("abc", AnyOfDelimiter("")));

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "abc");
}

TEST(StringSplitTest, Iterator)
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

TEST(StringSplitTest, Range1)
{
    auto R = split("a,b,c,d", ",");

    auto I = R.begin();
    auto E = R.end();

    EXPECT_NE(I, E);
    EXPECT_EQ(*R.begin(), "a");
    EXPECT_EQ(*R.begin(), "a");
    EXPECT_EQ(*R.begin(), "a");
    EXPECT_EQ(*R.begin(), "a");
    EXPECT_EQ(*R.begin(), "a");
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

#if 0

#include <boost/range/adaptor/filtered.hpp>
#include <boost/range/adaptor/transformed.hpp>
#include <boost/range/algorithm.hpp>

TEST(StringSplitTest, AdaptorsTransformed)
{
    using namespace boost::adaptors;

    struct AppendX
    {
        std::string operator ()(StringRef Str) const {
            return Str + "+x";
        }
    };

    std::vector<std::string> vec
        = convert(split("a,b,c,d", ",") | transformed(AppendX()));

    ASSERT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "a+x");
    EXPECT_EQ(vec[1], "b+x");
    EXPECT_EQ(vec[2], "c+x");
    EXPECT_EQ(vec[3], "d+x");
}

TEST(StringSplitTest, AdaptorsFiltered)
{
    using namespace boost::adaptors;

    struct SkipEmpty
    {
        bool operator ()(StringRef Str) const {
            return !Str.empty();
        }
    };

    std::vector<std::string> vec
        = convert(split("a,,,b,,,c,,,d,,,", ",") | filtered(SkipEmpty()));

    ASSERT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
    EXPECT_EQ(vec[2], "c");
    EXPECT_EQ(vec[3], "d");
}

#endif
