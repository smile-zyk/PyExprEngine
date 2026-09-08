#include "core/equation_common.h"
#include "core/equation_manager.h"
#include "core/equation_value.h"
#include "equation_value_test_utils.h"

#include "environment.h"  // rel::Environment
#include "value.h"         // rel::Value

#include <gtest/gtest.h>
#include <gmock/gmock.h>

#include <boost/uuid/random_generator.hpp>

#include <string>
#include <utility>
#include <vector>

using namespace xequation;

namespace
{

// 便捷函数：逐条 AddEquation（批量文本入口已删除）。
void AddEquations(EquationManager &mgr,
                  std::initializer_list<std::pair<std::string, std::string>> eqs)
{
    for (const auto &kv : eqs)
    {
        mgr.AddEquation(kv.first, kv.second);
    }
}

int GetInt(const EquationManager &mgr, const std::string &name)
{
    return AsScalar<int>(mgr.GetVariable(name));
}

} // namespace

class EquationManagerTest : public testing::Test
{
  protected:
    EquationManagerTest() : manager_(EquationManager::GetInstance()) {}

    void SetUp() override
    {
        manager_.Reset();
    }

    EquationManager &manager_;
};

TEST_F(EquationManagerTest, EquationAddRemoveEditGet)
{
    ObjectId id_a = manager_.AddEquation("A", "1");
    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_TRUE(manager_.IsEquationExist(id_a));
    EXPECT_EQ(manager_.GetEquationIds().size(), 1u);

    const Equation *equation_a = manager_.GetEquation("A");
    ASSERT_NE(equation_a, nullptr);
    EXPECT_EQ(equation_a->name, "A");
    EXPECT_EQ(equation_a->content, "1");
    EXPECT_EQ(equation_a->id, id_a);
    EXPECT_EQ(manager_.GetEquationById(id_a), equation_a);

    // content edit (name unchanged)
    manager_.EditEquation(id_a, "2");
    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_EQ(manager_.GetEquation("A")->content, "2");
    EXPECT_EQ(manager_.GetEquation("A")->id, id_a);  // same identity

    // rename: old name gone, new name appears with the SAME id
    ObjectId id_c = manager_.RenameEquation(id_a, "C");
    EXPECT_FALSE(manager_.IsEquationExist("A"));
    EXPECT_TRUE(manager_.IsEquationExist("C"));
    EXPECT_EQ(id_c, id_a);
    EXPECT_EQ(manager_.GetEquation("C")->id, id_a);

    manager_.RemoveEquation("C");
    EXPECT_FALSE(manager_.IsEquationExist("C"));
    EXPECT_TRUE(manager_.GetEquationIds().empty());
}

TEST_F(EquationManagerTest, EquationEditNameAndContent)
{
    // Combined edit: rename + change content atomically (same id).
    ObjectId id_a = manager_.AddEquation("A", "1");
    EXPECT_EQ(manager_.GetEquation("A")->content, "1");

    ObjectId id_b = manager_.EditEquation(id_a, "B", "2");
    EXPECT_EQ(id_b, id_a);  // identity preserved
    EXPECT_FALSE(manager_.IsEquationExist("A"));
    EXPECT_TRUE(manager_.IsEquationExist("B"));
    EXPECT_EQ(manager_.GetEquation("B")->name, "B");
    EXPECT_EQ(manager_.GetEquation("B")->content, "2");
    EXPECT_EQ(manager_.GetEquation("B")->id, id_a);

    // Same new name -> degenerate to a content-only edit.
    manager_.EditEquation(id_a, "B", "3");
    EXPECT_TRUE(manager_.IsEquationExist("B"));
    EXPECT_EQ(manager_.GetEquation("B")->content, "3");

    // Rename onto an existing name.
    manager_.AddEquation("C", "9");
    try
    {
        manager_.EditEquation(id_a, "C", "4");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationAlreadyExists);
        EXPECT_EQ(e.equation_name(), "C");
    }

    // Invalid new name.
    try
    {
        manager_.EditEquation(id_a, "1x", "4");
        FAIL();
    }
    catch (const ParseException &)
    {
    }

    // Missing equation (by id).
    const ObjectId missing_id = boost::uuids::random_generator()();
    try
    {
        manager_.EditEquation(missing_id, "Z", "4");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNotFound);
        EXPECT_EQ(e.id(), missing_id);
    }
}

