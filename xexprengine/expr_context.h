#pragma once
#include "expr_common.h"
#include "expression.h"
#include "value.h"
#include "variable_dependency_graph.h"
#include <functional>
#include <memory>
#include <string>
#include <unordered_map>
#include <unordered_set>

namespace xexprengine
{
class ExprEngine;

class ExprContext
{
  public:
    ExprContext() = default;

    void SetVariable(const std::string &var_name, const Value &value);
    Value GetVariable(const std::string &var_name);
    void RemoveVariable(const std::string &var_name);
    void RenameVariable(const std::string &old_name, const std::string &new_name);

    void SetExpression(const std::string &expr_name, std::string expr_str);
    Expression *GetExpression(const std::string &expr_name) const;

    bool IsVariableExist(const std::string &var_name) const;

    void
    TraverseVariable(const std::string &var_name, const std::function<void(ExprContext *, const std::string &)> &func);
    void UpdateVariableGraph();

    EvalResult Evaluate(const std::string &expr_str);
    Value Evaluate(Expression *expr);

    const std::string &name() { return name_; }

  private:
    std::unordered_map<std::string, std::unique_ptr<Expression>> expr_map_;
    std::unique_ptr<VariableDependencyGraph> graph_;

    std::string name_;
    ExprEngine *engine_ = nullptr;
};
} // namespace xexprengine