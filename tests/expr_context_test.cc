#include "expr_common.h"
#include "expr_context.h"
#include "variable.h"

#include <regex>
#include <string>

#include <gtest/gtest.h>

using namespace xexprengine;

//
class MockExprContext : public ExprContext
{
  public:
    MockExprContext() : ExprContext()
    {
        set_parse_callback([this](const std::string &expr) { return this->ParseCallback(expr); });
        set_evaluate_callback([this](const std::string &expr, const ExprContext *ctx) {
            return this->EvaluateCallback(expr, ctx);
        });
    }

    virtual Value GetContextValue(const std::string &var_name) const override
    {
        if (IsContextValueExist(var_name))
        {
            return context_.at(var_name);
        }
        return Value::Null();
    }

    virtual void SetContextValue(const std::string &var_name, const Value &value) override
    {
        context_[var_name] = value;
    }

    virtual bool RemoveContextValue(const std::string &var_name) override
    {
        if (IsContextValueExist(var_name))
        {
            context_.erase(var_name);
            return true;
        }
        return false;
    }

    virtual void ClearContextValue() override
    {
        context_.clear();
    }

    virtual bool IsContextValueExist(const std::string &var_name) const override
    {
        return context_.find(var_name) != context_.end();
    }

    ParseResult ParseCallback(const std::string &expr)
    {
        // use regular expression to parse variables
        // only support two variables and one operator
        // literal only support integer
        // varible name only support A-Z
        // check format: VAR OP VAR
        // if A + 1 only add A to dependency
        ParseResult result;
        std::regex expr_regex(R"(^\s*([A-Z]+|\d+)\s*([\+\-\*\/])\s*([A-Z]+|\d+)\s*$)");
        std::smatch match;
        // if A + 1 only add A to dependency
        if (std::regex_match(expr, match, expr_regex))
        {
            // extract variables and operator
            if (std::regex_match(match[1].str(), std::regex(R"(^[A-Z]+$)")))
            {
                result.variables.insert(match[1]);
            }
            if (std::regex_match(match[3].str(), std::regex(R"(^[A-Z]+$)")))
            {
                result.variables.insert(match[3]);
            }
        }
        else
        {
            result.success = false;
            result.error_code = ErrorCode::ParseError;
            result.error_message = "Invalid expression format";
        }
        return result;
    }

    EvalResult EvaluateCallback(const std::string &expr, const ExprContext *context)
    {
        EvalResult result;
        std::regex expr_regex(R"(^\s*([A-Z]+|\d+)\s*([\+\-\*\/])\s*([A-Z]+|\d+)\s*$)");
        std::smatch match;
        if (std::regex_match(expr, match, expr_regex))
        {
            // extract variables and operator
            std::string var1 = match[1];
            std::string op = match[2];
            std::string var2 = match[3];

            int val1 = 0;
            int val2 = 0;

            // get value of var1
            if (std::regex_match(var1, std::regex(R"(^\d+$)")))
            {
                val1 = std::stoi(var1);
            }
            else if (context->IsContextValueExist(var1))
            {
                Value v1 = context->GetContextValue(var1);
                if (v1.Type() == typeid(int))
                {
                    val1 = v1.Cast<int>();
                }
                else
                {
                    result.error_code = ErrorCode::TypeMismatch;
                    result.error_message = "Variable " + var1 + " is not an integer";
                    return result;
                }
            }
            else
            {
                result.error_code = ErrorCode::UnknownVariable;
                result.error_message = "Variable " + var1 + " not found";
                return result;
            }

            // get value of var2
            if (std::regex_match(var2, std::regex(R"(^\d+$)")))
            {
                val2 = std::stoi(var2);
            }
            else if (context->IsContextValueExist(var2))
            {
                Value v2 = context->GetContextValue(var2);
                if (v2.Type() == typeid(int))
                {
                    val2 = v2.Cast<int>();
                }
                else
                {
                    result.error_code = ErrorCode::TypeMismatch;
                    result.error_message = "Variable " + var2 + " is not an integer";
                    return result;
                }
            }
            else
            {
                result.error_code = ErrorCode::UnknownVariable;
                result.error_message = "Variable " + var2 + " not found";
                return result;
            }

            // perform calculation
            if (op == "+")
            {
                result.value = Value(val1 + val2);
            }
            else if (op == "-")
            {
                result.value = Value(val1 - val2);
            }
            else if (op == "*")
            {
                result.value = Value(val1 * val2);
            }
            else if (op == "/")
            {
                if (val2 == 0)
                {
                    result.error_code = ErrorCode::DivisionByZero;
                    result.error_message = "Division by zero";
                }
                else
                {
                    result.value = Value(val1 / val2);
                }
            }
            else
            {
                result.error_code = ErrorCode::InvalidOperation;
                result.error_message = "Invalid operator: " + op;
            }
        }
        else
        {
            result.error_code = ErrorCode::ParseError;
            result.error_message = "Invalid expression format";
        }
        return result;
    }

