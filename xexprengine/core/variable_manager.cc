#include "variable_manager.h"
#include "core/expr_common.h"
#include "core/variable.h"
#include "event_stamp.h"
#include <boost/filesystem.hpp>
#include <boost/filesystem/operations.hpp>
#include <boost/filesystem/path.hpp>

using namespace xexprengine;

VariableManager::VariableManager(
    std::unique_ptr<ExprContext> context, EvalCallback eval_callback, ParseCallback parse_callback
) noexcept
    : graph_(std::unique_ptr<DependencyGraph>(new DependencyGraph())),
      context_(std::move(context)),
      evaluate_callback_(eval_callback),
      parse_callback_(parse_callback)
{
}

void VariableManager::Parse(RawVariable *var)
{
    graph_->AddNode(var->name());
    var->set_status(VariableStatus::kRawVar);
    var->set_error_message("");
}

void VariableManager::Parse(ExprVariable *var)
{
    ParseResult parse_result = parse_callback_(var->expression());
    if (parse_result.is_success && parse_result.type == ParseType::kExpression)
    {
        graph_->AddNode(var->name());
        const DependencyGraph::Node *node = graph_->GetNode(var->name());
        const DependencyGraph::EdgeContainer::RangeByFrom edges = graph_->GetEdgesByFrom(var->name());
        std::vector<DependencyGraph::Edge> edges_to_remove;
        for (auto it = edges.first; it != edges.second; it++)
        {
            edges_to_remove.push_back(*it);
        }
        graph_->RemoveEdges(edges_to_remove);

        for (const std::string &dep : parse_result.dependency_symbols)
        {
            graph_->AddEdge({var->name(), dep});
        }
        var->set_status(VariableStatus::kParseSuccess);
        var->set_error_message(parse_result.parse_error_message);
    }
    else
    {
        var->set_status(VariableStatus::kParseSyntaxError);
        var->set_error_message(parse_result.parse_error_message);
    }
}

void VariableManager::Parse(ImportVariable *var) {}
void VariableManager::Parse(FuncVariable *var) {}

void VariableManager::Update(RawVariable *var) {}
void VariableManager::Update(ExprVariable *var) {}
void VariableManager::Update(ImportVariable *var) {}
void VariableManager::Update(FuncVariable *var) {}

void VariableManager::Clear(RawVariable *var)
{
    RemoveNodeInGraph(var->name());
    context_->Remove(var->name());
}

void VariableManager::Clear(ExprVariable *var)
{
    RemoveNodeInGraph(var->name());
    context_->Remove(var->name());
}

void VariableManager::Clear(ImportVariable *var)
{
    for (const std::string &symbol : var->symbols())
    {
        RemoveNodeInGraph(symbol);
        context_->Remove(symbol);
    }
    RemoveNodeInGraph(var->name());
    context_->Remove(var->name());
}

void VariableManager::Clear(FuncVariable *var)
{
    RemoveNodeInGraph(var->name());
    context_->Remove(var->name());
}

const Variable *VariableManager::GetVariable(const std::string &var_name) const
{
    if (IsVariableExist(var_name) == true)
    {
        return variable_map_.at(var_name).get();
    }
    return nullptr;
}

void VariableManager::SetValue(const std::string &var_name, const Value &value)
{
    auto var = VariableFactory::CreateRawVariable(var_name, value);
    SetVariable(var_name, std::move(var));
}

void VariableManager::SetExpression(const std::string &var_name, const std::string &expression)
{
    auto var = VariableFactory::CreateExprVariable(var_name, expression);
    SetVariable(var_name, std::move(var));
}

void VariableManager::SetVariable(const std::string &var_name, std::unique_ptr<Variable> variable)
{
    // update graph
    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    bool is_variable_exist = IsVariableExist(var_name);
    if (is_variable_exist == true)
    {
        variable_map_.at(var_name)->AcceptClear(this);
    }
    variable->AcceptParse(this);
    try
    {
        guard.commit();
    }
    catch (xexprengine::DependencyCycleException e)
    {
        variable->set_status(VariableStatus::kCycleDependency);
        std::string message = "find cycle: ";
        const auto &path = e.cycle_path();
        for (int i = 0; i < path.size(); i++)
        {
            if (i != 0)
            {
                message += ", ";
            }
            message += path[i];
        }
        variable->set_error_message(message);
    }

    if (is_variable_exist)
    {
        variable_map_.erase(var_name);
    }

    graph_->InvalidateNode(var_name);
    variable_map_.insert({var_name, std::move(variable)});
}

bool VariableManager::RemoveVariable(const std::string &var_name) noexcept
{
    if (IsVariableExist(var_name) == true)
    {
        graph_->InvalidateNode(var_name);

        // clear graph
        variable_map_.at(var_name)->AcceptClear(this);

        // update variable map
        variable_map_.erase(var_name);

        return true;
    }
    return false;
}

bool VariableManager::CheckNodeDependenciesComplete(
    const std::string &node_name, std::vector<std::string> &missing_dependencies
) const
{
    if (graph_->IsNodeExist(node_name) == false)
    {
        return false;
    }
    const DependencyGraph::Node *node = graph_->GetNode(node_name);
    DependencyGraph::EdgeContainer::RangeByFrom edges = graph_->GetEdgesByFrom(node_name);
    const auto &dependencies = node->dependencies();
    if (dependencies.size() == std::distance(edges.first, edges.second))
    {
        return true;
    }
    for (auto it = edges.first; it != edges.second; it++)
    {
        if (dependencies.count(it->to()) == 0)
        {
            missing_dependencies.push_back(it->to());
        }
    }
    return false;
}

