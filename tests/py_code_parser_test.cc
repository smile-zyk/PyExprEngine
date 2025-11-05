#include "core/expr_common.h"
#include "python/py_code_parser.h"
#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <pybind11/embed.h>


using namespace xequation;
using namespace xequation::python;

class PyCodeParserTest : public ::testing::Test
{
  protected:
    virtual void SetUp()
    {
        parser_.reset(new PyCodeParser());
    }

    virtual void TearDown()
    {
        parser_.reset();
    }

    std::unique_ptr<PyCodeParser> parser_;
};

TEST_F(PyCodeParserTest, ParseSimple)
{
    auto result = parser_->Parse("a = b + c");
    EXPECT_EQ(result.name, "a");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre("b", "c"));
    EXPECT_EQ(result.type, ParseType::kVarDecl);
    EXPECT_EQ(result.content, "a = b + c");

    result = parser_->Parse("import math as mt");
    EXPECT_EQ(result.name, "mt");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre());
    EXPECT_EQ(result.type, ParseType::kImport);
    EXPECT_EQ(result.content, "import math as mt");

    result = parser_->Parse("from math import cos");
    EXPECT_EQ(result.name, "cos");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre());
    EXPECT_EQ(result.type, ParseType::kImportFrom);
    EXPECT_EQ(result.content, "from math import cos");

    result = parser_->Parse("def hello(): print('Hello World')");
    EXPECT_EQ(result.name, "hello");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre());
    EXPECT_EQ(result.type, ParseType::kFuncDecl);
    EXPECT_EQ(result.content, "def hello(): print('Hello World')");

    result = parser_->Parse("class Person: pass");
    EXPECT_EQ(result.name, "Person");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre());
    EXPECT_EQ(result.type, ParseType::kClassDecl);
    EXPECT_EQ(result.content, "class Person: pass");
}

TEST_F(PyCodeParserTest, ParseError)
{
    try
    {
        auto result = parser_->Parse("a+b");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(
            e.what(),
            "Unsupported statement type: Expr. Supported types: FunctionDef, ClassDef, Import, ImportFrom, Assign"
        );
    }

    try
    {
        auto result = parser_->Parse("b = te+");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(e.what(), "Invalid syntax in code: b = te+");
    }

    try
    {
        auto result = parser_->Parse("print = 1+a");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(e.what(), "Name 'print' is a builtin and cannot be redefined");
    }

    try
    {
        auto result = parser_->Parse("import os, math");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(e.what(), "Import statement can only import one module at a time");
    }

    try
    {
        auto result = parser_->Parse("from math import sin, cos");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(e.what(), "From...import statement can only import one symbol at a time");
    }

    try
    {
        auto result = parser_->Parse("a,b = 1,2");
        FAIL() << "Expected ParseException";
    }
    catch (const ParseException &e)
    {
        EXPECT_STREQ(e.what(), "Assignment target must be a variable name");
    }
}

TEST_F(PyCodeParserTest, ParseComplex)
{
    auto result = parser_->Parse("a = 3 + 4j + 2 - 1j + 1 + 2j");
    EXPECT_EQ(result.name, "a");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre());
    EXPECT_EQ(result.type, ParseType::kVarDecl);
    EXPECT_EQ(result.content, "a = 3 + 4j + 2 - 1j + 1 + 2j");

    result = parser_->Parse("matrix = [[(i * j) + (i - j) * 1j for j in range(1, 4)] for i in range(1, 5)]");
    EXPECT_EQ(result.name, "matrix");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre("i", "j"));
    EXPECT_EQ(result.type, ParseType::kVarDecl);
    EXPECT_EQ(result.content, "matrix = [[(i * j) + (i - j) * 1j for j in range(1, 4)] for i in range(1, 5)]");

    result = parser_->Parse("condition = (global_flag := True) and any(complex(i, j).real > 0 for i in range(1, 10, 3) for j in range(1, 10, 4))");
    EXPECT_EQ(result.name, "condition");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre("i", "j"));
    EXPECT_EQ(result.type, ParseType::kVarDecl);
    EXPECT_EQ(result.content, "condition = (global_flag := True) and any(complex(i, j).real > 0 for i in range(1, 10, 3) for j in range(1, 10, 4))");

    result = parser_->Parse("func = process_data(filter_data(load_data(\"file.txt\")))");
    EXPECT_EQ(result.name, "func");
    EXPECT_THAT(result.dependencies, testing::UnorderedElementsAre("process_data", "filter_data", "load_data"));
    EXPECT_EQ(result.type, ParseType::kVarDecl);
    EXPECT_EQ(result.content, "func = process_data(filter_data(load_data(\"file.txt\")))");
}

TEST_F(PyCodeParserTest, CacheBasicFunctionality) {
    std::string code = "t = x + y";

    auto result1 = parser_->Parse(code);
    EXPECT_EQ(parser_->GetCacheSize(), 1u);

    auto result2 = parser_->Parse(code);
    EXPECT_EQ(parser_->GetCacheSize(), 1u);

    EXPECT_EQ(result1.name, result2.name);
    EXPECT_EQ(result1.content, result2.content);
    EXPECT_EQ(result1.dependencies, result2.dependencies);
    EXPECT_EQ(result1.type, result2.type);
}

