#include "variable_dependency_graph.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>

using namespace xexprengine;

class VariableDependencyGraphTest : public ::testing::Test, public VariableDependencyGraph {
protected:
    void SetUp() override {
        Reset();
    }
};

// Test add and remove node
TEST_F(VariableDependencyGraphTest, AddRemoveNode)
{
    // Test adding a node
    AddNode("A");
    EXPECT_TRUE(IsNodeExist("A"));
    EXPECT_TRUE(GetNodeDependencies("A").empty());
    EXPECT_TRUE(GetNodeDependents("A").empty());

    // Test removing a node
    RemoveNode("A");
    EXPECT_FALSE(IsNodeExist("A"));

    AddEdge({"A", "B"});
    AddNode("B");

    auto dependents = GetNodeDependents("B");
    EXPECT_EQ(dependents.size(), 0);

    AddNode("A");
    dependents = GetNodeDependents("B");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("A") > 0);
    auto dependencies = GetNodeDependencies("A");
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_TRUE(dependencies.count("B") > 0);
}

// Test add and remove edge
TEST_F(VariableDependencyGraphTest, AddRemoveEdge)
{
    AddNode("A");
    AddNode("B");

    // Test adding an edge
    VariableDependencyGraph::Edge edge{"A", "B"};
    AddEdge(edge);
    auto dependents = GetNodeDependents("B");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("A") > 0);

    auto dependencies = GetNodeDependencies("A");
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_TRUE(dependencies.count("B") > 0);

    // Test removing an edge
    RemoveEdge(edge);
    EXPECT_TRUE(GetNodeDependents("B").empty());
    EXPECT_TRUE(GetNodeDependencies("A").empty());
}

TEST_F(VariableDependencyGraphTest, RebuildConnectionAndClearNodeEdge)
{
    AddEdge({"A", "B"});
    AddEdge({"A", "C"});
    AddEdge({"B", "C"});
    AddEdge({"D", "A"});
    AddEdge({"E", "A"});
    AddEdge({"D", "B"});
    AddEdge({"E", "C"});
    AddEdge({"D", "E"});

    AddNode("A");
    AddNode("B");
    AddNode("C");
    AddNode("D");
    AddNode("E");
    
    auto a_dependencies = GetNodeDependencies("A");
    auto a_dependents = GetNodeDependents("A");
    ASSERT_EQ(2, a_dependencies.size());
    EXPECT_THAT(a_dependencies, ::testing::AllOf(
        ::testing::Contains("B"),
        ::testing::Contains("C")
    ));

    ASSERT_EQ(2, a_dependents.size());
    EXPECT_THAT(a_dependents, ::testing::AllOf(
        ::testing::Contains("D"),
        ::testing::Contains("E")
    ));

    auto b_dependencies = GetNodeDependencies("B");
    auto b_dependents = GetNodeDependents("B");
    ASSERT_EQ(1, b_dependencies.size()); 
    EXPECT_THAT(b_dependencies, ::testing::AllOf(
        ::testing::Contains("C")
    ));
    ASSERT_EQ(2, b_dependents.size()); 
    EXPECT_THAT(b_dependents, ::testing::AllOf(
        ::testing::Contains("D"),
        ::testing::Contains("A")
    ));

    auto c_dependencies = GetNodeDependencies("C");
    auto c_dependents = GetNodeDependents("C");
    ASSERT_EQ(0, c_dependencies.size()); 
    ASSERT_EQ(3, c_dependents.size()); 
    EXPECT_THAT(c_dependents, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("E")
    ));

    auto d_dependencies = GetNodeDependencies("D");
    auto d_dependents = GetNodeDependents("D");
    ASSERT_EQ(3, d_dependencies.size()); 
    EXPECT_THAT(d_dependencies, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("E")
    ));
    ASSERT_EQ(0, d_dependents.size()); 

    auto e_dependencies = GetNodeDependencies("E");
    auto e_dependents = GetNodeDependents("E");
    ASSERT_EQ(2, e_dependencies.size()); 
    EXPECT_THAT(e_dependencies, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("C")
    ));

    ASSERT_EQ(1, e_dependents.size()); 
    EXPECT_THAT(e_dependents, ::testing::AllOf(
        ::testing::Contains("D")
    ));
}

