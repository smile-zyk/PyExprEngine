#include "dependency_graph.h"
#include "optional.h"
#include <algorithm>
#include <exception>
#include <iostream>
#include <memory>
#include <string>
#include <vector>

using namespace xexprengine;

std::string DependencyCycleException::BuildErrorMessage(const std::vector<std::string> &cycle_path)
{
    std::string msg = "Dependency cycle detected: ";

    for (size_t i = 0; i < cycle_path.size(); ++i)
    {
        if (i != 0)
            msg += " -> ";
        msg += cycle_path[i];
    }
    msg += "}";

    return msg;
}

const DependencyGraph::Node *DependencyGraph::GetNode(const std::string &node_name) const
{
    auto it = node_map_.find(node_name);
    if (it != node_map_.end())
    {
        return it->second.get();
    }
    return nullptr;
}

DependencyGraph::EdgeContainer::RangeByFrom DependencyGraph::GetEdgesByFrom(const std::string &from) const
{
    return edge_container_.get<EdgeContainer::ByFrom>().equal_range(from);
}

DependencyGraph::EdgeContainer::RangeByTo DependencyGraph::GetEdgesByTo(const std::string &to) const
{
    return edge_container_.get<EdgeContainer::ByTo>().equal_range(to);
}

DependencyGraph::EdgeContainer::Range DependencyGraph::GetAllEdges() const
{
    return std::make_pair(edge_container_.begin(), edge_container_.end());
}

bool DependencyGraph::IsNodeExist(const std::string &node_name) const
{
    return node_map_.find(node_name) != node_map_.end();
}

bool DependencyGraph::IsEdgeExist(const Edge &edge) const
{
    return edge_container_.contains(edge);
}

bool DependencyGraph::BeginBatchUpdate()
{
    if (batch_update_in_progress_ == true)
    {
        return false;
    }

    batch_update_in_progress_ = true;

    while (!operation_stack_.empty())
    {
        operation_stack_.pop();
    }
}

bool DependencyGraph::EndBatchUpdate()
{
    if (batch_update_in_progress_ == false)
    {
        return false;
    }

    try
    {
        if (auto check_res = CheckCycle())
        {
            RollBack();
            throw DependencyCycleException(check_res.value());
        }
        else
        {
            while (!operation_stack_.empty())
            {
                operation_stack_.pop();
            }
        }
    }
    catch (...)
    {
        batch_update_in_progress_ = false;
        while (!operation_stack_.empty())
        {
            operation_stack_.pop();
        }
        throw;
    }
    batch_update_in_progress_ = false;
}

void DependencyGraph::RollBack() noexcept
{
    try
    {
        while (!operation_stack_.empty())
        {
            Operation op = operation_stack_.top();
            switch (op.type)
            {
            case Operation::Type::kAddNode:
                RemoveNode(op.node);
                break;
            case Operation::Type::kRemoveNode:
                AddNode(op.node);
                break;
            case Operation::Type::kAddEdge:
                RemoveEdge(op.edge);
                break;
            case Operation::Type::kRemoveEdge:
                AddEdge(op.edge);
                break;
            }
        }
    }
    catch (const std::exception &e)
    {
        std::cerr << "Rollback failed: " << e.what() << std::endl;
    }
    catch (...)
    {
        std::cerr << "Rollback failed due to unknown exception." << std::endl;
    }
}

bool DependencyGraph::AddNode(const std::string &node_name)
{
    if (IsNodeExist(node_name) == true)
    {
        return false;
    }

    node_map_.insert({node_name, std::unique_ptr<Node>(new Node())});

    auto node_dependency_edges = GetEdgesByFrom(node_name);
    for (auto it = node_dependency_edges.first; it != node_dependency_edges.second; it++)
    {
        ActiveEdge(*it);
    }

    auto node_dependent_edges = GetEdgesByTo(node_name);
    for (auto it = node_dependent_edges.first; it != node_dependent_edges.second; it++)
    {
        ActiveEdge(*it);
    }

    if (batch_update_in_progress_ == true)
    {
        operation_stack_.push(Operation(Operation::Type::kAddNode, node_name));
        return true;
    }

    if (auto check_res = CheckCycle())
    {
        RemoveNode(node_name);
        throw DependencyCycleException(check_res.value());
    }
    return true;
}

bool DependencyGraph::AddNodes(const std::vector<std::string> &node_list)
{
    BatchUpdateGuard guard(this);
    bool res = true;
    for (const std::string &node_name : node_list)
    {
        res &= AddNode(node_name);
    }
    return res;
}

