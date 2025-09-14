#include "expr_context.h"
#include "dependency_graph.h"
#include "event_stamp.h"
#include "expr_common.h"
#include "variable.h"
#include <memory>
#include <string>
#include <utility>

using namespace xexprengine;

ExprContext::ExprContext()
    : graph_(std::unique_ptr<DependencyGraph>(new DependencyGraph())),
      evaluate_callback_(nullptr),
      parse_callback_(nullptr)
{
}

Variable *ExprContext::GetVariable(const std::string &var_name) const
{
    if (IsVariableExist(var_name) == true)
    {
        return variable_map_.at(var_name).get();
    }
    return nullptr;
}

bool ExprContext::AddVariable(std::unique_ptr<Variable> var)
{
    std::string var_name = var->name();
    if (IsVariableExist(var_name) == false)
    {
        DependencyGraph::BatchUpdateGuard guard(graph_.get());
        graph_->AddNode(var_name);
        variable_map_.insert({var_name, std::move(var)});
        UpdateVariableDependencies(var_name);
        graph_->InvalidateNode(var_name);
        guard.commit();
        return true;
    }
    return false;
}

bool ExprContext::AddVariables(std::vector<std::unique_ptr<Variable>> var_list)
{
    bool res = true;
    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    for (std::unique_ptr<Variable> &var : var_list)
    {
        res &= AddVariable(std::move(var));
    }
    guard.commit();
    return res;
}

void ExprContext::SetValue(const std::string &var_name, const Value &value)
{
    auto var = VariableFactory::CreateVariable(var_name, value);
    SetVariable(var_name, std::move(var));
}

void ExprContext::SetExpression(const std::string &var_name, const std::string &expression)
{
    auto var = VariableFactory::CreateVariable(var_name, expression);
    SetVariable(var_name, std::move(var));
}

void ExprContext::SetVariable(const std::string &var_name, std::unique_ptr<Variable> variable)
{
    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    if (IsVariableExist(var_name) == false)
    {
        AddVariable(std::move(variable));
    }
    else
    {
        variable_map_[var_name] = std::move(variable);
        UpdateVariableDependencies(var_name);
        graph_->InvalidateNode(var_name);
    }
    guard.commit();
}

bool ExprContext::RemoveVariable(const std::string &var_name)
{
    if (IsVariableExist(var_name) == true)
    {
        // make sure all dependent node dirty
        graph_->InvalidateNode(var_name);
        graph_->RemoveNode(var_name);
        variable_map_.erase(var_name);
        RemoveContextValue(var_name);
        return true;
    }
    return false;
}

bool ExprContext::RemoveVariables(const std::vector<std::string> &var_name_list)
{
    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    bool res = true;
    for (const std::string &var_name : var_name_list)
    {
        res &= RemoveVariable(var_name);
    }
    guard.commit();
    return res;
}

bool ExprContext::RenameVariable(const std::string &old_name, const std::string &new_name)
{
    if (IsVariableExist(old_name) == true && IsVariableExist(new_name) == false)
    {
        // process graph
        DependencyGraph::BatchUpdateGuard guard(graph_.get());
        graph_->InvalidateNode(old_name);
        graph_->RemoveNode(old_name);
        graph_->AddNode(new_name);
        graph_->InvalidateNode(new_name);
        auto old_edge_iterator = graph_->GetEdgesByFrom(old_name);
        std::vector<DependencyGraph::Edge> old_edges;
        std::vector<DependencyGraph::Edge> new_edges;
        for (auto it = old_edge_iterator.first; it != old_edge_iterator.second; it++)
        {
            old_edges.push_back(*it);
            new_edges.push_back({new_name, it->to()});
        }
        graph_->RemoveEdges(old_edges);
        graph_->AddEdges(new_edges);
        guard.commit();

        // process variable map
        std::unique_ptr<Variable> origin_var = std::move(variable_map_[old_name]);
        variable_map_.erase(old_name);
        origin_var->set_name(new_name);
        variable_map_.insert({new_name, std::move(origin_var)});

        // proces context
        Value old_value = GetContextValue(old_name);
        RemoveContextValue(old_name);
        SetContextValue(new_name, old_value);
        return true;
    }
    return false;
}