  private:
    std::unordered_map<std::string, Value> context_;
};

class ExprContextTest : public testing::Test
{
  protected:
    void SetUp() override
    {
        context_.Reset();
    }

    MockExprContext context_;
};

TEST_F(ExprContextTest, BasicVariableOperations)
{
    // variable value set to context util update

    // Add raw variable A=10
    context_.SetValue("A", 10);
    EXPECT_TRUE(context_.IsVariableExist("A"));
    EXPECT_FALSE(context_.IsContextValueExist("A"));
    EXPECT_EQ(context_.GetVariable("A")->GetType(), Variable::Type::Raw);
    EXPECT_EQ(context_.GetVariable("A")->As<RawVariable>()->value().Cast<int>(), 10);
    EXPECT_TRUE(context_.UpdateVariable("A"));
    EXPECT_TRUE(context_.IsContextValueExist("A"));
    EXPECT_EQ(context_.GetContextValue("A").Cast<int>(), 10);
    EXPECT_FALSE(context_.UpdateVariable("A")); // not dirty

    context_.SetExpression("B", "A + 1");
    EXPECT_TRUE(context_.IsVariableExist("B"));
    EXPECT_FALSE(context_.IsContextValueExist("B"));
    EXPECT_EQ(context_.GetVariable("B")->GetType(), Variable::Type::Expr);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->expression(), "A + 1");
    EXPECT_TRUE(context_.UpdateVariable("B"));
    EXPECT_TRUE(context_.IsContextValueExist("B"));
    EXPECT_EQ(context_.GetContextValue("B").Cast<int>(), 11);
    EXPECT_FALSE(context_.UpdateVariable("B")); // not dirty
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->cached_value().Cast<int>(), 11);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->error_code(), ErrorCode::Success);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->error_message(), "");

    // change A to 20
    context_.SetValue("A", 20);
    context_.Update();
    EXPECT_EQ(context_.GetContextValue("A").Cast<int>(), 20);
    EXPECT_EQ(context_.GetContextValue("B").Cast<int>(), 21);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->cached_value().Cast<int>(), 21);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->error_code(), ErrorCode::Success);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->error_message(), "");

    // change B expression to A * 2
    context_.SetExpression("B", "A * 2");
    context_.Update();
    EXPECT_EQ(context_.GetContextValue("A").Cast<int>(), 20);
    EXPECT_EQ(context_.GetContextValue("B").Cast<int>(), 40);
    EXPECT_EQ(context_.GetVariable("B")->As<ExprVariable>()->cached_value().Cast<int>(), 40);

    // Add expr variable C = B - A
    context_.SetExpression("C", "B - A");
    context_.Update();
    EXPECT_TRUE(context_.IsVariableExist("C"));
    EXPECT_EQ(context_.GetContextValue("C").Cast<int>(), 20);
    EXPECT_EQ(context_.GetVariable("C")->As<ExprVariable>()->cached_value().Cast<int>(), 20);
    EXPECT_EQ(context_.GetVariable("C")->As<ExprVariable>()->error_code(), ErrorCode::Success);
    EXPECT_EQ(context_.GetVariable("C")->As<ExprVariable>()->error_message(), "");
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}