// Test cycle detection
TEST_F(VariableDependencyGraphTest, CycleDetection) {
    AddNode("A");
    AddNode("B");
    AddNode("C");

    // Acyclic graph
    AddEdge({"A", "B"});
    AddEdge({"B", "C"});

    // Create cycle
    try {
        AddEdge({"C", "A"});
        FAIL() << "Expected DependencyCycleException";
    } catch (const DependencyCycleException& ex) {
        auto cycle = ex.GetCyclePath()[0];
        ASSERT_EQ(4, cycle.size());
        EXPECT_EQ("C", cycle[0]);
        EXPECT_EQ("A", cycle[1]);
        EXPECT_EQ("B", cycle[2]);
        EXPECT_EQ("C", cycle[3]);
    }

    // Self-cycle
    try {
        AddEdge({"A", "A"});
        FAIL() << "Expected DependencyCycleException";
    } catch (const DependencyCycleException&) {
        SUCCEED();
    }
}

// Test topological sorting
TEST_F(VariableDependencyGraphTest, TopologicalSort) {
    AddNode("A");
    AddNode("B");
    AddNode("C");
    AddNode("D");

    // Linear dependency
    AddEdge({"A", "B"});
    AddEdge({"B", "C"});
    auto order = TopologicalSort();
    ASSERT_EQ(4, order.size());
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "B"));
    EXPECT_TRUE(std::find(order.begin(), order.end(), "B") > 
                std::find(order.begin(), order.end(), "C"));

    // Forked dependency
    AddEdge({"A", "D"});
    order = TopologicalSort();
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "D"));
}

// Test dirty marking and update mechanism
TEST_F(VariableDependencyGraphTest, MakeDirtyAndUpdate) {
    AddNode("A");
    AddNode("B");
    AddNode("C");
    AddEdge({"A", "B"});
    AddEdge({"B", "C"});

    std::vector<std::string> updatedNodes;
    auto callback = [&updatedNodes](const std::string& node) {
        updatedNodes.push_back(node);
    };

    // Mark single node as dirty
    SetNodeDirty("B", true);
    UpdateGraph(callback);
    ASSERT_EQ(2, updatedNodes.size()); // A and B should be updated
    EXPECT_EQ("B", updatedNodes[0]);
    EXPECT_EQ("A", updatedNodes[1]);

    // Clear and test root node
    updatedNodes.clear();
    SetNodeDirty("C", true);
    UpdateGraph(callback);
    ASSERT_EQ(3, updatedNodes.size()); // A, B, C should all be updated
    EXPECT_EQ("C", updatedNodes[0]);
    EXPECT_EQ("B", updatedNodes[1]);
    EXPECT_EQ("A", updatedNodes[2]);
}

// Test edge cases
TEST_F(VariableDependencyGraphTest, EdgeCases) {
    // Operations on non-existent edges
    EXPECT_NO_THROW(RemoveEdge({"X", "Y"}));
    EXPECT_NO_THROW(ClearNodeDependencyEdges("Nonexistent"));

    // Operations on empty graph
    EXPECT_TRUE(TopologicalSort().empty());
}

// Test complex dependency scenarios
TEST_F(VariableDependencyGraphTest, ComplexDependencyScenario) {
    // Build complex dependency graph
    AddNode("A");
    AddNode("B");
    AddNode("C");
    AddNode("D");
    AddNode("E");

    AddEdge({"A", "B"});
    AddEdge({"A", "C"});
    AddEdge({"B", "D"});
    AddEdge({"C", "D"});
    AddEdge({"D", "E"});

    // Verify topological sort
    auto order = TopologicalSort();
    ASSERT_EQ(5, order.size());
    // A should come after B and C
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "B"));
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "C"));
    // B and C should come after D
    EXPECT_TRUE(std::find(order.begin(), order.end(), "B") > 
                std::find(order.begin(), order.end(), "D"));
    EXPECT_TRUE(std::find(order.begin(), order.end(), "C") > 
                std::find(order.begin(), order.end(), "D"));
    // D should come after E
    EXPECT_TRUE(std::find(order.begin(), order.end(), "D") > 
                std::find(order.begin(), order.end(), "E"));

    // Verify dependency propagation
    std::vector<std::string> updatedNodes;
    SetNodeDirty("E", true);
    UpdateGraph([&updatedNodes](const std::string& node) {
        updatedNodes.push_back(node);
    });
    ASSERT_EQ(5, updatedNodes.size()); // All nodes should be updated
    EXPECT_THAT(updatedNodes, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("C"),
        ::testing::Contains("D"),
        ::testing::Contains("E")
    ));

    updatedNodes.clear();
    SetNodeDirty("D", true);
    UpdateGraph([&updatedNodes](const std::string& node) {
        updatedNodes.push_back(node);
    });
    ASSERT_EQ(4, updatedNodes.size()); 
    EXPECT_THAT(updatedNodes, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("C"),
        ::testing::Contains("D")
    ));
}