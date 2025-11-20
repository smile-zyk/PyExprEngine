#include "core/equation.h"
#include "core/equation_common.h"
#include "core/equation_context.h"
#include "core/equation_group.h"
#include "core/equation_manager.h"

#include "gmock/gmock.h"
#include <regex>
#include <string>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <unordered_set>

using namespace xequation;

class EquationParser
{
  public:
    static ParseResult parseMultipleExpressions(const std::string &input)
    {
        ParseResult result;

        size_t start = 0;
        size_t end = 0;

        while (end != std::string::npos)
        {
            end = input.find(';', start);

            std::string expr = input.substr(start, (end == std::string::npos) ? std::string::npos : end - start);

            expr = std::regex_replace(expr, std::regex(R"(^\s+|\s+$)"), "");

            if (!expr.empty())
            {
                ParseResultItem item = parseExpression(expr);
                result.push_back(item);
            }

            if (end != std::string::npos)
            {
                start = end + 1;
            }
        }

        return result;
    }

  private:
    static ParseResultItem parseExpression(const std::string &expr)
    {
        ParseResultItem item;

        std::regex assign_regex(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)\s*$)");
        std::smatch assign_match;

        if (std::regex_match(expr, assign_match, assign_regex))
        {
            std::string variable_name = assign_match[1].str();
            std::string expression = assign_match[2].str();

            item.name = variable_name;

            parseDependencies(expression, item);
            item.content = expression;
            item.type = Equation::Type::kVariable;
        }
        else
        {
            throw ParseException("Syntax error: assignment operator '=' not found or variable name missing");
        }

        return item;
    }

    static void parseDependencies(const std::string &expr, ParseResultItem &item)
    {
        std::regex var_regex(R"(\b[A-Za-z_][A-Za-z0-9_]*\b)");
        auto words_begin = std::sregex_iterator(expr.begin(), expr.end(), var_regex);
        auto words_end = std::sregex_iterator();

        std::vector<std::string> res;

        for (std::sregex_iterator i = words_begin; i != words_end; ++i)
        {
            std::string var_name = i->str();

            if (std::regex_match(var_name, std::regex(R"(^\d+$)")))
            {
                continue;
            }

            res.push_back(var_name);
        }
        item.dependencies = res;
    }
};

ExecResult ExecExpr(const std::string &code, EquationContext *context)
{
    ExecResult result;

    std::regex assign_regex(R"(^\s*([A-Za-z_][A-Za-z0-9_]*)\s*=\s*(.+)\s*$)");
    std::smatch assign_match;

    if (std::regex_match(code, assign_match, assign_regex))
    {
        std::string name = assign_match[1];
        std::string expr = assign_match[2];

        std::regex expr_regex(
            R"(^\s*(([A-Za-z_][A-Za-z0-9_]*|\d+)(\s*([\+\-\*\/])\s*([A-Za-z_][A-Za-z0-9_]*|\d+))?)\s*$)"
        );
        std::smatch expr_match;

        if (std::regex_match(expr, expr_match, expr_regex))
        {
            std::string var1 = expr_match[2];
            int val1 = 0;

            if (std::regex_match(var1, std::regex(R"(^\d+$)")))
            {
                val1 = std::stoi(var1);
            }
            else if (context->Contains(var1))
            {
                Value v1 = context->Get(var1);
                if (v1.Type() == typeid(int))
                {
                    val1 = v1.Cast<int>();
                }
                else
                {
                    result.status = Equation::Status::kTypeError;
                    result.message = "Variable " + var1 + " is not an integer";
                    return result;
                }
            }
            else
            {
                result.status = Equation::Status::kNameError;
                result.message = "Variable " + var1 + " not found";
                return result;
            }

            if (!expr_match[4].matched)
            {
                result.status = Equation::Status::kSuccess;
                context->Set(name, val1);
                return result;
            }

            std::string op = expr_match[4];
            std::string var2 = expr_match[5];
            int val2 = 0;

            if (std::regex_match(var2, std::regex(R"(^\d+$)")))
            {
                val2 = std::stoi(var2);
            }
            else if (context->Contains(var2))
            {
                Value v2 = context->Get(var2);
                if (v2.Type() == typeid(int))
                {
                    val2 = v2.Cast<int>();
                }
                else
                {
                    result.status = Equation::Status::kTypeError;
                    result.message = "Variable " + var2 + " is not an integer";
                    return result;
                }
            }
            else
            {
                result.status = Equation::Status::kNameError;
                result.message = "Variable " + var2 + " not found";
                return result;
            }

            if (op == "+")
            {
                result.status = Equation::Status::kSuccess;
                context->Set(name, val1 + val2);
            }
            else if (op == "-")
            {
                result.status = Equation::Status::kSuccess;
                context->Set(name, val1 - val2);
            }
            else if (op == "*")
            {
                result.status = Equation::Status::kSuccess;
                context->Set(name, val1 * val2);
            }
            else if (op == "/")
            {
                if (val2 == 0)
                {
                    result.status = Equation::Status::kZeroDivisionError;
                    result.message = "Division by zero";
                }
                else
                {
                    result.status = Equation::Status::kSuccess;
                    context->Set(name, val1 / val2);
                }
            }
            else
            {
                result.status = Equation::Status::kAttributeError;
                result.message = "Invalid operator: " + op;
            }
        }
        else
        {
            result.status = Equation::Status::kSyntaxError;
            result.message = "Invalid expression syntax";
        }
    }
    else
    {
        result.status = Equation::Status::kSyntaxError;
        result.message = "Invalid assignment syntax. Expected: variable = expression";
    }

    return result;
}

