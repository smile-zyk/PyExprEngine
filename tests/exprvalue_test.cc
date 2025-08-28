#include "gtest/gtest.h"
#include <string>
#include <unordered_set>
#include <vector>

#include "value.h"

using namespace xexprengine;

TEST(Value, InitializationAndNullCheck) 
{
    Value ev1;
    EXPECT_TRUE(ev1.IsNull());

    Value ev2 = 42;
    EXPECT_FALSE(ev2.IsNull());
    EXPECT_EQ(ev2.Cast<int>(), 42);

    Value ev3 = 2.71828;
    EXPECT_FALSE(ev3.IsNull());
    EXPECT_EQ(ev3.Cast<double>(), 2.71828);

    Value ev4 = "Hello";
    EXPECT_FALSE(ev4.IsNull());
    EXPECT_EQ(ev4.Cast<std::string>(), "Hello");

    Value ev5 = std::vector<int>{1, 2, 3};
    EXPECT_FALSE(ev5.IsNull());
    EXPECT_EQ(ev5.Cast<std::vector<int>>(), std::vector<int>({1, 2, 3}));
    
    Value ev6 = std::list<double>{1.1, 2.2, 3.3};
    EXPECT_FALSE(ev6.IsNull());
    EXPECT_EQ(ev6.Cast<std::list<double>>(), std::list<double>({1.1, 2.2, 3.3}));

    Value ev7 = std::map<std::string, int>{{"one", 1}, {"two", 2}};
    EXPECT_FALSE(ev7.IsNull());
    auto map_value = ev7.Cast<std::map<std::string, int>>();
    EXPECT_EQ(map_value["one"], 1);
    EXPECT_EQ(map_value["two"], 2);

    Value ev8 = std::set<std::string>{"apple", "banana"};
    EXPECT_FALSE(ev8.IsNull());
    EXPECT_EQ(ev8.Cast<std::set<std::string>>(), std::set<std::string>({"apple", "banana"}));
}

TEST(Value, NestedValue) 
{
    Value ev1 = std::vector<Value>{Value(1), Value(2.5), Value("test")};
    EXPECT_FALSE(ev1.IsNull());
    auto vec_value = ev1.Cast<std::vector<Value>>();
    EXPECT_EQ(vec_value.size(), 3);
    EXPECT_EQ(vec_value[0].Cast<int>(), 1);
    EXPECT_EQ(vec_value[1].Cast<double>(), 2.5);
    EXPECT_EQ(vec_value[2].Cast<std::string>(), "test");
    EXPECT_STREQ(ev1.ToString().c_str(), "[1, 2.500000, test]");

    Value ev2 = std::map<Value, Value>{{"key1", Value(100)}, {5, Value("value")}};
    EXPECT_FALSE(ev2.IsNull());
    auto map_value = ev2.Cast<std::map<Value, Value>>();
    EXPECT_EQ(map_value["key1"].Cast<int>(), 100);
    EXPECT_EQ(map_value[5].Cast<std::string>(), "value");
    EXPECT_STREQ(ev2.ToString().c_str(), "{key1: 100, 5: value}");

    Value ev3 = std::set<Value>{Value(1), Value(2), Value(3)};
    EXPECT_FALSE(ev3.IsNull());
    auto set_value = ev3.Cast<std::set<Value>>();
    EXPECT_EQ(set_value.size(), 3);
    EXPECT_TRUE(set_value.find(Value(1)) != set_value.end());
    EXPECT_TRUE(set_value.find(Value(2)) != set_value.end());
    EXPECT_TRUE(set_value.find(Value(3)) != set_value.end());
    EXPECT_STREQ(ev3.ToString().c_str(), "{1, 2, 3}");

    Value ev4 = std::unordered_set<Value>{Value(1), Value(2), Value(3)};
    EXPECT_FALSE(ev4.IsNull());
    auto hash_set_value = ev4.Cast<std::unordered_set<Value>>();
    EXPECT_EQ(hash_set_value.size(), 3);
    EXPECT_TRUE(hash_set_value.find(Value(1)) != hash_set_value.end());
    EXPECT_TRUE(hash_set_value.find(Value(2)) != hash_set_value.end());
    EXPECT_TRUE(hash_set_value.find(Value(3)) != hash_set_value.end());
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}