TEST_F(PyCodeParserTest, CacheDifferentExpressions) {
    std::string code1 = "t0 = x + y";
    std::string code2 = "t1 = a * b";
    std::string code3 = "t2 = func(param)";

    parser_->Parse(code1);
    EXPECT_EQ(parser_->GetCacheSize(), 1u);

    parser_->Parse(code2);
    EXPECT_EQ(parser_->GetCacheSize(), 2u);

    parser_->Parse(code3);
    EXPECT_EQ(parser_->GetCacheSize(), 3u);

    parser_->Parse(code1);
    EXPECT_EQ(parser_->GetCacheSize(), 3u);
}

TEST_F(PyCodeParserTest, ClearCache) {
    std::string code = "t = test_variable";
    parser_->Parse(code);

    EXPECT_GT(parser_->GetCacheSize(), 0u);
    parser_->ClearCache();
    EXPECT_EQ(parser_->GetCacheSize(), 0u);

    parser_->Parse(code);
    EXPECT_EQ(parser_->GetCacheSize(), 1u);
}

TEST_F(PyCodeParserTest, CacheSizeLimit) {
    parser_->SetMaxCacheSize(3);

    parser_->Parse("t = a");
    parser_->Parse("t = b");
    parser_->Parse("t = c");
    parser_->Parse("t = d");

    EXPECT_LE(parser_->GetCacheSize(), 3u);
}

TEST_F(PyCodeParserTest, CacheLRUBehavior) {
    parser_->SetMaxCacheSize(2);

    parser_->Parse("t = expr1");
    parser_->Parse("t = expr2");

    parser_->Parse("t = expr1");

    parser_->Parse("t = expr3");

    EXPECT_EQ(parser_->GetCacheSize(), 2u);

    parser_->Parse("t = expr2");
    EXPECT_EQ(parser_->GetCacheSize(), 2u);
}

TEST_F(PyCodeParserTest, SetMaxCacheSizeDynamic) {
    EXPECT_EQ(parser_->GetCacheSize(), 0u);

    parser_->SetMaxCacheSize(2);
    parser_->Parse("t = a");
    parser_->Parse("t = b");
    parser_->Parse("t = c");
    EXPECT_LE(parser_->GetCacheSize(), 2u);

    parser_->ClearCache();
    parser_->SetMaxCacheSize(10);
    for (int i = 0; i < 8; ++i) {
        parser_->Parse("t = expr" + std::to_string(i));
    }
    EXPECT_EQ(parser_->GetCacheSize(), 8u);

    parser_->Parse("t = expr8");
    parser_->Parse("t = expr9");
    EXPECT_EQ(parser_->GetCacheSize(), 10u);
}

TEST_F(PyCodeParserTest, CacheWithIdenticalContent) {
    std::string code1 = "t = x + y";
    std::string code2 = "t = x + y";

    parser_->Parse(code1);
    size_t size1 = parser_->GetCacheSize();

    parser_->Parse(code2);
    size_t size2 = parser_->GetCacheSize();

    EXPECT_EQ(size1, size2);
}

TEST_F(PyCodeParserTest, CacheWithDifferentContent) {
    std::string code1 = "t = x + y";
    std::string code2 = "t = x + y ";

    parser_->Parse(code1);
    size_t size1 = parser_->GetCacheSize();

    parser_->Parse(code2);
    size_t size2 = parser_->GetCacheSize();

    EXPECT_EQ(size2, size1 + 1);
}

TEST_F(PyCodeParserTest, MultipleClearCache) {
    parser_->Parse("t = x");
    EXPECT_GT(parser_->GetCacheSize(), 0u);

    parser_->ClearCache();
    EXPECT_EQ(parser_->GetCacheSize(), 0u);

    parser_->ClearCache();
    EXPECT_EQ(parser_->GetCacheSize(), 0u);

    parser_->Parse("t = y");
    EXPECT_EQ(parser_->GetCacheSize(), 1u);
}

TEST_F(PyCodeParserTest, CachePerformance) {
    std::string complex_expr = "t = sqrt(x*x + y*y) + sin(angle) * cos(angle)";

    auto start1 = std::chrono::high_resolution_clock::now();
    auto result1 = parser_->Parse(complex_expr);
    auto end1 = std::chrono::high_resolution_clock::now();
    auto duration1 = std::chrono::duration_cast<std::chrono::microseconds>(end1 - start1);

    auto start2 = std::chrono::high_resolution_clock::now();
    auto result2 = parser_->Parse(complex_expr);
    auto end2 = std::chrono::high_resolution_clock::now();
    auto duration2 = std::chrono::duration_cast<std::chrono::microseconds>(end2 - start2);

    EXPECT_LE(duration2.count(), duration1.count());

    EXPECT_EQ(result1.name, result2.name);
    EXPECT_EQ(result1.content, result2.content);
    EXPECT_EQ(result1.dependencies, result2.dependencies);
    EXPECT_EQ(result1.type, result2.type);
}

int main(int argc, char **argv)
{
    ::testing::InitGoogleTest(&argc, argv);
    py::scoped_interpreter guard{};
    int ret = RUN_ALL_TESTS();
    return ret;
}