TEST_F(EquationManagerTest, EquationTagDefaultsAndCustom)
{
    // No tag supplied -> empty tag (identity defaults are a UI concern).
    manager_.AddEquation("A", "1");
    EXPECT_EQ(manager_.GetEquation("A")->tag, "");

    // A Marker identity can be supplied at creation time.
    manager_.AddEquation("B", "2", "Marker");
    EXPECT_EQ(manager_.GetEquation("B")->tag, "Marker");

    // A custom free-form tag is also accepted.
    manager_.AddEquation("C", "3", "SimulationResult");
    EXPECT_EQ(manager_.GetEquation("C")->tag, "SimulationResult");

    // Edit / rename keep the tag (identity does not change).
    manager_.EditEquation(manager_.GetEquation("C")->id, "4");
    EXPECT_EQ(manager_.GetEquation("C")->tag, "SimulationResult");
    manager_.RenameEquation(manager_.GetEquation("C")->id, "D");
    EXPECT_EQ(manager_.GetEquation("D")->tag, "SimulationResult");
}

TEST_F(EquationManagerTest, ExpressionTagDefaultsAndCustom)
{
    // No tag supplied -> empty tag (identity defaults are a UI concern).
    const ObjectId watch = manager_.AddExpression("1+1");
    EXPECT_EQ(manager_.GetExpression(watch)->tag, "");

    // Graph identity at creation time.
    const ObjectId graph = manager_.AddExpression("1+2", "Graph");
    EXPECT_EQ(manager_.GetExpression(graph)->tag, "Graph");

    // Custom free-form tag.
    const ObjectId custom = manager_.AddExpression("1+3", "Plotted");
    EXPECT_EQ(manager_.GetExpression(custom)->tag, "Plotted");
}

TEST_F(EquationManagerTest, ExpressionAddRemoveSignals)
{
    int added = 0;
    int removing = 0;
    std::string removed_id;
    EquationManager &mgr = manager_;
    ScopedConnection c_added = mgr.signals_manager().ConnectScoped<EquationEvent::kExpressionAdded>(
        [&](const Expression *) { ++added; });
    ScopedConnection c_removing = mgr.signals_manager().ConnectScoped<EquationEvent::kExpressionRemoving>(
        [&](const Expression *) { ++removing; });
    ScopedConnection c_removed = mgr.signals_manager().ConnectScoped<EquationEvent::kExpressionRemoved>(
        [&](const std::string &id) { removed_id = id; });

    const ObjectId expr_id = manager_.AddExpression("x*2", "Graph");
    EXPECT_EQ(added, 1);

    manager_.RemoveExpression(expr_id);
    EXPECT_EQ(removing, 1);
    EXPECT_FALSE(removed_id.empty());

    // Removing a non-existent expression is a no-op: no extra signal.
    manager_.RemoveExpression(expr_id);
    EXPECT_EQ(removing, 1);
}

TEST_F(EquationManagerTest, EquationException)
{
    manager_.AddEquation("A", "1");
    manager_.AddEquation("B", "2");

    // duplicate add
    try
    {
        manager_.AddEquation("A", "3");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationAlreadyExists);
        EXPECT_EQ(e.equation_name(), "A");
    }

    // edit missing
    const ObjectId missing_id = boost::uuids::random_generator()();
    try
    {
        manager_.EditEquation(missing_id, "1");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNotFound);
        EXPECT_EQ(e.id(), missing_id);
    }

    // rename missing
    try
    {
        manager_.RenameEquation(missing_id, "Z");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNotFound);
        EXPECT_EQ(e.id(), missing_id);
    }

    // rename onto an existing name
    const ObjectId id_a = manager_.GetEquation("A")->id;
    try
    {
        manager_.RenameEquation(id_a, "B");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationAlreadyExists);
        EXPECT_EQ(e.equation_name(), "B");
    }

    // remove missing -> no-op, no throw
    manager_.RemoveEquation("Z");
    EXPECT_FALSE(manager_.IsEquationExist("Z"));

    // update missing
    try
    {
        manager_.UpdateEquation("E");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNotFound);
    }
}

