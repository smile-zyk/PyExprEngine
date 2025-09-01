#include "variable_dependency_graph.h"
#include <gtest/gtest.h>
#include <gmock/gmock.h>
#include <algorithm>

using namespace xexprengine;

class VariableDependencyGraphTest : public ::testing::Test {
protected:
    void SetUp() override {
        graph = std::make_unique<VariableDependencyGraph>();
    }

    std::unique_ptr<VariableDependencyGraph> graph;
};

// Test basic node operations
TEST_F(VariableDependencyGraphTest, BasicNodeOperations) {
    // Add node
    graph->AddNode("A");
    EXPECT_TRUE(graph->IsNodeExist("A"));
    EXPECT_FALSE(graph->IsNodeExist("B"));

    // Add duplicate node
    EXPECT_NO_THROW(graph->AddNode("A"));

    // Get dependencies for non-existent node
    EXPECT_TRUE(graph->GetNodeActiveDependencies("B").empty());
    EXPECT_TRUE(graph->GetNodeActiveDependents("B").empty());

    // Remove node
    graph->RemoveNode("A");
    EXPECT_FALSE(graph->IsNodeExist("A"));
}

// Test edge operations
TEST_F(VariableDependencyGraphTest, EdgeOperations) {
    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");

    // Add edge
    graph->AddEdge({"A", "B"});
    EXPECT_TRUE(graph->GetNodeActiveDependencies("A").count("B"));
    EXPECT_TRUE(graph->GetNodeActiveDependents("B").count("A"));

    // Add duplicate edge
    EXPECT_NO_THROW(graph->AddEdge({"A", "B"}));

    // Remove edge
    graph->RemoveEdge({"A", "B"});
    EXPECT_TRUE(graph->GetNodeActiveDependencies("A").empty());
    EXPECT_TRUE(graph->GetNodeActiveDependents("B").empty());

    // Test edge cache
    graph->AddEdge({"A", "C"});
    graph->RemoveNode("C");
    graph->AddNode("C");
    EXPECT_TRUE(graph->GetNodeActiveDependencies("A").count("C")); // Should be automatically restored
}

TEST_F(VariableDependencyGraphTest, AddRemoveNode)
{
    // Test adding a node
    graph->AddNode("A");
    EXPECT_TRUE(graph->IsNodeExist("A"));
    EXPECT_TRUE(graph->GetNodeActiveDependencies("A").empty());
    EXPECT_TRUE(graph->GetNodeActiveDependents("A").empty());

    // Test removing a node
    graph->RemoveNode("A");
    EXPECT_FALSE(graph->IsNodeExist("A"));

    graph->AddEdge({"A", "B"});
    graph->AddNode("B");

    auto dependents = graph->GetNodeActiveDependents("B");
    EXPECT_EQ(dependents.size(), 0);

    graph->AddNode("A");
    dependents = graph->GetNodeActiveDependents("B");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("A") > 0);
    auto dependencies = graph->GetNodeActiveDependencies("A");
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_TRUE(dependencies.count("B") > 0);
}

TEST_F(VariableDependencyGraphTest, AddRemoveEdge)
{
    graph->AddNode("A");
    graph->AddNode("B");

    // Test adding an edge
    VariableDependencyGraph::Edge edge{"A", "B"};
    graph->AddEdge(edge);
    auto dependents = graph->GetNodeActiveDependents("B");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("A") > 0);

    auto dependencies = graph->GetNodeActiveDependencies("A");
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_TRUE(dependencies.count("B") > 0);

    // Test removing an edge
    graph->RemoveEdge(edge);
    EXPECT_TRUE(graph->GetNodeActiveDependents("B").empty());
    EXPECT_TRUE(graph->GetNodeActiveDependencies("A").empty());
}

