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

// Test add and remove node
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

// Test add and remove edge
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

TEST_F(VariableDependencyGraphTest, RebuildConnectionAndClearNodeEdge)
{
    graph->AddEdge({"A", "B"});
    graph->AddEdge({"A", "C"});
    graph->AddEdge({"B", "C"});
    graph->AddEdge({"D", "A"});
    graph->AddEdge({"E", "A"});
    graph->AddEdge({"D", "B"});
    graph->AddEdge({"E", "C"});
    graph->AddEdge({"D", "E"});

    graph->AddNode("A");
    graph->AddNode("B");
    graph->AddNode("C");
    graph->AddNode("D");
    graph->AddNode("E");
    
    auto a_dependencies = graph->GetNodeActiveDependencies("A");
    auto a_dependents = graph->GetNodeActiveDependents("A");
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

    auto b_dependencies = graph->GetNodeActiveDependencies("B");
    auto b_dependents = graph->GetNodeActiveDependents("B");
    ASSERT_EQ(1, b_dependencies.size()); 
    EXPECT_THAT(b_dependencies, ::testing::AllOf(
        ::testing::Contains("C")
    ));
    ASSERT_EQ(2, b_dependents.size()); 
    EXPECT_THAT(b_dependents, ::testing::AllOf(
        ::testing::Contains("D"),
        ::testing::Contains("A")
    ));

    auto c_dependencies = graph->GetNodeActiveDependencies("C");
    auto c_dependents = graph->GetNodeActiveDependents("C");
    ASSERT_EQ(0, c_dependencies.size()); 
    ASSERT_EQ(3, c_dependents.size()); 
    EXPECT_THAT(c_dependents, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("E")
    ));

    auto d_dependencies = graph->GetNodeActiveDependencies("D");
    auto d_dependents = graph->GetNodeActiveDependents("D");
    ASSERT_EQ(3, d_dependencies.size()); 
    EXPECT_THAT(d_dependencies, ::testing::AllOf(
        ::testing::Contains("A"),
        ::testing::Contains("B"),
        ::testing::Contains("E")
    ));
    ASSERT_EQ(0, d_dependents.size()); 

    auto e_dependencies = graph->GetNodeActiveDependencies("E");
    auto e_dependents = graph->GetNodeActiveDependents("E");
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
    EXPECT_NO_THROW(graph->ClearNodeDependencyEdges("Nonexistent"));

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