TEST_F(EquationManagerTest, EquationManagerUpdate)
{
    AddEquations(manager_, {{"A", "B+C"}, {"B", "D+E"}, {"C", "F"}, {"D", "1"}, {"F", "10"}});
    manager_.AddEquation("E", "5");
    manager_.Update();

    EXPECT_TRUE(manager_.HasVariable("A"));
    EXPECT_TRUE(manager_.HasVariable("B"));
    EXPECT_TRUE(manager_.HasVariable("C"));
    EXPECT_TRUE(manager_.HasVariable("D"));
    EXPECT_TRUE(manager_.HasVariable("E"));
    EXPECT_TRUE(manager_.HasVariable("F"));
    EXPECT_EQ(GetInt(manager_, "A"), 16);
    EXPECT_EQ(GetInt(manager_, "B"), 6);
    EXPECT_EQ(GetInt(manager_, "C"), 10);
    EXPECT_EQ(GetInt(manager_, "D"), 1);
    EXPECT_EQ(GetInt(manager_, "E"), 5);
    EXPECT_EQ(GetInt(manager_, "F"), 10);

    // remove D (A/B lose a dependency)
    manager_.RemoveEquation("D");
    manager_.Update();
    EXPECT_FALSE(manager_.HasVariable("A"));
    EXPECT_FALSE(manager_.HasVariable("B"));
    EXPECT_TRUE(manager_.HasVariable("C"));
    EXPECT_TRUE(manager_.HasVariable("E"));
    EXPECT_TRUE(manager_.HasVariable("F"));
    EXPECT_EQ(manager_.GetEquation("A")->status, ResultStatus::kError);
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);

    // re-add D=E and update it: dependents must recover
    manager_.AddEquation("D", "E");
    manager_.UpdateEquation("D");
    EXPECT_TRUE(manager_.HasVariable("A"));
    EXPECT_TRUE(manager_.HasVariable("B"));
    EXPECT_TRUE(manager_.HasVariable("C"));
    EXPECT_TRUE(manager_.HasVariable("D"));
    EXPECT_EQ(GetInt(manager_, "A"), 20);
    EXPECT_EQ(GetInt(manager_, "B"), 10);
    EXPECT_EQ(GetInt(manager_, "C"), 10);
    EXPECT_EQ(GetInt(manager_, "D"), 5);

    // edit C to depend on E too
    manager_.EditEquation(manager_.GetEquation("C")->id, "E+F");
    manager_.UpdateEquation("C");
    EXPECT_EQ(GetInt(manager_, "A"), 25);
    EXPECT_EQ(GetInt(manager_, "B"), 10);
    EXPECT_EQ(GetInt(manager_, "C"), 15);
}

TEST_F(EquationManagerTest, Eval)
{
    AddEquations(manager_, {{"A", "B+C"}, {"B", "D+E"}, {"C", "F"}, {"D", "1"}, {"E", "5"}, {"F", "10"}});
    manager_.Update();
    InterpretResult res = manager_.Eval("A+B");
    EXPECT_EQ(res.status, ResultStatus::kSuccess);
    EXPECT_EQ(AsScalar<int>(res.value), 22);

    res = manager_.Eval("G+1");
    EXPECT_FALSE(res.value.HasValue());
    EXPECT_EQ(res.status, ResultStatus::kError);
}

TEST_F(EquationManagerTest, RenameDependencyDoesNotUpdateDependent)
{
    const ObjectId id_a = manager_.AddEquation("A", "1");
    manager_.AddEquation("B", "A");
    manager_.Update();

    EXPECT_TRUE(manager_.HasVariable("B"));
    EXPECT_EQ(GetInt(manager_, "B"), 1);
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kSuccess);

    // Rename A=1 to C=1.
    manager_.RenameEquation(id_a, "C");

    // "B"'s dependency "A" is gone, so "B" must be dirty.
    EXPECT_TRUE(manager_.graph().GetNode("B")->dirty_flag());

    manager_.UpdateEquation("C");

    EXPECT_FALSE(manager_.HasVariable("B"));
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
    EXPECT_TRUE(manager_.HasVariable("C"));
    EXPECT_EQ(GetInt(manager_, "C"), 1);
}

TEST_F(EquationManagerTest, UpdateEquationAfterRenameRecomputesDirtyDependents)
{
    const ObjectId id_a = manager_.AddEquation("A", "1");
    manager_.AddEquation("B", "A");
    manager_.Update();

    EXPECT_EQ(GetInt(manager_, "B"), 1);

    manager_.RenameEquation(id_a, "C");
    manager_.UpdateEquation("C");

    EXPECT_FALSE(manager_.HasVariable("B"));
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
    EXPECT_TRUE(manager_.HasVariable("C"));
    EXPECT_EQ(GetInt(manager_, "C"), 1);
}

