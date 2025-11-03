#pragma once
#include <memory>
#include <string>
#include <map>

#include "dependency_graph.h"
#include "expr_common.h"
#include "expr_context.h"
#include "variable.h"

namespace xexprengine
{
class VariableManager
{
  public:
    VariableManager(std::unique_ptr<ExprContext> context, ExecCallback eval_callback = nullptr, ParseCallback parse_callback = nullptr) noexcept;
    virtual ~VariableManager() noexcept = default;

    const Variable* GetVariable(const std::string &var_name) const;

    // if code is invalid will throw exception
    void AddEquation(const std::string& code);

    bool RemoveVariable(const std::string &var_name) noexcept;

    bool IsVariableExist(const std::string &var_name) const;

    void Reset();

    void Update();

    bool UpdateVariable(const std::string& var_name);

    const DependencyGraph *graph()
    {
        return graph_.get();
    }

    const ExprContext* context()
    {
        return context_.get();
    }

  private:
    VariableManager(const VariableManager &) = delete;
    VariableManager &operator=(const VariableManager &) = delete;

    VariableManager(VariableManager &&) noexcept = delete;
    VariableManager &operator=(VariableManager &&) noexcept = delete;

    // update value to context and update statue to variable
    bool UpdateVariableInternal(const std::string &var_name);
    
    bool AddVariableToGraph(const Variable* var);
    bool RemoveValueInContext();

  private:
    std::unique_ptr<DependencyGraph> graph_;
    std::unique_ptr<ExprContext> context_;
    std::unordered_map<std::string, Variable> variable_map_;
    ExecCallback exec_callback_;
    ParseCallback parse_callback_;
};
} // namespace xexprengine