//
class MockExprContext : public EquationContext
{
  public:
    virtual Value Get(const std::string &var_name) const override
    {
        if (Contains(var_name))
        {
            return manager_.at(var_name);
        }
        return Value::Null();
    }

    virtual void Set(const std::string &var_name, const Value &value) override
    {
        manager_[var_name] = value;
    }

    virtual bool Remove(const std::string &var_name) override
    {
        if (Contains(var_name))
        {
            manager_.erase(var_name);
            return true;
        }
        return false;
    }

    virtual void Clear() override
    {
        manager_.clear();
    }

    virtual bool Contains(const std::string &var_name) const override
    {
        return manager_.find(var_name) != manager_.end();
    }

    virtual std::unordered_set<std::string> keys() const override
    {
        std::unordered_set<std::string> key_set;
        for (const auto &entry : manager_)
        {
            key_set.insert(entry.first);
        }
        return key_set;
    }

  private:
    std::unordered_map<std::string, Value> manager_;
};

class EquationManagerTest : public testing::Test
{
  protected:
    EquationManagerTest()
        : manager_(
              std::unique_ptr<MockExprContext>(new MockExprContext()), ExecExpr,
              EquationParser::parseMultipleExpressions
          )
    {
    }

    void SetUp() override
    {
        manager_.Reset();
    }

    EquationManager manager_;
};

