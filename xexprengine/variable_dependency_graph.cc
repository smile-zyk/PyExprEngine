#include "variable_dependency_graph.h"
#include <algorithm>
#include <queue>
#include <string>
#include <vector>

using namespace xexprengine;

std::unordered_set<std::string> VariableDependencyGraph::GetNodeActiveDependencies(const std::string &var_name) const
{
    auto it = node_map_.find(var_name);
    if (it != node_map_.end())
    {
        return it->second.active_dependencies;
    }
    {
        return {};
    }
}

std::unordered_set<std::string> VariableDependencyGraph::GetNodeActiveDependents(const std::string &var_name) const
{
    auto it = node_map_.find(var_name);
    if (it != node_map_.end())
    {
        return it->second.active_dependents;
    }
    {
        return {};
    }
}

void VariableDependencyGraph::AddNode(const std::string &name)
{
    if (name.empty())
    {
        return;
    }

    if (!node_map_.count(name))
    {
        node_map_[name] = Node{};
    }

    if (node_dependency_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependency_edge_cache_[name])
        {
            node_map_[name].active_dependencies.insert(edge.second);
            if (node_map_.count(edge.second))
            {
                node_map_[edge.second].active_dependents.insert(name);
            }
        }
    }

    if (node_dependent_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependent_edge_cache_[name])
        {
            node_map_[name].active_dependents.insert(edge.first);
            if (node_map_.count(edge.first))
            {
                node_map_[edge.first].active_dependencies.insert(name);
            }
        }
    }

    if (Value res = CheckNodeCycle(name))
    {
        RemoveNode(name);
        throw DependencyCycleException(res.Cast<std::vector<std::string>>());
    }
}

void VariableDependencyGraph::RemoveNode(const std::string &name)
{
    if (node_map_.count(name))
    {
        node_map_.erase(name);
    }

    if (node_dependency_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependency_edge_cache_[name])
        {
            if (node_map_.count(edge.second))
            {
                node_map_[edge.second].active_dependents.erase(name);
            }
        }
    }

    if (node_dependent_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependent_edge_cache_[name])
        {
            if (node_map_.count(edge.first))
            {
                node_map_[edge.first].active_dependencies.erase(name);
            }
        }
    }
}

void VariableDependencyGraph::ClearNodeEdge(const std::string &name)
{
    if (node_map_.count(name))
    {
        node_map_[name].active_dependencies.clear();
    }

    if (node_dependency_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependency_edge_cache_[name])
        {
            RemoveEdge(edge);
        }
        node_dependency_edge_cache_.erase(name);
    }
}

void VariableDependencyGraph::RemoveEdge(const Edge &edge)
{
    edge_set_.erase(edge);
    if (node_map_.count(edge.first) && node_map_.count(edge.second))
    {
        node_map_[edge.first].active_dependencies.erase(edge.second);
        node_map_[edge.second].active_dependents.erase(edge.first);
    }

    if(node_dependency_edge_cache_.count(edge.first))
    {
        node_dependency_edge_cache_[edge.first].erase(edge);
    }
    if(node_dependent_edge_cache_.count(edge.second))
    {
        node_dependent_edge_cache_[edge.second].erase(edge);
    }
}

Value VariableDependencyGraph::CheckNodeCycle(const std::string &node)
{
    if (node_map_.find(node) == node_map_.end())
    {
        return Value::Null();
    }
    std::unordered_set<std::string> visited;
    std::unordered_set<std::string> recursion_stack;
    std::vector<std::string> cycle_path;
    if (CheckCycleDFS(node, visited, recursion_stack, cycle_path))
    {
        return cycle_path;
    }
    else
    {
        return Value::Null();
    }
}

