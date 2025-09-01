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
            if (node_map_.count(edge.second))
            {
                node_map_[name].active_dependencies.insert(edge.second);
                node_map_[edge.second].active_dependents.insert(name);
            }
        }
    }

    if (node_dependent_edge_cache_.count(name))
    {
        for (const auto &edge : node_dependent_edge_cache_[name])
        {
            if (node_map_.count(edge.first))
            {
                node_map_[name].active_dependents.insert(edge.first);
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

    if (node_dependency_edge_cache_.count(edge.first))
    {
        node_dependency_edge_cache_[edge.first].erase(edge);
    }
    if (node_dependent_edge_cache_.count(edge.second))
    {
        node_dependent_edge_cache_[edge.second].erase(edge);
    }
}

Value VariableDependencyGraph::CheckNodeCycle(const std::string &node)
{
    if (!IsNodeExist(node))
        return Value::Null();

    if (!HasCycle())
        return Value::Null();

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
    const std::string &node, std::unordered_set<std::string> &visited,
    std::unordered_set<std::string> &current_path_set, std::vector<std::string> &cycle_path
)
{
    if (!visited.count(node))
    {
        visited.insert(node);
        current_path_set.insert(node);
        cycle_path.push_back(node);

        for (const auto &dependency : node_map_[node].active_dependencies)
        {
            if (!visited.count(dependency) && CheckCycleDFS(dependency, visited, current_path_set, cycle_path))
            {
                return true;
            }
            else if (current_path_set.count(dependency))
            {
                auto it = std::find(cycle_path.begin(), cycle_path.end(), dependency);
                if (it != cycle_path.end())
                {
                    cycle_path.erase(cycle_path.begin(), it);
                }
                cycle_path.push_back(dependency);
                return true;
            }
        }

        cycle_path.pop_back();
        current_path_set.erase(node);
    }
    return false;
}

void VariableDependencyGraph::AddEdge(const Edge &edge)
{
    edge_set_.insert(edge);
    if (node_map_.count(edge.first) && node_map_.count(edge.second))
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
    std::vector<std::string> top_order = TopologicalSort();

    return top_order.size() != node_map_.size();
}

std::vector<std::string> VariableDependencyGraph::TopologicalSort()
{
    // Kahn's Algorithm
    std::unordered_map<std::string, int> in_degree;
    std::queue<std::string> zero_in_degree_queue;
    std::vector<std::string> topo_order;

    for (const auto &entry : node_map_)
    {
        in_degree[entry.first] = entry.second.active_dependencies.size();
        if (in_degree[entry.first] == 0)
        {
            zero_in_degree_queue.push(entry.first);
        }
    }

    while (!zero_in_degree_queue.empty())
    {
        auto node = zero_in_degree_queue.front();
        zero_in_degree_queue.pop();
        topo_order.push_back(node);

        for (const auto &dependent : node_map_[node].active_dependents)
        {
            if (--in_degree[dependent] == 0)
            {
                zero_in_degree_queue.push(dependent);
            }
        }
    }

    return topo_order;
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
    if (HasCycle())
    {
        return;
    }

    std::unordered_set<std::string> processed_nodes;
    for (auto &entry : node_map_)
    {
        const std::string &node_name = entry.first;
        Node &node = entry.second;

        if (node.is_dirty)
        {
            MakeNodeDependentsDirty(node_name, processed_nodes);
        }
    }

    auto topo_order = TopologicalSort();

    for (const auto &node_name : topo_order)
    {
        if (processed_nodes.count(node_name))
        {
            update_callback(node_name);
            node_map_[node_name].is_dirty = false;
        }
    }
}

void VariableDependencyGraph::MakeNodeDependentsDirty(
    const std::string &node_name, std::unordered_set<std::string> &processed_nodes)
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
        MakeNodeDependentsDirty(dependent, processed_nodes);
    }
}