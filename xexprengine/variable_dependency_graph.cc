#include "variable_dependency_graph.h"

namespace xexprengine
{
    void ExprDependencyGraph::AddNode(const std::string &name)
    {
        if (!node_map_.count(name))
        {
            node_map_[name] = Node{};
        }

        if (node_dependency_edge_cache_.count(name))
        {
            for (const auto &edge : node_dependency_edge_cache_[name])
            {
                node_map_[name].active_dependencies.insert(edge.second);
            }
        }

        if (node_dependent_edge_cache_.count(name))
        {
            for (const auto &edge : node_dependent_edge_cache_[name])
            {
                node_map_[name].active_dependents.insert(edge.first);
            }
        }

        // check cycle
        if (CheckCycle(name))
        {
            // handle cycle
        }
    }

    void ExprDependencyGraph::RemoveNode(const std::string &name)
    {
        if (node_map_.count(name))
        {
            ClearNodeEdge(name);
            node_map_.erase(name);
        }

        if (node_dependency_edge_cache_.count(name))
        {
            for (const auto &edge : node_dependency_edge_cache_[name])
            {
                if(node_map_.count(edge.second))
                {
                    node_map_[edge.second].active_dependents.erase(name);
                }
            }
        }

        if (node_dependent_edge_cache_.count(name))
        {
            for (const auto &edge : node_dependent_edge_cache_[name])
            {
                if(node_map_.count(edge.first))
                {
                    node_map_[edge.first].active_dependencies.erase(name);
                }
            }
        }
    }

    void ExprDependencyGraph::ClearNodeEdge(const std::string &name)
    {
        if (node_map_.count(name))
        {
            node_map_[name].active_dependencies.clear();
        }

        if (node_dependency_edge_cache_.count(name))
        {
            for (const auto &edge : node_dependency_edge_cache_[name])
            {
                edge_set_.erase(edge);
            }
            node_dependency_edge_cache_.erase(name);
        }
    }

    void ExprDependencyGraph::RemoveEdge(const Edge &edge)
    {
        edge_set_.erase(edge);
        if (node_map_.count(edge.first))
        {
            node_map_[edge.first].active_dependents.erase(edge.second);
        }
        if (node_map_.count(edge.second))
        {
            node_map_[edge.second].active_dependencies.erase(edge.first);
        }

        node_dependency_edge_cache_[edge.first].erase(edge);
        node_dependent_edge_cache_[edge.second].erase(edge);
    }
    
    bool ExprDependencyGraph::CheckCycle(const std::string &node)
    {
        std::unordered_set<std::string> visited;
        std::unordered_set<std::string> rec_stack;
        return CheckCycleUtil(node, visited, rec_stack);
    }
    
    bool ExprDependencyGraph::CheckCycleUtil(const std::string &node, std::unordered_set<std::string> &visited,
                          std::unordered_set<std::string> &rec_stack)
    {
        if (rec_stack.count(node))
            return true;

        if (visited.count(node))
            return false;

        visited.insert(node);
        rec_stack.insert(node);

        if (node_map_.count(node))
        {
            for (const auto &dep : node_map_[node].active_dependencies)
            {
                if (CheckCycleUtil(dep, visited, rec_stack))
                    return true;
            }
        }

        rec_stack.erase(node);
        return false;
    }

    void ExprDependencyGraph::AddEdge(const Edge &edge)
    {
        edge_set_.insert(edge);
        if (node_map_.count(edge.first))
        {
            node_map_[edge.first].active_dependents.insert(edge.second);
        }
        if (node_map_.count(edge.second))
        {
            node_map_[edge.second].active_dependencies.insert(edge.first);
        }

        node_dependency_edge_cache_[edge.first].insert(edge);
        node_dependent_edge_cache_[edge.second].insert(edge);

        // check cycle
        if (CheckCycle(edge.first))
        {
            // handle cycle
        }
    }
}