TEST_F(EquationManagerTest, ResetContextThenUpdateRecoversAllValues)
{
    AddEquations(manager_, {{"A", "B+C"}, {"B", "D+E"}, {"C", "F"}, {"D", "1"}, {"E", "5"}, {"F", "10"}});
    manager_.Update();

    EXPECT_EQ(GetInt(manager_, "A"), 16);
    EXPECT_EQ(GetInt(manager_, "B"), 6);
    EXPECT_EQ(GetInt(manager_, "C"), 10);

    // After a successful recompute the nodes should be clean.
    EXPECT_FALSE(manager_.graph().GetNode("A")->dirty_flag());
    EXPECT_FALSE(manager_.graph().GetNode("B")->dirty_flag());

    manager_.ResetContext();

    // The context is cleared and every node should be re-dirtied.
    EXPECT_FALSE(manager_.HasVariable("A"));
    EXPECT_TRUE(manager_.graph().GetNode("A")->dirty_flag());
    EXPECT_TRUE(manager_.graph().GetNode("B")->dirty_flag());
    EXPECT_TRUE(manager_.graph().GetNode("F")->dirty_flag());

    manager_.Update();

    // Everything is restored.
    EXPECT_EQ(GetInt(manager_, "A"), 16);
    EXPECT_EQ(GetInt(manager_, "B"), 6);
    EXPECT_EQ(GetInt(manager_, "C"), 10);
    EXPECT_EQ(GetInt(manager_, "D"), 1);
    EXPECT_EQ(GetInt(manager_, "E"), 5);
    EXPECT_EQ(GetInt(manager_, "F"), 10);
    EXPECT_EQ(manager_.GetEquation("A")->status, ResultStatus::kSuccess);
}

TEST_F(EquationManagerTest, UpdateEquationStatusThenUpdateRecovers)
{
    AddEquations(manager_, {{"A", "1"}, {"B", "A"}});
    manager_.Update();

    EXPECT_EQ(GetInt(manager_, "B"), 1);

    // Simulate an interruption: "B" is marked Error and removed from env.
    manager_.UpdateEquationStatus("B", ResultStatus::kError);
    EXPECT_FALSE(manager_.HasVariable("B"));
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
    EXPECT_TRUE(manager_.graph().GetNode("B")->dirty_flag());

    // A later Update must recompute "B".
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "B"), 1);
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kSuccess);
    EXPECT_FALSE(manager_.graph().GetNode("B")->dirty_flag());
}

// ============================================================================
// Registered expressions (只算不存)
// ============================================================================

TEST_F(EquationManagerTest, ExpressionRegisterAndEvaluate)
{
    AddEquations(manager_, {{"A", "1"}, {"B", "2"}});
    manager_.Update();

    ObjectId expr_id = manager_.AddExpression("A+B");
    EXPECT_TRUE(manager_.IsExpressionExist(expr_id));
    EXPECT_EQ(manager_.GetExpressionIds().size(), 1u);

    const Expression *expr = manager_.GetExpression(expr_id);
    ASSERT_NE(expr, nullptr);
    EXPECT_EQ(expr->content, "A+B");
    EXPECT_THAT(expr->parse_symbols, testing::UnorderedElementsAre("A", "B"));

    // Not evaluated yet.
    EXPECT_FALSE(manager_.GetExpressionValue(expr_id).HasValue());

    manager_.UpdateExpression(expr_id);
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 3);
    EXPECT_EQ(manager_.GetExpression(expr_id)->result.status, ResultStatus::kSuccess);

    // The expression is not a symbol: only the two equations A, B are listed.
    EXPECT_THAT(manager_.GetEquationNames(), ::testing::ElementsAre("A", "B"));
    EXPECT_FALSE(manager_.HasVariable("A+B"));
}

TEST_F(EquationManagerTest, ExpressionRecomputesWhenEquationChanges)
{
    manager_.AddEquation("A", "1");
    manager_.Update();

    ObjectId expr_id = manager_.AddExpression("A*2");
    manager_.Update();
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 2);

    manager_.EditEquation(manager_.GetEquation("A")->id, "5");
    manager_.Update();
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 10);
    EXPECT_EQ(manager_.GetExpression(expr_id)->result.status, ResultStatus::kSuccess);
}

TEST_F(EquationManagerTest, ExpressionRecomputesOnUpdateEquation)
{
    AddEquations(manager_, {{"A", "1"}, {"H", "A"}});
    manager_.Update();

    ObjectId expr_id = manager_.AddExpression("H*2");
    manager_.Update();
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 2);

    manager_.EditEquation(manager_.GetEquation("A")->id, "7");
    manager_.UpdateEquation("A");
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 14);
    EXPECT_EQ(manager_.GetExpression(expr_id)->result.status, ResultStatus::kSuccess);
}

TEST_F(EquationManagerTest, ExpressionFailureStaysDirtyThenRecovers)
{
    // "X" is undefined at registration: the expression fails and stays dirty.
    // Defining X and updating must recover it.
    ObjectId expr_id = manager_.AddExpression("X+1");
    const Expression *expr = manager_.GetExpression(expr_id);
    ASSERT_NE(expr, nullptr);

    manager_.Update();
    EXPECT_EQ(expr->result.status, ResultStatus::kError);

    std::string expr_node_name;
    manager_.graph().Traversal([&](const std::string &node_name) {
        if (!manager_.IsEquationExist(node_name))
        {
            expr_node_name = node_name;
        }
    });
    ASSERT_FALSE(expr_node_name.empty());
    EXPECT_TRUE(manager_.graph().GetNode(expr_node_name)->dirty_flag());

    manager_.AddEquation("X", "2");
    manager_.Update();
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 3);
    EXPECT_FALSE(manager_.graph().GetNode(expr_node_name)->dirty_flag());
}