bool ExprContext::UpdateVariableDependencies(const std::string &var_name)
{
    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    if (IsVariableExist(var_name) == false)
    {
        return false;
    }

    // remove origin dependencies
    const DependencyGraph::Node *node = graph_->GetNode(var_name);
    const DependencyGraph::EdgeContainer::RangeByFrom edges = graph_->GetEdgesByFrom(var_name);
    std::vector<DependencyGraph::Edge> edges_to_remove;
    for (auto it = edges.first; it != edges.second; it++)
    {
        edges_to_remove.push_back(*it);
    }
    graph_->RemoveEdges(edges_to_remove);

    // add new dependencies
    Variable *var = GetVariable(var_name);
    if (var->GetType() == Variable::Type::Expr)
    {
        ExprVariable *expr_var = var->As<ExprVariable>();
        if (parse_callback_ == nullptr)
        {
            return false;
        }
        ParseResult result = parse_callback_(expr_var->expression());
        for (const std::string &dep_var : result.variables)
        {
            graph_->AddEdge({var_name, dep_var});
        }
    }
    guard.commit();
    return true;
}

bool ExprContext::IsVariableDependencyEntire(const std::string &var_name) const
{
    if (IsVariableExist(var_name) == false)
    {
        return false;
    }
    const DependencyGraph::Node *node = graph_->GetNode(var_name);
    DependencyGraph::EdgeContainer::RangeByFrom edges = graph_->GetEdgesByFrom(var_name);
    if (node->dependencies().size() != std::distance(edges.first, edges.second))
    {
        return false;
    }
    return true;
}

bool ExprContext::IsVariableExist(const std::string &var_name) const
{
    return graph_->IsNodeExist(var_name) && variable_map_.count(var_name);
}

void ExprContext::Reset()
{
    graph_->Reset();
    variable_map_.clear();
    ClearContextValue();
}

bool ExprContext::UpdateVariable(const std::string &var_name)
{
    if (!IsVariableExist(var_name))
    {
        return false;
    }

    // check node dirty
    DependencyGraph::Node *node = graph_->GetNode(var_name);
    Variable *var = GetVariable(var_name);

    bool is_should_update = false;

    if (node->dirty_flag() == true)
    {
        is_should_update = true;
    }
    else
    {
        if (var->GetType() == Variable::Type::Expr)
        {
            bool is_entire = IsVariableDependencyEntire(var_name);
            if (!is_entire)
            {
                ExprVariable *expr_var = var->As<ExprVariable>();
                expr_var->set_error_code(ErrorCode::UnknownVariable);
                expr_var->set_error_message("Variable dependency is not entire");
                return false;
            }
            else
            {
                EventStamp max_event_stamp;
                for (const std::string &dependency : node->dependencies())
                {
                    const DependencyGraph::Node *dep_node = graph_->GetNode(dependency);
                    if (dep_node && dep_node->event_stamp() > max_event_stamp)
                    {
                        max_event_stamp = dep_node->event_stamp();
                    }
                }
                if (node->event_stamp() <= max_event_stamp)
                {
                    is_should_update = true;
                }
            }
        }
    }

    if (!is_should_update)
    {
        return false;
    }

    // node is dirty and out-of-date
    if (var->GetType() == Variable::Type::Expr)
    {
        ExprVariable *expr_var = var->As<ExprVariable>();
        if (evaluate_callback_ == nullptr)
        {
            node->set_dirty_flag(false);
            return false;
        }
        EvalResult result = evaluate_callback_(expr_var->expression(), this);
        if (result.value != expr_var->cached_value())
        {
            SetContextValue(var_name, result.value);
            expr_var->set_cached_value(result.value);
            expr_var->set_error_code(result.error_code);
            expr_var->set_error_message(result.error_message);
            graph_->UpdateNodeEventStamp(var_name);
        }
        else
        {
            expr_var->set_error_code(result.error_code);
            expr_var->set_error_message(result.error_message);
        }
    }
    else if (var->GetType() == Variable::Type::Raw)
    {
        RawVariable *raw_var = var->As<RawVariable>();
        if (raw_var->value() != raw_var->cached_value())
        {
            SetContextValue(var_name, raw_var->value());
            raw_var->set_cached_value(raw_var->value());
            graph_->UpdateNodeEventStamp(var_name);
        }
    }
    node->set_dirty_flag(false);
    return true;
}

void ExprContext::Update()
{
    graph_->Traversal([&](const std::string &var_name) { UpdateVariable(var_name); });
}