bool VariableDependencyGraph::CheckCycleDFS(
    const std::string &node, std::unordered_set<std::string> &visited, std::unordered_set<std::string> &recursionStack,
    std::vector<std::string> &cyclePath
)
{
    if (!visited.count(node))
    {
        visited.insert(node);
        recursionStack.insert(node);
        cyclePath.push_back(node);

        for (const auto &dependency : node_map_[node].active_dependencies)
        {
            if (!visited.count(dependency) && CheckCycleDFS(dependency, visited, recursionStack, cyclePath))
            {
                return true;
            }
            else if (recursionStack.count(dependency))
            {
                auto it = std::find(cyclePath.begin(), cyclePath.end(), dependency);
                if (it != cyclePath.end())
                {
                    cyclePath.erase(cyclePath.begin(), it);
                }
                cyclePath.push_back(dependency);
                return true;
            }
        }

        cyclePath.pop_back();
        recursionStack.erase(node);
    }
    return false;
}

void VariableDependencyGraph::AddEdge(const Edge &edge)
{
    edge_set_.insert(edge);
    if (node_map_.count(edge.first) && node_map_.count(edge.first))
    {
        node_map_[edge.first].active_dependencies.insert(edge.second);
        node_map_[edge.second].active_dependents.insert(edge.first);
    }

    node_dependency_edge_cache_[edge.first].insert(edge);
    node_dependent_edge_cache_[edge.second].insert(edge);

    if (Value res = CheckNodeCycle(edge.first))
    {
        RemoveEdge(edge);
        throw DependencyCycleException(res.Cast<std::vector<std::string>>());
    }
}

bool VariableDependencyGraph::HasCycle()
{
    std::unordered_set<std::string> visited;
    std::vector<std::string> dummyPath;

    for (const auto &entry : node_map_)
    {
        const auto &node = entry.first;
        if (!visited.count(node))
        {
            std::unordered_set<std::string> recursionStack;
            if (CheckCycleDFS(node, visited, recursionStack, dummyPath))
            {
                return true;
            }
        }
    }
    return false;
}

std::vector<std::string> VariableDependencyGraph::TopologicalSort()
{
    if (HasCycle())
    {
        return {};
    }

    std::unordered_map<std::string, int> inDegree;
    std::vector<std::string> topoOrder;

    for (const auto &entry : node_map_)
    {
        inDegree[entry.first] = entry.second.active_dependencies.size();
    }

    std::queue<std::string> q;
    for (const auto &entry : inDegree)
    {
        if (entry.second == 0)
        {
            q.push(entry.first);
        }
    }

    // Kahn's Algorithm
    while (!q.empty())
    {
        std::string node = q.front();
        q.pop();
        topoOrder.push_back(node);

        for (const auto &dependent : node_map_[node].active_dependents)
        {
            if (--inDegree[dependent] == 0)
            {
                q.push(dependent);
            }
        }
    }

    return topoOrder;
}

bool VariableDependencyGraph::IsNodeExist(const std::string &name) const
{
    return node_map_.find(name) != node_map_.end();
}

void VariableDependencyGraph::MakeNodeDirty(const std::string &var_name)
{
    if (node_map_.count(var_name))
    {
        node_map_[var_name].is_dirty = true;
    }
}

void VariableDependencyGraph::UpdateGraph(std::function<void(const std::string &)> update_callback)
{
    std::unordered_set<std::string> processed_nodes;
    for (auto &entry : node_map_)
    {
        const std::string &node_name = entry.first;
        Node &node = entry.second;

        if (node.is_dirty)
        {
            MarkDependentsDirty(node_name, processed_nodes);
        }
    }

    for (auto &entry : node_map_)
    {
        const std::string &node_name = entry.first;
        Node &node = entry.second;

        if (node.is_dirty)
        {
            update_callback(node_name);
            node.is_dirty = false;
        }
    }
}

void VariableDependencyGraph::MarkDependentsDirty(
    const std::string &node_name, std::unordered_set<std::string> &processed_nodes
)
{
    if (processed_nodes.count(node_name))
    {
        return;
    }
    processed_nodes.insert(node_name);

    Node &node = node_map_[node_name];
    node.is_dirty = true;

    for (const auto &dependent : node.active_dependents)
    {
        MarkDependentsDirty(dependent, processed_nodes);
    }
}