TEST_F(EquationManagerTest, EquationGroupAddRemoveEditGet)
{
    EquationGroupId id_0 = manager_.AddEquationGroup("A=1");
    EXPECT_TRUE(manager_.IsEquationGroupExist(id_0));
    EXPECT_TRUE(manager_.IsEquationExist("A"));

    const EquationGroup *group_0 = manager_.GetEquationGroup(id_0);
    const Equation *equation_a = manager_.GetEquation("A");

    EXPECT_TRUE(group_0);
    EXPECT_EQ(group_0->id(), id_0);
    EXPECT_EQ(group_0->GetEquationNames(), std::vector<std::string>{"A"});
    EXPECT_EQ(group_0->manaegr(), &manager_);
    EXPECT_EQ(group_0->statement(), "A=1");

    EXPECT_TRUE(equation_a);
    EXPECT_TRUE(group_0->IsEquationExist("A"));
    EXPECT_TRUE(equation_a == group_0->GetEquation("A"));
    EXPECT_EQ(equation_a->name(), "A");
    EXPECT_EQ(equation_a->dependencies(), std::vector<std::string>{});
    EXPECT_EQ(equation_a->content(), "1");
    EXPECT_EQ(equation_a->group_id(), id_0);
    EXPECT_EQ(equation_a->manager(), &manager_);
    EXPECT_EQ(equation_a->message(), "");
    EXPECT_EQ(equation_a->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_a->status(), Equation::Status::kPending);

    manager_.EditEquationGroup(id_0, "A=2;B=A");
    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_TRUE(manager_.IsEquationExist("B"));

    EXPECT_TRUE(equation_a);
    EXPECT_TRUE(group_0->IsEquationExist("A"));
    EXPECT_TRUE(equation_a == group_0->GetEquation("A"));
    EXPECT_EQ(equation_a->name(), "A");
    EXPECT_EQ(equation_a->dependencies(), std::vector<std::string>{});
    EXPECT_EQ(equation_a->content(), "2");
    EXPECT_EQ(equation_a->group_id(), id_0);
    EXPECT_EQ(equation_a->manager(), &manager_);
    EXPECT_EQ(equation_a->message(), "");
    EXPECT_EQ(equation_a->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_a->status(), Equation::Status::kPending);

    const Equation *equation_b = manager_.GetEquation("B");
    EXPECT_TRUE(equation_b);
    EXPECT_TRUE(group_0->IsEquationExist("B"));
    EXPECT_TRUE(equation_b == group_0->GetEquation("B"));
    EXPECT_EQ(equation_b->name(), "B");
    EXPECT_EQ(equation_b->dependencies(), std::vector<std::string>{"A"});
    EXPECT_EQ(equation_b->content(), "A");
    EXPECT_EQ(equation_b->group_id(), id_0);
    EXPECT_EQ(equation_b->manager(), &manager_);
    EXPECT_EQ(equation_b->message(), "");
    EXPECT_EQ(equation_b->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_b->status(), Equation::Status::kPending);

    manager_.EditEquationGroup(id_0, "B=3;C=B+1");
    EXPECT_FALSE(manager_.IsEquationExist("A"));
    EXPECT_TRUE(manager_.IsEquationExist("B"));
    EXPECT_TRUE(manager_.IsEquationExist("C"));

    EXPECT_TRUE(equation_b);
    EXPECT_TRUE(group_0->IsEquationExist("B"));
    EXPECT_TRUE(equation_b == group_0->GetEquation("B"));
    EXPECT_EQ(equation_b->name(), "B");
    EXPECT_EQ(equation_b->dependencies(), std::vector<std::string>{});
    EXPECT_EQ(equation_b->content(), "3");
    EXPECT_EQ(equation_b->group_id(), id_0);
    EXPECT_EQ(equation_b->manager(), &manager_);
    EXPECT_EQ(equation_b->message(), "");
    EXPECT_EQ(equation_b->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_b->status(), Equation::Status::kPending);

    const Equation *equation_c = manager_.GetEquation("C");
    EXPECT_TRUE(equation_c);
    EXPECT_TRUE(group_0->IsEquationExist("C"));
    EXPECT_TRUE(equation_c == group_0->GetEquation("C"));
    EXPECT_EQ(equation_c->name(), "C");
    EXPECT_EQ(equation_c->dependencies(), std::vector<std::string>{"B"});
    EXPECT_EQ(equation_c->content(), "B+1");
    EXPECT_EQ(equation_c->group_id(), id_0);
    EXPECT_EQ(equation_c->manager(), &manager_);
    EXPECT_EQ(equation_c->message(), "");
    EXPECT_EQ(equation_c->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_c->status(), Equation::Status::kPending);

    EquationGroupId id_1 = manager_.AddEquationGroup("D=B+2;E=D+B");
    EXPECT_TRUE(manager_.IsEquationGroupExist(id_1));
    EXPECT_TRUE(manager_.IsEquationExist("D"));
    EXPECT_TRUE(manager_.IsEquationExist("E"));

    const EquationGroup *group_1 = manager_.GetEquationGroup(id_1);
    const Equation *equation_d = manager_.GetEquation("D");
    const Equation *equation_e = manager_.GetEquation("E");

    EXPECT_TRUE(group_1);
    EXPECT_EQ(group_1->id(), id_1);
    EXPECT_THAT(group_1->GetEquationNames(), ::testing::ElementsAre("D", "E"));

    EXPECT_TRUE(equation_d);
    EXPECT_TRUE(group_1->IsEquationExist("D"));
    EXPECT_TRUE(equation_d == group_1->GetEquation("D"));
    EXPECT_EQ(equation_d->name(), "D");
    EXPECT_EQ(equation_d->dependencies(), std::vector<std::string>{"B"});
    EXPECT_EQ(equation_d->content(), "B+2");
    EXPECT_EQ(equation_d->group_id(), id_1);
    EXPECT_EQ(equation_d->manager(), &manager_);
    EXPECT_EQ(equation_d->message(), "");
    EXPECT_EQ(equation_d->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_d->status(), Equation::Status::kPending);

    EXPECT_TRUE(equation_e);
    EXPECT_TRUE(group_1->IsEquationExist("E"));
    EXPECT_TRUE(equation_e == group_1->GetEquation("E"));
    EXPECT_EQ(equation_e->name(), "E");
    EXPECT_THAT(equation_e->dependencies(), ::testing::ElementsAre("D", "B"));
    EXPECT_EQ(equation_e->content(), "D+B");
    EXPECT_EQ(equation_e->group_id(), id_1);
    EXPECT_EQ(equation_e->manager(), &manager_);
    EXPECT_EQ(equation_e->message(), "");
    EXPECT_EQ(equation_e->type(), Equation::Type::kVariable);
    EXPECT_EQ(equation_e->status(), Equation::Status::kPending);

    manager_.RemoveEquationGroup(id_1);
    EXPECT_FALSE(manager_.IsEquationGroupExist(id_1));
    EXPECT_FALSE(manager_.IsEquationExist("D"));
    EXPECT_FALSE(manager_.IsEquationExist("E"));
}