TEST_F(EquationManagerTest, ExpressionRemove)
{
    ObjectId expr_id = manager_.AddExpression("A+1");
    EXPECT_TRUE(manager_.IsExpressionExist(expr_id));
    EXPECT_EQ(manager_.GetExpressionIds().size(), 1u);

    manager_.RemoveExpression(expr_id);
    EXPECT_FALSE(manager_.IsExpressionExist(expr_id));
    EXPECT_TRUE(manager_.GetExpressionIds().empty());
    EXPECT_TRUE(manager_.graph().TopologicalSort().empty());
}

TEST_F(EquationManagerTest, UpdateExpressionNotFoundThrows)
{
    ObjectId bogus;  // default-constructed (nil) -> not registered
    EXPECT_THROW(manager_.UpdateExpression(bogus), EquationException);
}

// ============================================================================
// External input symbols (外部输入)
// ============================================================================

TEST_F(EquationManagerTest, ExternalInputRegisterConflicts)
{
    EXPECT_TRUE(manager_.AddExternalInput("c"));
    EXPECT_TRUE(manager_.IsExternalInput("c"));
    EXPECT_THAT(manager_.GetExternalInputNames(), testing::Contains("c"));

    EXPECT_FALSE(manager_.AddExternalInput("c"));
    manager_.AddEquation("a", "1");
    EXPECT_FALSE(manager_.AddExternalInput("a"));

    manager_.RemoveExternalInput("c");
    EXPECT_FALSE(manager_.IsExternalInput("c"));
    EXPECT_THAT(manager_.GetExternalInputNames(), testing::Not(testing::Contains("c")));
}

TEST_F(EquationManagerTest, ExternalInputRegisteredBeforeEquation)
{
    EXPECT_TRUE(manager_.AddExternalInput("c"));
    manager_.AddEquation("x", "c*2");
    manager_.Update();

    // "c" is not in the env -> x is an error until the host provides a value.
    EXPECT_EQ(manager_.GetEquation("x")->status, ResultStatus::kError);

    // The host owns the value (env Define); InvalidateExternalInputs marks the
    // input dirty and recomputes the dependents immediately.
    manager_.environment().Define("c", rel::Value::Integer(3));
    manager_.InvalidateExternalInputs({"c"});
    EXPECT_EQ(GetInt(manager_, "x"), 6);
}

TEST_F(EquationManagerTest, ExternalInputRegisteredAfterEquation)
{
    manager_.AddEquation("x", "c*2");
    EXPECT_TRUE(manager_.AddExternalInput("c"));
    manager_.Update();

    EXPECT_EQ(manager_.GetEquation("x")->status, ResultStatus::kError);

    manager_.environment().Define("c", rel::Value::Integer(4));
    manager_.InvalidateExternalInputs({"c"});
    EXPECT_EQ(GetInt(manager_, "x"), 8);
}

TEST_F(EquationManagerTest, ExternalInputUnknownNameIsIgnored)
{
    manager_.AddExternalInput("c");
    manager_.AddEquation("x", "c+1");
    manager_.environment().Define("c", rel::Value::Integer(10));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "x"), 11);

    // A name with no graph node is ignored: it neither invalidates anything
    // nor adds update work.  With no pending dirty nodes, nothing is
    // recomputed -- x keeps its previous value even though "c" changed.
    manager_.environment().Define("c", rel::Value::Integer(20));
    manager_.InvalidateExternalInputs({"bogus"});
    EXPECT_EQ(GetInt(manager_, "x"), 11);  // unchanged

    // Invalidating the real input recomputes the dependent with the new value.
    manager_.InvalidateExternalInputs({"c"});
    EXPECT_EQ(GetInt(manager_, "x"), 21);
}

TEST_F(EquationManagerTest, ExternalInputPropagatesToExpressions)
{
    manager_.AddExternalInput("c");
    manager_.AddEquation("x", "c+1");
    ObjectId expr_id = manager_.AddExpression("x*2");
    manager_.Update();
    EXPECT_EQ(manager_.GetEquation("x")->status, ResultStatus::kError);

    manager_.environment().Define("c", rel::Value::Integer(4));
    manager_.InvalidateExternalInputs({"c"});
    EXPECT_EQ(GetInt(manager_, "x"), 5);
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(expr_id)), 10);
}