bool VariableManager::IsVariableExist(const std::string &var_name) const
{
    return graph_->IsNodeExist(var_name) && variable_map_.count(var_name);
}

void VariableManager::Reset()
{
    graph_->Reset();
    variable_map_.clear();
    context_->Clear();
}

std::string BuildMissingDepsMessage(const std::vector<std::string> &missing_deps)
{
    if (missing_deps.empty())
    {
        return "Missing dependencies: unknown";
    }

    std::string message = "Missing dependencies: ";
    for (size_t i = 0; i < missing_deps.size(); ++i)
    {
        if (i > 0)
        {
            message += ", ";
        }
        message += missing_deps[i];
    }
    return message;
}

bool VariableManager::UpdateVariableInternal(const std::string &var_name)
{
    if (!IsVariableExist(var_name) || evaluate_callback_ == nullptr)
    {
        return false;
    }

    const DependencyGraph::Node *node = graph_->GetNode(var_name);

    if (!node->dirty_flag())
    {
        return false;
    }

    auto RemoveVariableInContext = [this](const std::string &var_name) {
        if (context_->Remove(var_name))
        {
            graph_->UpdateNodeEventStamp(var_name);
        }
    };

    auto UpdateValueToContext = [this](const std::string &var_name, const Value &value) {
        if (value != context_->Get(var_name))
        {
            context_->Set(var_name, value);
            graph_->UpdateNodeEventStamp(var_name);
        }
    };

    bool needs_evaluation = true;

    if (var->status() == VariableStatus::kParseSyntaxError)
    {
        needs_evaluation = false;
        RemoveVariableInContext(var_name);
    }
    else
    {
        std::vector<std::string> missing_deps;
        if (!CheckNodeDependenciesComplete(var_name, missing_deps))
        {
            needs_evaluation = false;
            var->set_status(VariableStatus::kMissingDependency);
            var->set_error_message(BuildMissingDepsMessage(missing_deps));
            RemoveVariableInContext(var_name);
        }
        else
        {
            // Check if dependencies have newer event stamps
            EventStamp max_dep_stamp;
            for (const std::string &dep : node->dependencies())
            {
                if (const DependencyGraph::Node *dep_node = graph_->GetNode(dep))
                {
                    max_dep_stamp = std::max(max_dep_stamp, dep_node->event_stamp());
                }
            }
            needs_evaluation = (node->event_stamp() <= max_dep_stamp);
        }
    }

    if (!needs_evaluation)
    {
        node->set_dirty_flag(false);
        return false;
    }

    bool eval_success = true;

    if (var->GetType() == Variable::Type::Expr)
    {
        const std::string &expression = var->As<ExprVariable>()->expression();
        EvalResult result = evaluate_callback_(expression, context_.get());
        eval_success = (result.status == VariableStatus::kExprEvalSuccess);
        if (eval_success)
        {
            UpdateValueToContext(var_name, result.value);
        }
        else
        {
            RemoveVariableInContext(var_name);
        }
        var->set_status(result.status);
        var->set_error_message(result.eval_error_message);
    }
    else if (var->GetType() == Variable::Type::Raw)
    {
        const Value &value = var->As<RawVariable>()->value();
        UpdateValueToContext(var_name, value);
        var->set_status(VariableStatus::kRawVar);
        var->set_error_message("");
    }
    node->set_dirty_flag(false);
    return eval_success;
}

bool VariableManager::AddVariableToGraph(const Variable *var)
{
    const std::string &var_name = var->name();
    if (graph_->IsNodeExist(var_name))
    {
        return false;
    }

    DependencyGraph::BatchUpdateGuard guard(graph_.get());
    graph_->AddNode(var_name);
    if (var->GetType() == Variable::Type::Expr)
    {
        const ExprVariable *expr_var = var->As<ExprVariable>();
        const std::string &expression = expr_var->expression();
        ParseResult parse_result = parse_callback_(expression);
        UpdateNodeDependencies(var_name, parse_result.variables);
    }

    try
    {
        guard.commit();
    }
    catch (xexprengine::DependencyCycleException e)
    {
        throw;
    }
    return true;
}

bool VariableManager::RemoveNodeInGraph(const std::string &var_name) noexcept
{
    if (graph_->IsNodeExist(var_name) == false)
    {
        return false;
    }
    // for invalidate graph node to trigger dependent node update
    graph_->InvalidateNode(var_name);
    graph_->RemoveNode(var_name);
    auto edges = graph_->GetEdgesByFrom(var_name);
    std::vector<DependencyGraph::Edge> edges_to_remove;
    for (auto it = edges.first; it != edges.second; it++)
    {
        edges_to_remove.push_back(*it);
    }
    graph_->RemoveEdges(edges_to_remove);
    return true;
}

void VariableManager::Update()
{
    graph_->Traversal([&](const std::string &var_name) { UpdateVariableInternal(var_name); });
}

bool VariableManager::UpdateVariable(const std::string &var_name)
{
    if (IsVariableExist(var_name) == false)
    {
        return false;
    }

    auto topo_order = graph_->TopologicalSort(var_name);

    for (const auto &node_name : topo_order)
    {
        UpdateVariableInternal(node_name);
    }

    return true;
}