TEST_F(EquationManagerTest, EquationException)
{
    auto id = manager_.AddEquationGroup("A=1;B=2");
    manager_.AddEquationGroup("C=3");
    try
    {
        auto tmp_id = manager_.AddEquationGroup("A=3");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationAlreayExists);
        EXPECT_EQ(e.equation_name(), "A");
    }

    try
    {
        manager_.EditEquationGroup(id, "C=2");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationAlreayExists);
        EXPECT_EQ(e.equation_name(), "C");
    }

    manager_.RemoveEquationGroup(id);

    try
    {
        manager_.EditEquationGroup(id, "C=1");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationGroupNotFound);
        EXPECT_EQ(e.group_id(), id);
    }

    try
    {
        manager_.RemoveEquationGroup(id);
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationGroupNotFound);
        EXPECT_EQ(e.group_id(), id);
    }
    try
    {
        manager_.UpdateEquation("E");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNotFound);
        EXPECT_EQ(e.equation_name(), "E");
    }
    try
    {
        manager_.UpdateEquationGroup(id);
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationGroupNotFound);
        EXPECT_EQ(e.group_id(), id);
    }
}

TEST_F(EquationManagerTest, EquationManagerUpdate) {}

TEST_F(EquationManagerTest, Eval) {}

// TEST_F(EquationManagerTest, AddEditRemoveEquationStatement)
// {
//     std::string multiple_statements = "A=1;B=A+C";

//     manager_.AddMultipleEquations(multiple_statements);
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kPending, "A=1");
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kPending, "B=A+C", {"A=C"});
//     VerifyNode(manager_.graph()->GetNode("B"), {"A"}, {});
//     VerifyNode(manager_.graph()->GetNode("A"), {}, {"B"});
//     VerifyEdges({{"B=A"}, {"B=C"}}, true);

//     EXPECT_THROW(manager_.AddEquationGroup("A=2"), DuplicateEquationNameError);
//     EXPECT_THROW(manager_.EditEquation("A=C=D"), std::runtime_error);
//     EXPECT_THROW(manager_.RemoveEquation("A"), std::runtime_error);

//     std::string new_statements = "C=A;B=A+C;E=1";
//     EXPECT_THROW(manager_.AddMultipleEquations(new_statements), DuplicateEquationNameError);
//     manager_.EditMultipleEquations(multiple_statements, new_statements);
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kPending, "C=A", {"A"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kPending, "B=A+C", {"A=C"});
//     VerifyVar(manager_.GetEquation("E"), Equation::Type::kVariable, Equation::Status::kPending, "E=1");
//     EXPECT_FALSE(manager_.IsEquationExist("A"));
//     VerifyNode(manager_.graph()->GetNode("B"), {"C"}, {});
//     VerifyNode(manager_.graph()->GetNode("C"), {}, {"B"});
//     VerifyNode(manager_.graph()->GetNode("E"), {}, {});
//     VerifyEdges({{"C=A"}, {"B=C"}, {"B=A"}}, true);

//     EXPECT_THROW(manager_.AddMultipleEquations(multiple_statements), DuplicateEquationNameError);

