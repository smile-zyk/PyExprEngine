#include <gtest/gtest.h>
#include "variable_dependency_graph.h"

using namespace xexprengine;

class VariableDependencyGraphTest : public ::testing::Test {
protected:
    VariableDependencyGraph graph;
};

TEST_F(VariableDependencyGraphTest, TestIsNodeExist) {
    EXPECT_FALSE(graph.IsNodeExist("A"));
    graph.AddNode("A");
    EXPECT_TRUE(graph.IsNodeExist("A"));
    graph.RemoveNode("A");
    EXPECT_FALSE(graph.IsNodeExist("A"));
}

TEST_F(VariableDependencyGraphTest, TestAddRemoveNode) {
    // Test adding a node
    graph.AddNode("A");
    EXPECT_TRUE(graph.IsNodeExist("A"));
    EXPECT_TRUE(graph.GetNodeActiveDependencies("A").empty());
    EXPECT_TRUE(graph.GetNodeActiveDependents("A").empty());

    // Test removing a node
    graph.RemoveNode("A");
    EXPECT_FALSE(graph.IsNodeExist("A"));
}

TEST_F(VariableDependencyGraphTest, TestAddRemoveEdge) {
    graph.AddNode("A");
    graph.AddNode("B");
    
    // Test adding an edge
    VariableDependencyGraph::Edge edge{"A", "B"};
    graph.AddEdge(edge);
    auto dependents = graph.GetNodeActiveDependents("B");
    EXPECT_EQ(dependents.size(), 1);
    EXPECT_TRUE(dependents.count("A") > 0);

    auto dependencies = graph.GetNodeActiveDependencies("A");
    EXPECT_EQ(dependencies.size(), 1);
    EXPECT_TRUE(dependencies.count("B") > 0);

    // Test removing an edge
    graph.RemoveEdge(edge);
    EXPECT_TRUE(graph.GetNodeActiveDependents("B").empty());
    EXPECT_TRUE(graph.GetNodeActiveDependencies("A").empty());
}

TEST_F(VariableDependencyGraphTest, TestClearNodeEdge) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddEdge({"A", "B"});
    graph.AddEdge({"C", "B"});

    graph.ClearNodeEdge("B");
    EXPECT_TRUE(graph.GetNodeActiveDependencies("B").empty());
    EXPECT_TRUE(graph.GetNodeActiveDependents("A").empty());
    EXPECT_TRUE(graph.GetNodeActiveDependents("C").empty());
}

TEST_F(VariableDependencyGraphTest, TestMakeNodeDirty) {
    graph.AddNode("A");
    graph.MakeNodeDirty("A");
    // Note: Since is_dirty is private in Node struct, we can't directly test it
    // This would need to be verified through other observable behavior
}

TEST_F(VariableDependencyGraphTest, TestTopologicalSortSimple) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddEdge({"A", "B"});

    auto sorted = graph.TopologicalSort();
    ASSERT_EQ(sorted.size(), 2);
    EXPECT_EQ(sorted[0], "A");
    EXPECT_EQ(sorted[1], "B");
}

TEST_F(VariableDependencyGraphTest, TestTopologicalSortComplex) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddNode("D");
    graph.AddEdge({"A", "B"});
    graph.AddEdge({"B", "C"});
    graph.AddEdge({"A", "D"});

    auto sorted = graph.TopologicalSort();
    ASSERT_EQ(sorted.size(), 4);
    // A should come before B and D
    EXPECT_LT(std::find(sorted.begin(), sorted.end(), "A"), 
              std::find(sorted.begin(), sorted.end(), "B"));
    EXPECT_LT(std::find(sorted.begin(), sorted.end(), "A"), 
              std::find(sorted.begin(), sorted.end(), "D"));
    // B should come before C
    EXPECT_LT(std::find(sorted.begin(), sorted.end(), "B"), 
              std::find(sorted.begin(), sorted.end(), "C"));
}

TEST_F(VariableDependencyGraphTest, TestCycleDetection) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddEdge({"A", "B"});
    graph.AddEdge({"B", "C"});
    graph.AddEdge({"C", "A"});

    EXPECT_THROW(graph.TopologicalSort(), DependencyCycleException);
}

