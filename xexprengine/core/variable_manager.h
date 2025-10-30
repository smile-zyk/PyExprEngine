#pragma once
#include <map>
#include <memory>
#include <string>

#include "dependency_graph.h"
#include "expr_common.h"
#include "expr_context.h"
#include "variable.h"

namespace xexprengine
{
class VariableManager : public VariableVisitor
{
  public:
    VariableManager(
        std::unique_ptr<ExprContext> context, EvalCallback eval_callback = nullptr,
        ParseCallback parse_callback = nullptr
    ) noexcept;
    virtual ~VariableManager() noexcept = default;

    // get variable
    const Variable *GetVariable(const std::string &var_name) const;

    // set variable : if variable not exist , add variable, otherwise overwrite variable
    void SetValue(const std::string &var_name, const Value &value);
    void SetExpression(const std::string &var_name, const std::string &expression);
    void SetVariable(const std::string &var_name, std::unique_ptr<Variable> variable);
    std::string ImportModule(const std::string &statement, const std::string &last_variable = "");
    std::string DeclFunction(const std::string &statement, const std::string &last_variable = "");

    // remove variable
    bool RemoveVariable(const std::string &var_name) noexcept;

    // check both graph node and variable exist
    bool IsVariableExist(const std::string &var_name) const;

    // clear graph and variable map
    void Reset();

    // use graph update all variable to context
    void Update();

    // use graph update single varible and its dependents to context
    bool UpdateVariable(const std::string &var_name);

    // visitor interface
    void Parse(RawVariable *) override;
    void Parse(ExprVariable *) override;
    void Parse(ImportVariable *) override;
    void Parse(FuncVariable *) override;

    void Update(RawVariable *) override;
    void Update(ExprVariable *) override;
    void Update(ImportVariable *) override;
    void Update(FuncVariable *) override;

    void Clear(RawVariable *) override;
    void Clear(ExprVariable *) override;
    void Clear(ImportVariable *) override;
    void Clear(FuncVariable *) override;

    const DependencyGraph *graph()
    {
        return graph_.get();
    }

    const ExprContext *context()
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

    // sync graph
    bool AddNodeToGraph(const std::string &node_name, const std::vector<std::string> &node_dependencies);
    bool RemoveNodeInGraph(const std::string &node_name) noexcept;

    // sync context
    bool UpdateVariableToContext(const std::string &var_name, const Value &value);
    bool RemoveVariableInContext(const std::string &node_name);

    // graph helper functions
    bool UpdateNodeDependencies(const std::string &node_name, const std::unordered_set<std::string> &node_dependencies);

  private:
    std::unique_ptr<DependencyGraph> graph_;
    std::unique_ptr<ExprContext> context_;
    std::unordered_map<std::string, std::unique_ptr<Variable>> variable_map_;
    EvalCallback evaluate_callback_;
    ParseCallback parse_callback_;
};
} // namespace xexprengine