//     EXPECT_THROW(manager_.RemoveMultipleEquations(multiple_statements), std::runtime_error);
//     manager_.RemoveMultipleEquations(new_statements);
//     EXPECT_FALSE(manager_.IsEquationExist("A"));
//     EXPECT_FALSE(manager_.IsEquationExist("B"));
//     EXPECT_FALSE(manager_.IsEquationExist("C"));
//     EXPECT_FALSE(manager_.IsEquationExist("E"));
// }

// TEST_F(EquationManagerTest, CycleDetection)
// {
//     std::string statements = "A=B*C;B=D;C=2";
//     manager_.AddMultipleEquations(statements);

//     VerifyNode(manager_.graph()->GetNode("A"), {"B=C"}, {});
//     VerifyNode(manager_.graph()->GetNode("B"), {}, {"A"});
//     VerifyNode(manager_.graph()->GetNode("C"), {}, {"A"});
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kPending, "A=B*C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kPending, "B=D", {"D"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kPending, "C=2");
//     VerifyEdges({{"A=B"}, {"A=C"}, {"B=D"}}, true);

//     EXPECT_THROW(manager_.EditEquation("B=D=B"), std::runtime_error);
//     EXPECT_THROW(manager_.AddEquationGroup("D=A+B"), DependencyCycleException);
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kPending, "A=B*C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kPending, "B=D", {"D"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kPending, "C=2");
//     EXPECT_EQ(manager_.GetEquation("D"), nullptr);
//     VerifyEdges({{"A=B"}, {"A=C"}, {"B=D"}}, true);

//     manager_.AddEquationGroup("D=E");
//     EXPECT_THROW(manager_.AddEquationGroup("E=B"), DependencyCycleException);
//     VerifyNode(manager_.graph()->GetNode("D"), {}, {"B"});
//     VerifyVar(manager_.GetEquation("D"), Equation::Type::kVariable, Equation::Status::kPending, "D=E", {"E"});
//     EXPECT_FALSE(manager_.IsEquationExist("E"));
//     VerifyEdges({{"A=B"}, {"A=C"}, {"B=D"}, {"D=E"}}, true);
// }

// TEST_F(EquationManagerTest, UpdateContext)
// {
//     std::string statements0 = "A=B+C;B=D+E;C=F;D=1;F=10";
//     manager_.AddMultipleEquations(statements0);
//     manager_.AddEquationGroup("E=5");
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kPending, "A=B+C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kPending, "B=D+E", {"D=E"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kPending, "C=F", {"F"});
//     VerifyVar(manager_.GetEquation("D"), Equation::Type::kVariable, Equation::Status::kPending, "D=1", {});
//     VerifyVar(manager_.GetEquation("E"), Equation::Type::kVariable, Equation::Status::kPending, "E=5", {});
//     VerifyVar(manager_.GetEquation("F"), Equation::Type::kVariable, Equation::Status::kPending, "F=10", {});
//     manager_.Update();
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kSuccess, "A=B+C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kSuccess, "B=D+E", {"D=E"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kSuccess, "C=F", {"F"});
//     VerifyVar(manager_.GetEquation("D"), Equation::Type::kVariable, Equation::Status::kSuccess, "D=1", {});
//     VerifyVar(manager_.GetEquation("E"), Equation::Type::kVariable, Equation::Status::kSuccess, "E=5", {});
//     VerifyVar(manager_.GetEquation("F"), Equation::Type::kVariable, Equation::Status::kSuccess, "F=10", {});
//     EXPECT_TRUE(manager_.context()->Contains("A"));
//     EXPECT_TRUE(manager_.context()->Contains("B"));
//     EXPECT_TRUE(manager_.context()->Contains("C"));
//     EXPECT_TRUE(manager_.context()->Contains("D"));
//     EXPECT_TRUE(manager_.context()->Contains("E"));
//     EXPECT_TRUE(manager_.context()->Contains("F"));
//     EXPECT_TRUE(manager_.context()->Get("A").Cast<int>() == 16);
//     EXPECT_TRUE(manager_.context()->Get("B").Cast<int>() == 6);
//     EXPECT_TRUE(manager_.context()->Get("C").Cast<int>() == 10);
//     EXPECT_TRUE(manager_.context()->Get("D").Cast<int>() == 1);
//     EXPECT_TRUE(manager_.context()->Get("E").Cast<int>() == 5);
//     EXPECT_TRUE(manager_.context()->Get("F").Cast<int>() == 10);

