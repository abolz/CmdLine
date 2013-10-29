// This file is distributed under the MIT license.
// See the LICENSE file for details.

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
    std::vector<StringRef> vec{ split({}, ",") };

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, EmptyString2)
{
    std::vector<StringRef> vec{ split("", ",") };

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "");
}

TEST(StringSplitTest, X1)
{
    std::vector<StringRef> vec{ split(",", ",") };

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], "");
}

TEST(StringSplitTest, X2)
{
    std::vector<StringRef> vec{ split(", ", ",") };

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "");
    EXPECT_EQ(vec[1], " ");
}

TEST(StringSplitTest, Z1)
{
    std::vector<StringRef> vec{ split("a", ",") };

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "a");
}

TEST(StringSplitTest, Z2)
{
    std::vector<StringRef> vec{ split("a,", ",") };

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "");
}

TEST(StringSplitTest, Z3)
{
    std::vector<StringRef> vec{ split("a,b", ",") };

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
}

TEST(StringSplitTest, Test6)
{
    std::vector<StringRef> vec{ split("a.b-c,. d, e .f-", any_of(".,-")) };

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

TEST(StringSplitTest, Test7)
{
    std::vector<StringRef> vec{ split("-a-b-c----d", "--") };

    ASSERT_EQ(vec.size(), 3);
    EXPECT_EQ(vec[0], "-a-b-c");
    EXPECT_EQ(vec[1], "");
    EXPECT_EQ(vec[2], "d");
}

TEST(StringSplitTest, Test7_1)
{
    std::vector<StringRef> vec{ split("-a-b-c----d", "-") };

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

TEST(StringSplitTest, Test8)
{
    std::vector<StringRef> vec{ split("a-b-c-d-e", "-", 2) };

    ASSERT_EQ(vec.size(), 2);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
}

TEST(StringSplitTest, EmptySep)
{
    std::vector<StringRef> vec{ split("abc", "") };

    ASSERT_EQ(vec.size(), 1);
    EXPECT_EQ(vec[0], "abc");
}

TEST(StringSplitTest, Iterator)
{
    auto I = split("a,b,c,d", ",");
    auto E = I.end();

    std::vector<StringRef> vec;

    for (; I != E; ++I)
    {
        vec.push_back(*I);
    }

    ASSERT_EQ(vec.size(), 4);
    EXPECT_EQ(vec[0], "a");
    EXPECT_EQ(vec[1], "b");
    EXPECT_EQ(vec[2], "c");
    EXPECT_EQ(vec[3], "d");
}


#if 1

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