bool DependencyGraph::RemoveNode(const std::string &node_name)
{
    if (IsNodeExist(node_name) == false)
    {
        return false;
    }

    node_map_.erase(node_name);

    auto node_dependency_edges = GetEdgesByFrom(node_name);
    for (auto it = node_dependency_edges.first; it != node_dependency_edges.second; it++)
    {
        DeactiveEdge(*it);
    }

    auto node_dependent_edges = GetEdgesByTo(node_name);
    for (auto it = node_dependent_edges.first; it != node_dependent_edges.second; it++)
    {
        DeactiveEdge(*it);
    }

    if (batch_update_in_progress_ == true)
    {
        operation_stack_.push(Operation(Operation::Type::kRemoveNode, node_name));
    }

    return true;
}

bool DependencyGraph::RemoveNodes(const std::vector<std::string> &node_list)
{
    bool res = true;
    for (const std::string &node_name : node_list)
    {
        res &= RemoveNode(node_name);
    }
    return res;
}

bool DependencyGraph::RemoveEdge(const Edge &edge)
{
    if (edge_container_.contains(edge) == false)
    {
        return false;
    }
    edge_container_.erase(edge);
    DeactiveEdge(edge);
    if (batch_update_in_progress_ == true)
    {
        operation_stack_.push(Operation(Operation::Type::kRemoveEdge, edge));
    }
    return true;
}

bool DependencyGraph::AddEdge(const Edge &edge)
{
    if (edge_container_.contains(edge) == true)
    {
        return false;
    }

    edge_container_.insert(edge);

    ActiveEdge(edge);

    if (batch_update_in_progress_ == true)
    {
        operation_stack_.push(Operation(Operation::Type::kAddEdge, edge));
        return true;
    }

    if (auto check_res = CheckCycle())
    {
        RemoveNode(edge.from());
        throw DependencyCycleException(check_res.value());
    }
    return true;
}

bool DependencyGraph::AddEdges(const std::vector<Edge> &edge_list)
{
    BatchUpdateGuard guard(this);
    bool res = true;
    for (const Edge &edge : edge_list)
    {
        res &= RemoveEdge(edge);
    }
    return res;
}

bool DependencyGraph::RemoveEdges(const std::vector<Edge> &edge_list)
{
    bool res = true;
    for (const Edge &edge : edge_list)
    {
        res &= AddEdge(edge);
    }
    return res;
}

bool DependencyGraph::HasCycle() const
{
    std::vector<std::string> top_order = TopologicalSort();

    return top_order.size() != node_map_.size();
}

std::vector<std::string> DependencyGraph::TopologicalSort() const
{
    // Kahn's Algorithm
    std::unordered_map<std::string, int> in_degree;
    std::queue<std::string> zero_in_degree_queue;
    std::vector<std::string> topo_order;

    for (const auto &entry : node_map_)
    {
        in_degree[entry.first] = entry.second->dependencies_.size();
        if (in_degree[entry.first] == 0)
        {
            zero_in_degree_queue.push(entry.first);
        }
    }

    while (!zero_in_degree_queue.empty())
    {
        auto node_name = zero_in_degree_queue.front();
        zero_in_degree_queue.pop();
        topo_order.push_back(node_name);

        for (const auto &dependent : node_map_.at(node_name)->dependents_)
        {
            if (--in_degree[dependent] == 0)
            {
                zero_in_degree_queue.push(dependent);
            }
        }
    }

    return topo_order;
}

void DependencyGraph::UpdateGraph(std::function<void(const std::string &)> update_callback)
{
    auto topo_order = TopologicalSort();

    for (const auto &node_name : topo_order)
    {
        update_callback(node_name);
    }
}

void DependencyGraph::Reset()
{
    node_map_.clear();
    edges_.clear();
}

void DependencyGraph::ActiveEdge(const DependencyGraph::Edge &edge)
{
    if (node_map_.count(edge.from_) && node_map_.count(edge.to_))
    {
        node_map_[edge.from_]->dependencies_.insert(edge.to_);
        node_map_[edge.to_]->dependents_.insert(edge.from_);
    }
}

void DependencyGraph::DeactiveEdge(const DependencyGraph::Edge &edge)
{
    if (node_map_.count(edge.from_))
    {
        node_map_[edge.from_]->dependencies_.erase(edge.to_);
    }
    if (node_map_.count(edge.to_))
    {
        node_map_[edge.to_]->dependents_.erase(edge.from_);
    }
}