//     std::string statements1 = "A=B+C;B=D+E;C=F;F=10";
//     manager_.EditMultipleEquations(statements0,statements1);
//     EXPECT_FALSE(manager_.context()->Contains("D"));
//     EXPECT_FALSE(manager_.IsEquationExist("D"));
//     manager_.Update();
//     EXPECT_FALSE(manager_.context()->Contains("D"));
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kNameError, "A=B+C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kNameError, "B=D+E", {"D=E"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kSuccess, "C=F", {"F"});
//     VerifyVar(manager_.GetEquation("E"), Equation::Type::kVariable, Equation::Status::kSuccess, "E=5", {});
//     VerifyVar(manager_.GetEquation("F"), Equation::Type::kVariable, Equation::Status::kSuccess, "F=10", {});
//     EXPECT_FALSE(manager_.context()->Contains("A"));
//     EXPECT_FALSE(manager_.context()->Contains("B"));
//     EXPECT_TRUE(manager_.context()->Contains("C"));
//     EXPECT_FALSE(manager_.context()->Contains("D"));
//     EXPECT_TRUE(manager_.context()->Contains("E"));
//     EXPECT_TRUE(manager_.context()->Contains("F"));
//     EXPECT_TRUE(manager_.context()->Get("C").Cast<int>() == 10);
//     EXPECT_TRUE(manager_.context()->Get("E").Cast<int>() == 5);
//     EXPECT_TRUE(manager_.context()->Get("F").Cast<int>() == 10);

//     manager_.AddEquationGroup("D=E");
//     std::string statements2 = "A=B+C;B=D+E;C=E+F;F=10";
//     manager_.EditMultipleEquations(statements1, statements2);
//     manager_.Update();
//     VerifyVar(manager_.GetEquation("A"), Equation::Type::kVariable, Equation::Status::kSuccess, "A=B+C", {"B=C"});
//     VerifyVar(manager_.GetEquation("B"), Equation::Type::kVariable, Equation::Status::kSuccess, "B=D+E", {"D=E"});
//     VerifyVar(manager_.GetEquation("C"), Equation::Type::kVariable, Equation::Status::kSuccess, "C=E+F", {"E=F"});
//     VerifyVar(manager_.GetEquation("D"), Equation::Type::kVariable, Equation::Status::kSuccess, "D=E", {"E"});
//     VerifyVar(manager_.GetEquation("E"), Equation::Type::kVariable, Equation::Status::kSuccess, "E=5", {});
//     VerifyVar(manager_.GetEquation("F"), Equation::Type::kVariable, Equation::Status::kSuccess, "F=10", {});
//     EXPECT_TRUE(manager_.context()->Contains("A"));
//     EXPECT_TRUE(manager_.context()->Contains("B"));
//     EXPECT_TRUE(manager_.context()->Contains("C"));
//     EXPECT_TRUE(manager_.context()->Contains("D"));
//     EXPECT_TRUE(manager_.context()->Contains("E"));
//     EXPECT_TRUE(manager_.context()->Contains("F"));
//     EXPECT_TRUE(manager_.context()->Get("A").Cast<int>() == 25);
//     EXPECT_TRUE(manager_.context()->Get("B").Cast<int>() == 10);
//     EXPECT_TRUE(manager_.context()->Get("C").Cast<int>() == 15);
//     EXPECT_TRUE(manager_.context()->Get("D").Cast<int>() == 5);
//     EXPECT_TRUE(manager_.context()->Get("E").Cast<int>() == 5);
//     EXPECT_TRUE(manager_.context()->Get("F").Cast<int>() == 10);

//     manager_.EditEquation("E=E=6");
//     manager_.UpdateEquation("E");
//     EXPECT_TRUE(manager_.context()->Get("A").Cast<int>() == 28);
//     EXPECT_TRUE(manager_.context()->Get("B").Cast<int>() == 12);
//     EXPECT_TRUE(manager_.context()->Get("C").Cast<int>() == 16);
//     EXPECT_TRUE(manager_.context()->Get("D").Cast<int>() == 6);
//     EXPECT_TRUE(manager_.context()->Get("E").Cast<int>() == 6);
//     EXPECT_TRUE(manager_.context()->Get("F").Cast<int>() == 10);
// }

int main(int argc, char **argv)
{
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}