// This file is distributed under the MIT license.
// See the LICENSE file for details.

#include "Support/StringRef.h"

#include <gtest/gtest.h>

using namespace support;

template <class StringT>
void CheckFind()
{
    StringT E = "";
    StringT X = "x";
    StringT Y = "y";
    StringT S = "xxx";

    EXPECT_EQ(E.find(E),                                0);
    EXPECT_EQ(S.find(E),                                0);
    EXPECT_EQ(E.find(E, 2),                             StringT::npos);
    EXPECT_EQ(S.find(E, 2),                             2);
    EXPECT_EQ(E.find(E, 8),                             StringT::npos);
    EXPECT_EQ(S.find(E, 8),                             StringT::npos);
    EXPECT_EQ(E.find(E, StringT::npos),                 StringT::npos);
    EXPECT_EQ(S.find(E, StringT::npos),                 StringT::npos);

    EXPECT_EQ(E.find_first_of(E),                       StringT::npos);
    EXPECT_EQ(S.find_first_of(E),                       StringT::npos);
    EXPECT_EQ(E.find_first_of(E, 2),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(E, 2),                    StringT::npos);
    EXPECT_EQ(E.find_first_of(E, 8),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(E, 8),                    StringT::npos);
    EXPECT_EQ(E.find_first_of(E, StringT::npos),        StringT::npos);
    EXPECT_EQ(S.find_first_of(E, StringT::npos),        StringT::npos);
    EXPECT_EQ(E.find_first_of(X),                       StringT::npos);
    EXPECT_EQ(S.find_first_of(X),                       0);
    EXPECT_EQ(E.find_first_of(X, 2),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(X, 2),                    2);
    EXPECT_EQ(E.find_first_of(X, 8),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(X, 8),                    StringT::npos);
    EXPECT_EQ(E.find_first_of(X, StringT::npos),        StringT::npos);
    EXPECT_EQ(S.find_first_of(X, StringT::npos),        StringT::npos);
    EXPECT_EQ(E.find_first_of(Y, 2),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(Y, 2),                    StringT::npos);
    EXPECT_EQ(E.find_first_of(Y, 8),                    StringT::npos);
    EXPECT_EQ(S.find_first_of(Y, 8),                    StringT::npos);
    EXPECT_EQ(E.find_first_of(Y, StringT::npos),        StringT::npos);
    EXPECT_EQ(S.find_first_of(Y, StringT::npos),        StringT::npos);

    EXPECT_EQ(E.find_first_not_of(E),                   StringT::npos);
    EXPECT_EQ(S.find_first_not_of(E),                   0);
    EXPECT_EQ(E.find_first_not_of(E, 2),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(E, 2),                2);
    EXPECT_EQ(E.find_first_not_of(E, 8),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(E, 8),                StringT::npos);
    EXPECT_EQ(E.find_first_not_of(E, StringT::npos),    StringT::npos);
    EXPECT_EQ(S.find_first_not_of(E, StringT::npos),    StringT::npos);
    EXPECT_EQ(E.find_first_not_of(X),                   StringT::npos);
    EXPECT_EQ(S.find_first_not_of(X),                   StringT::npos);
    EXPECT_EQ(E.find_first_not_of(X, 2),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(X, 2),                StringT::npos);
    EXPECT_EQ(E.find_first_not_of(X, 8),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(X, 8),                StringT::npos);
    EXPECT_EQ(E.find_first_not_of(X, StringT::npos),    StringT::npos);
    EXPECT_EQ(S.find_first_not_of(X, StringT::npos),    StringT::npos);
    EXPECT_EQ(E.find_first_not_of(Y, 2),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(Y, 2),                2);
    EXPECT_EQ(E.find_first_not_of(Y, 8),                StringT::npos);
    EXPECT_EQ(S.find_first_not_of(Y, 8),                StringT::npos);
    EXPECT_EQ(E.find_first_not_of(Y, StringT::npos),    StringT::npos);
    EXPECT_EQ(S.find_first_not_of(Y, StringT::npos),    StringT::npos);

    EXPECT_EQ(E.find_last_of(E),                        StringT::npos);
    EXPECT_EQ(S.find_last_of(E),                        StringT::npos);
    EXPECT_EQ(E.find_last_of(E, 2),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(E, 2),                     StringT::npos);
    EXPECT_EQ(E.find_last_of(E, 8),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(E, 8),                     StringT::npos);
    EXPECT_EQ(E.find_last_of(E, 0),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(E, 0),                     StringT::npos);
    EXPECT_EQ(E.find_last_of(E),                        StringT::npos);
    EXPECT_EQ(S.find_last_of(E),                        StringT::npos);
    EXPECT_EQ(E.find_last_of(X, 2),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(X, 2),                     2);
    EXPECT_EQ(E.find_last_of(X, 8),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(X, 8),                     2);
    EXPECT_EQ(E.find_last_of(X, 0),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(X, 0),                     0);
    EXPECT_EQ(E.find_last_of(Y, 2),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(Y, 2),                     StringT::npos);
    EXPECT_EQ(E.find_last_of(Y, 8),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(Y, 8),                     StringT::npos);
    EXPECT_EQ(E.find_last_of(Y, 0),                     StringT::npos);
    EXPECT_EQ(S.find_last_of(Y, 0),                     StringT::npos);

    EXPECT_EQ(E.find_last_not_of(E),                    StringT::npos);
    EXPECT_EQ(S.find_last_not_of(E),                    2);
    EXPECT_EQ(E.find_last_not_of(E, 2),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(E, 2),                 2);
    EXPECT_EQ(E.find_last_not_of(E, 8),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(E, 8),                 2);
    EXPECT_EQ(E.find_last_not_of(E, 0),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(E, 0),                 0);
    EXPECT_EQ(E.find_last_not_of(E),                    StringT::npos);
    EXPECT_EQ(S.find_last_not_of(E),                    2);
    EXPECT_EQ(E.find_last_not_of(X, 2),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(X, 2),                 StringT::npos);
    EXPECT_EQ(E.find_last_not_of(X, 8),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(X, 8),                 StringT::npos);
    EXPECT_EQ(E.find_last_not_of(X, 0),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(X, 0),                 StringT::npos);
    EXPECT_EQ(E.find_last_not_of(Y, 2),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(Y, 2),                 2);
    EXPECT_EQ(E.find_last_not_of(Y, 8),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(Y, 8),                 2);
    EXPECT_EQ(E.find_last_not_of(Y, 0),                 StringT::npos);
    EXPECT_EQ(S.find_last_not_of(Y, 0),                 0);
}

TEST(StringRefTest, CheckStdString)
{
    CheckFind<std::string>();
}

TEST(StringRefTest, CheckStringRef)
{
    CheckFind<StringRef>();
}
