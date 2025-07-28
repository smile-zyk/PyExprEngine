#include "gtest/gtest.h"

#include "exprvalue.h"

TEST(baseTest, init)
{
    ExprValue value = 1;
    int res = value.Cast<int>();
    EXPECT_EQ(res, 1);
}