// ============================================================================
// Broken objects: created, but NOT registered on the dependency graph
// ============================================================================

TEST_F(EquationManagerTest, AddEquationWithSyntaxErrorIsCreatedButNotRegistered)
{
    // A syntax error no longer throws: the equation is created (so the user
    // keeps what they typed) but stays out of the graph.
    const ObjectId id = manager_.AddEquation("A", "1 +");

    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_EQ(manager_.GetEquation("A")->content, "1 +");
    EXPECT_EQ(manager_.GetEquation("A")->status, ResultStatus::kError);
    EXPECT_FALSE(manager_.GetEquation("A")->message.empty());

    EXPECT_FALSE(manager_.IsEquationRegistered("A"));
    EXPECT_FALSE(manager_.IsEquationRegistered(id));
    EXPECT_EQ(manager_.graph().GetNode("A"), nullptr);

    // The graph stays sortable (a broken node would break topological order).
    manager_.Update();
    EXPECT_FALSE(manager_.HasVariable("A"));
}

TEST_F(EquationManagerTest, AddEquationWithCycleIsCreatedButNotRegistered)
{
    manager_.AddEquation("A", "B");
    // "B = A" closes a cycle: B is created but not put on the graph, so A
    // (and everything else) stays topologically sortable.
    const ObjectId id_b = manager_.AddEquation("B", "A");

    EXPECT_TRUE(manager_.IsEquationExist("B"));
    EXPECT_EQ(manager_.GetEquation("B")->content, "A");
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
    EXPECT_FALSE(manager_.IsEquationRegistered(id_b));

    // A is untouched and still registered.
    EXPECT_TRUE(manager_.IsEquationRegistered("A"));
    EXPECT_FALSE(manager_.graph().TopologicalSort().empty());

    manager_.Update();
    EXPECT_EQ(manager_.GetEquation("A")->status, ResultStatus::kError);  // B unbound
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
}

TEST_F(EquationManagerTest, EditEquationToCycleDetachesAndHeals)
{
    manager_.AddEquation("A", "1");
    const ObjectId id_b = manager_.AddEquation("B", "A");
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "B"), 1);
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));

    // "A = B" would close the cycle -> A keeps the new content but is detached.
    manager_.EditEquation(manager_.GetEquation("A")->id, "B");
    EXPECT_EQ(manager_.GetEquation("A")->content, "B");
    EXPECT_EQ(manager_.GetEquation("A")->status, ResultStatus::kError);
    EXPECT_FALSE(manager_.IsEquationRegistered("A"));
    // B is still registered (it was never the problem).
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));

    // Heal: editing A back to a literal re-registers it automatically.
    manager_.EditEquation(manager_.GetEquation("A")->id, "7");
    EXPECT_TRUE(manager_.IsEquationRegistered("A"));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "A"), 7);
    EXPECT_EQ(GetInt(manager_, "B"), 7);
}

TEST_F(EquationManagerTest, EditEquationToSyntaxErrorDetachesPreviousNode)
{
    manager_.AddEquation("A", "1");
    manager_.AddEquation("B", "A");
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "B"), 1);

    // Editing B to garbage must remove B's OLD graph node (not keep it).
    manager_.EditEquation(manager_.GetEquation("B")->id, "A +");
    EXPECT_EQ(manager_.GetEquation("B")->content, "A +");
    EXPECT_EQ(manager_.GetEquation("B")->status, ResultStatus::kError);
    EXPECT_FALSE(manager_.IsEquationRegistered("B"));
    EXPECT_FALSE(manager_.HasVariable("B"));

    // Fixing it re-registers B and it computes again.
    manager_.EditEquation(manager_.GetEquation("B")->id, "A * 3");
    EXPECT_TRUE(manager_.IsEquationRegistered("B"));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "B"), 3);
}

TEST_F(EquationManagerTest, DetachedEquationHealsWhenDependencyAppears)
{
    // "B = A" with A missing: B is created but detached (A is not a node yet,
    // so this is not a cycle -- it registers fine).  Use a real cycle instead:
    manager_.AddEquation("A", "B");
    const ObjectId id_b = manager_.AddEquation("B", "A");
    EXPECT_FALSE(manager_.IsEquationRegistered(id_b));

    // Breaking the cycle from the other side heals B automatically.
    manager_.EditEquation(manager_.GetEquation("A")->id, "5");
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "A"), 5);
    EXPECT_EQ(GetInt(manager_, "B"), 5);
}

