#include "gtest/gtest.h"
#include <string>
#include <vector>

#include "exprvalue.h"

TEST(Value, ConvertToString)
{
    int a = 1;
    std::string int_str = ConvertToString(a);
    std::string int_expect_str = "1";
    EXPECT_STREQ(int_str.c_str(), int_expect_str.c_str());

    float b = 3.f;
    std::string float_str = ConvertToString(b);
    std::string float_expect_str = "3.000000";
    EXPECT_STREQ(float_str.c_str(), float_expect_str.c_str());

    double c = 3.1415926;
    std::string double_str = ConvertToString(c);
    std::string double_expect_str = "3.141593";
    EXPECT_STREQ(double_str.c_str(), double_expect_str.c_str());

    std::string d = "test_string";
    std::string string_str = ConvertToString(d);
    EXPECT_STREQ(string_str.c_str(), d.c_str());

    struct A
    {
        std::string ToString() const { return "struct A"; }
    };

    A e;
    std::string custom_str = ConvertToString(e);
    std::string custom_expect_str = "struct A";
    EXPECT_STREQ(custom_str.c_str(), custom_expect_str.c_str());

    ExprValue f = "test_expr";
    std::string expr_str = ConvertToString(f);
    std::string expr_expect_str = "test_expr";
    EXPECT_STREQ(expr_str.c_str(), expr_expect_str.c_str());

    ExprValue g = ExprValue::Null();
    std::string null_str = ConvertToString(g);
    std::string null_expect_str = "null";
    EXPECT_STREQ(null_str.c_str(), null_expect_str.c_str());

    ExprValue h = std::vector<ExprValue>{1, 2.5, "test"};
    std::string list_str = ConvertToString(h);
    std::string list_expect_str = "[1, 2.500000, test]";
    EXPECT_STREQ(list_str.c_str(), list_expect_str.c_str());

    ExprValue i = std::map<ExprValue, ExprValue>{{std::vector<ExprValue>{1,2,"3"}, ExprValue(1)}, {"key2", ExprValue("value2")}};
    std::string map_str = ConvertToString(i);
    std::string map_expect_str = "{key2: value2, [1, 2, 3]: 1}";
    EXPECT_STREQ(map_str.c_str(), map_expect_str.c_str());

    ExprValue j = std::set<ExprValue>{1, 2, 3};
    std::string set_str = ConvertToString(j);
    std::string set_expect_str = "{1, 2, 3}";
    EXPECT_STREQ(set_str.c_str(), set_expect_str.c_str());

    auto j_set = j.Cast<std::set<ExprValue>>();
    for (const auto& item : j_set) {
        EXPECT_TRUE(item == ExprValue(1) || item == ExprValue(2) || item == ExprValue(3));
    }
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}