// Test cycle detection
TEST_F(VariableDependencyGraphTest, CycleDetection) {
    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");

    // Acyclic graph
    graph->AddEdge({"A", "B"});
    graph->AddEdge({"B", "C"});

    // Create cycle
    try {
        graph->AddEdge({"C", "A"});
        FAIL() << "Expected DependencyCycleException";
    } catch (const DependencyCycleException& ex) {
        auto cycle = ex.getCyclePath();
        ASSERT_EQ(4, cycle.size());
        EXPECT_EQ("C", cycle[0]);
        EXPECT_EQ("A", cycle[1]);
        EXPECT_EQ("B", cycle[2]);
        EXPECT_EQ("C", cycle[3]);
    }

    // Self-cycle
    try {
        graph->AddEdge({"A", "A"});
        FAIL() << "Expected DependencyCycleException";
    } catch (const DependencyCycleException&) {
        SUCCEED();
    }
}

// Test topological sorting
TEST_F(VariableDependencyGraphTest, TopologicalSort) {
    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");
    graph->AddNode("D");

    // Linear dependency
    graph->AddEdge({"A", "B"});
    graph->AddEdge({"B", "C"});
    auto order = graph->TopologicalSort();
    ASSERT_EQ(4, order.size());
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "B"));
    EXPECT_TRUE(std::find(order.begin(), order.end(), "B") > 
                std::find(order.begin(), order.end(), "C"));

    // Forked dependency
    graph->AddEdge({"A", "D"});
    order = graph->TopologicalSort();
    EXPECT_TRUE(std::find(order.begin(), order.end(), "A") > 
                std::find(order.begin(), order.end(), "D"));
}

// Test dirty marking and update mechanism
TEST_F(VariableDependencyGraphTest, MakeDirtyAndUpdate) {
    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");
    graph->AddEdge({"A", "B"});
    graph->AddEdge({"B", "C"});

    std::vector<std::string> updatedNodes;
    auto callback = [&updatedNodes](const std::string& node) {
        updatedNodes.push_back(node);
    };

    // Mark single node as dirty
    graph->MakeNodeDirty("B");
    graph->UpdateGraph(callback);
    ASSERT_EQ(2, updatedNodes.size()); // A and B should be updated
    EXPECT_EQ("B", updatedNodes[0]);
    EXPECT_EQ("A", updatedNodes[1]);

    // Clear and test root node
    updatedNodes.clear();
    graph->MakeNodeDirty("C");
    graph->UpdateGraph(callback);
    ASSERT_EQ(3, updatedNodes.size()); // A, B, C should all be updated
    EXPECT_EQ("C", updatedNodes[0]);
    EXPECT_EQ("B", updatedNodes[1]);
    EXPECT_EQ("A", updatedNodes[2]);
}

// Test edge cases
TEST_F(VariableDependencyGraphTest, EdgeCases) {
    // Empty name node
    EXPECT_NO_THROW(graph->AddNode(""));
    EXPECT_FALSE(graph->IsNodeExist(""));

    // Operations on non-existent edges
    EXPECT_NO_THROW(graph->RemoveEdge({"X", "Y"}));
    EXPECT_NO_THROW(graph->ClearNodeEdge("Nonexistent"));

    // Operations on empty graph
    EXPECT_TRUE(graph->TopologicalSort().empty());
}

// Test complex dependency scenarios
TEST_F(VariableDependencyGraphTest, ComplexDependencyScenario) {
    // Build complex dependency graph
    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");
    graph->AddNode("D");
    graph->AddNode("E");

    graph->AddEdge({"A", "B"});
    graph->AddEdge({"A", "C"});
    graph->AddEdge({"B", "D"});
    graph->AddEdge({"C", "D"});
    graph->AddEdge({"D", "E"});

    // Verify topological sort
    auto order = graph->TopologicalSort();
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
    graph->MakeNodeDirty("E");
    graph->UpdateGraph([&updatedNodes](const std::string& node) {
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
    graph->MakeNodeDirty("D");
    graph->UpdateGraph([&updatedNodes](const std::string& node) {
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