#include <gtest/gtest.h>
#include <pybind11/pytypes.h>

#include "python/py_expr_engine.h"
#include "python/py_expr_context.h"

using namespace xexprengine;

TEST(PyExprEngineTest, InitTest)
{
    PyExprEngine::SetPyEnvConfig(PyExprEngine::PyEnvConfig());
    PyExprEngine::GetInstance();
}

TEST(PyExprEngineTest, ParseTest)
{
    auto result = PyExprEngine::GetInstance().Parse("a + b + c");

    EXPECT_EQ(result.status, VariableStatus::kParseSuccess);
    EXPECT_EQ(result.variables.size(), 3);
}

TEST(PyExprEngineTest, EvalTest)
{
    auto result = PyExprEngine::GetInstance().Parse("a + b + c");

    EXPECT_EQ(result.status, VariableStatus::kParseSuccess);
    EXPECT_EQ(result.variables.size(), 3);

    auto context = std::make_unique<PyExprContext>();
    std::vector<int> values = {1, 2, 3};
    context->SetValue("a", values);
    context->SetValue("b", 2);
    context->SetValue("c", 3);
    context->SetExpression("d", "len(a) + b * c");
    context->Update();

    auto v = context->GetContextValue("d");
    auto obj = v.Cast<py::object>();
    EXPECT_EQ(obj.cast<int>(), 9);
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}