TEST_F(EquationManagerTest, DetachedEquationHealsOnRemoveAndOnUpdate)
{
    manager_.AddEquation("A", "B");
    const ObjectId id_b = manager_.AddEquation("B", "A");
    EXPECT_FALSE(manager_.IsEquationRegistered(id_b));

    // Removing A breaks the cycle -> B heals.
    manager_.RemoveEquation("A");
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));

    // Re-create the cycle: now A is the one that cannot join.
    const ObjectId id_a = manager_.AddEquation("A", "B");
    EXPECT_FALSE(manager_.IsEquationRegistered(id_a));
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));

    // Heal A -> both are on the graph again.
    manager_.EditEquation(id_a, "2");
    EXPECT_TRUE(manager_.IsEquationRegistered(id_a));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "A"), 2);
    EXPECT_EQ(GetInt(manager_, "B"), 2);
}

TEST_F(EquationManagerTest, RenameKeepsDetachedEquationDetached)
{
    manager_.AddEquation("A", "1");
    const ObjectId id_b = manager_.AddEquation("B", "A");
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "B"), 1);

    // Break B with a syntax error: it keeps the content but is detached.
    manager_.EditEquation(id_b, "A +");
    EXPECT_FALSE(manager_.IsEquationRegistered(id_b));

    // Renaming a detached equation works and keeps it detached (the content
    // is unchanged, so it is still broken).
    manager_.RenameEquation(id_b, "C");
    EXPECT_TRUE(manager_.IsEquationExist("C"));
    EXPECT_FALSE(manager_.IsEquationExist("B"));
    EXPECT_EQ(manager_.GetEquation("C")->id, id_b);   // same identity
    EXPECT_FALSE(manager_.IsEquationRegistered("C"));

    // Heal: C becomes valid -> registered again.
    manager_.EditEquation(manager_.GetEquation("C")->id, "9");
    EXPECT_TRUE(manager_.IsEquationRegistered("C"));
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "C"), 9);
}

TEST_F(EquationManagerTest, RenameBreakingACycleHealsTheOtherSide)
{
    // A = B, B = A: B cannot join the graph (cycle).
    manager_.AddEquation("A", "B");
    const ObjectId id_b = manager_.AddEquation("B", "A");
    EXPECT_FALSE(manager_.IsEquationRegistered(id_b));

    // Renaming A drops the "A" name out of B's content's reach, so the cycle
    // is gone and B heals automatically.
    manager_.RenameEquation(manager_.GetEquation("A")->id, "C");
    EXPECT_TRUE(manager_.IsEquationRegistered(id_b));
}

TEST_F(EquationManagerTest, AddExpressionWithSyntaxErrorIsCreatedButNotRegistered)
{
    const ObjectId id = manager_.AddExpression("1 +");

    EXPECT_TRUE(manager_.IsExpressionExist(id));
    EXPECT_EQ(manager_.GetExpression(id)->content, "1 +");
    EXPECT_EQ(manager_.GetExpression(id)->result.status, ResultStatus::kError);
    EXPECT_FALSE(manager_.GetExpression(id)->result.message.empty());
    EXPECT_FALSE(manager_.IsExpressionRegistered(id));

    // The graph stays empty / sortable.
    EXPECT_TRUE(manager_.graph().TopologicalSort().empty());
}

TEST_F(EquationManagerTest, EditExpressionKeepsIdAndDetachesOnError)
{
    manager_.AddEquation("A", "1");
    manager_.Update();

    const ObjectId id = manager_.AddExpression("A + 1");
    manager_.UpdateExpression(id);
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(id)), 2);
    EXPECT_TRUE(manager_.IsExpressionRegistered(id));

    // Edit in place: same id, new content.
    manager_.EditExpression(id, "A * 10");
    EXPECT_TRUE(manager_.IsExpressionExist(id));
    EXPECT_EQ(manager_.GetExpression(id)->content, "A * 10");
    manager_.UpdateExpression(id);
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(id)), 10);

    // Edit to garbage: id and content are kept, the node is detached.
    manager_.EditExpression(id, "A *");
    EXPECT_TRUE(manager_.IsExpressionExist(id));
    EXPECT_EQ(manager_.GetExpression(id)->content, "A *");
    EXPECT_EQ(manager_.GetExpression(id)->result.status, ResultStatus::kError);
    EXPECT_FALSE(manager_.IsExpressionRegistered(id));

    // Fixing it re-registers the expression (same id).
    manager_.EditExpression(id, "A * 3");
    EXPECT_TRUE(manager_.IsExpressionRegistered(id));
    manager_.UpdateExpression(id);
    EXPECT_EQ(AsScalar<int>(manager_.GetExpressionValue(id)), 3);
}

TEST_F(EquationManagerTest, EditExpressionUnknownIdThrows)
{
    const ObjectId bogus = boost::uuids::random_generator()();
    EXPECT_THROW(manager_.EditExpression(bogus, "1"), EquationException);
}