TEST_F(VariableDependencyGraphTest, TestCycleExceptionDetails) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddEdge({"A", "B"});
    graph.AddEdge({"B", "C"});
    graph.AddEdge({"C", "A"});

    try {
        graph.TopologicalSort();
        FAIL() << "Expected DependencyCycleException";
    }
    catch (const DependencyCycleException& e) {
        const auto& cyclePath = e.getCyclePath();
        ASSERT_GE(cyclePath.size(), 3);
        EXPECT_EQ(cyclePath[0], "A");
        EXPECT_EQ(cyclePath[1], "B");
        EXPECT_EQ(cyclePath[2], "C");
        // Might loop back to A depending on implementation
    }
    catch (...) {
        FAIL() << "Expected DependencyCycleException";
    }
}

TEST_F(VariableDependencyGraphTest, TestMultipleDependencies) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddNode("D");
    graph.AddEdge({"A", "D"});
    graph.AddEdge({"B", "D"});
    graph.AddEdge({"C", "D"});

    auto deps = graph.GetNodeActiveDependencies("D");
    EXPECT_EQ(deps.size(), 3);
    EXPECT_TRUE(deps.count("A") > 0);
    EXPECT_TRUE(deps.count("B") > 0);
    EXPECT_TRUE(deps.count("C") > 0);

    auto sorted = graph.TopologicalSort();
    ASSERT_EQ(sorted.size(), 4);
    // D should be last
    EXPECT_EQ(sorted.back(), "D");
}

TEST_F(VariableDependencyGraphTest, TestIndependentNodes) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");

    auto sorted = graph.TopologicalSort();
    ASSERT_EQ(sorted.size(), 3);
    // Order of independent nodes is not guaranteed, just check all are present
    EXPECT_TRUE(std::find(sorted.begin(), sorted.end(), "A") != sorted.end());
    EXPECT_TRUE(std::find(sorted.begin(), sorted.end(), "B") != sorted.end());
    EXPECT_TRUE(std::find(sorted.begin(), sorted.end(), "C") != sorted.end());
}

TEST_F(VariableDependencyGraphTest, TestRemoveNodeWithEdges) {
    graph.AddNode("A");
    graph.AddNode("B");
    graph.AddNode("C");
    graph.AddEdge({"A", "B"});
    graph.AddEdge({"B", "C"});

    graph.RemoveNode("B");
    
    EXPECT_TRUE(graph.GetNodeActiveDependencies("C").empty());
    EXPECT_TRUE(graph.GetNodeActiveDependents("A").empty());
    EXPECT_FALSE(graph.IsNodeExist("B"));
}

TEST_F(VariableDependencyGraphTest, TestSelfReference) {
    graph.AddNode("A");
    graph.AddEdge({"A", "A"});

    EXPECT_THROW(graph.TopologicalSort(), DependencyCycleException);
}

TEST_F(VariableDependencyGraphTest, TestEdgeTypeUsage) {
    graph.AddNode("A");
    graph.AddNode("B");
    
    // Test using the public Edge type
    VariableDependencyGraph::Edge edge{"A", "B"};
    graph.AddEdge(edge);
    
    auto deps = graph.GetNodeActiveDependencies("B");
    EXPECT_EQ(deps.size(), 1);
    EXPECT_TRUE(deps.count("A") > 0);
    
    graph.RemoveEdge(edge);
    EXPECT_TRUE(graph.GetNodeActiveDependencies("B").empty());
}

TEST_F(VariableDependencyGraphTest, TestNonexistentNodeOperations) {
    // Operations on non-existent nodes should throw
    EXPECT_THROW(graph.GetNodeActiveDependencies("X"), std::out_of_range);
    EXPECT_THROW(graph.GetNodeActiveDependents("X"), std::out_of_range);
    EXPECT_THROW(graph.MakeNodeDirty("X"), std::out_of_range);
    EXPECT_THROW(graph.ClearNodeEdge("X"), std::out_of_range);
    
    // RemoveNode and IsNodeExist should handle non-existent nodes gracefully
    EXPECT_NO_THROW(graph.RemoveNode("X"));
    EXPECT_FALSE(graph.IsNodeExist("X"));
}

int main(int argc, char **argv) {
    testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}