TEST_F(EquationManagerTest, IsValidEquationIdentifier)
{
    // Identifier syntax: letter / underscore first, then alphanumeric / underscore.
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("x"));
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("A"));
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("_x"));
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("x1"));
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("x_1"));
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("snr_out"));

    // Does not start with a letter / underscore.
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("1x"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("1"));

    // Contains characters outside [A-Za-z0-9_].
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("a b"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("a-b"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("a.b"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("a+b"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier(""));

    // REL builtin (constant / function) names are not usable.
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("pi"));
    EXPECT_FALSE(EquationManager::IsValidEquationIdentifier("sin"));
}

TEST_F(EquationManagerTest, IsValidEquationName)
{
    // Available: valid identifier, not reserved, not taken.
    EXPECT_TRUE(manager_.IsValidEquationName("x"));
    EXPECT_TRUE(manager_.IsValidEquationName("_x"));

    // Invalid identifier / reserved name.
    EXPECT_FALSE(manager_.IsValidEquationName("1x"));
    EXPECT_FALSE(manager_.IsValidEquationName("a b"));
    EXPECT_FALSE(manager_.IsValidEquationName("pi"));

    // Taken by an existing equation.
    manager_.AddEquation("A", "1");
    EXPECT_FALSE(manager_.IsValidEquationName("A"));
    EXPECT_TRUE(manager_.IsValidEquationName("B"));

    // The identifier remains valid even when the name is taken -- the
    // two checks are independent.
    EXPECT_TRUE(EquationManager::IsValidEquationIdentifier("A"));
}

TEST_F(EquationManagerTest, ReservedBuiltinNameIsRejected)
{
    // A REL builtin constant / function can never be bound in the environment
    // (Environment::Define() throws), so the equation must be rejected up
    // front -- not created and then fail on every Update().
    EXPECT_TRUE(EquationManager::IsReservedName("pi"));
    EXPECT_TRUE(EquationManager::IsReservedName("PI"));
    EXPECT_TRUE(EquationManager::IsReservedName("e"));
    EXPECT_TRUE(EquationManager::IsReservedName("sin"));
    EXPECT_FALSE(EquationManager::IsReservedName("my_pi"));
    EXPECT_FALSE(EquationManager::IsReservedName("x"));

    try
    {
        manager_.AddEquation("pi", "3");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNameReserved);
        EXPECT_EQ(e.equation_name(), "pi");
    }
    // Nothing was created, so nothing shows up anywhere.
    EXPECT_FALSE(manager_.IsEquationExist("pi"));
    EXPECT_TRUE(manager_.GetEquationNames().empty());
    EXPECT_TRUE(manager_.graph().TopologicalSort().empty());

    // A function name is reserved too.
    try
    {
        manager_.AddEquation("sin", "1");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNameReserved);
    }
    EXPECT_FALSE(manager_.IsEquationExist("sin"));
}

TEST_F(EquationManagerTest, RenameToReservedBuiltinNameIsRejected)
{
    const ObjectId id = manager_.AddEquation("A", "1");
    manager_.Update();
    EXPECT_EQ(GetInt(manager_, "A"), 1);

    try
    {
        manager_.RenameEquation(id, "pi");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNameReserved);
    }
    // The rename was rejected as a whole: A is untouched and still bound.
    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_FALSE(manager_.IsEquationExist("pi"));
    EXPECT_EQ(manager_.GetEquation("A")->id, id);
    EXPECT_TRUE(manager_.IsEquationRegistered("A"));
    EXPECT_EQ(GetInt(manager_, "A"), 1);

    // Same for the combined edit (name + content).
    try
    {
        manager_.EditEquation(id, "e", "2");
        FAIL();
    }
    catch (const EquationException &e)
    {
        EXPECT_EQ(e.error_code(), EquationException::ErrorCode::kEquationNameReserved);
    }
    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_EQ(manager_.GetEquation("A")->content, "1");
}

TEST_F(EquationManagerTest, DetachedObjectsArePersisted)
{
    // A broken equation is still part of the project state (so the user does
    // not lose it on save/reload).
    manager_.AddEquation("A", "1 +");
    manager_.AddExpression("2 *");

    const std::string path = "detached_state_test.json";
    manager_.SaveToFile(path);
    manager_.Reset();
    manager_.LoadFromFile(path);
    std::remove(path.c_str());

    EXPECT_TRUE(manager_.IsEquationExist("A"));
    EXPECT_EQ(manager_.GetEquation("A")->content, "1 +");
    EXPECT_FALSE(manager_.IsEquationRegistered("A"));
    EXPECT_EQ(manager_.GetExpressionIds().size(), 1u);
}
