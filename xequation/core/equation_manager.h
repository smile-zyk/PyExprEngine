#pragma once
#include <cstddef>
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>

#include "core/equation_common.h"
#include "dependency_graph.h"
#include "equation.h"
#include "equation_common.h"
#include "equation_context.h"

namespace xequation
{

class DuplicateEquationNameError : public std::exception
{
  private:
    std::string exist_equation_name_;
    std::string exist_equation_content_;

  public:
    DuplicateEquationNameError(const std::string &eqn_name, const std::string eqn_content)
        : exist_equation_name_(eqn_name), exist_equation_content_(eqn_content)
    {
    }

    const char *what() const noexcept override
    {
        static std::string res;
        res = exist_equation_name_ + " is exist! exist equation content: " + exist_equation_content_;
        return res.c_str();
    }

    const std::string &exist_equation_name() const
    {
        return exist_equation_name_;
    }
    const std::string &exist_equation_content() const
    {
        return exist_equation_content_;
    }
};

class EquationManager
{
  public:
    using EquationCallback = std::function<void(const EquationManager *manager, const std::string &equation_name)>;

    using CallbackId = size_t;

    EquationManager(
        std::unique_ptr<EquationContext> context, ExecHandler exec_handler, ParseHandler parse_callback,
        EvalHandler eval_callback = nullptr
    ) noexcept;

    virtual ~EquationManager() noexcept = default;

    const Equation* GetEquation(const std::string& equation_name) const;
    
    const EquationGroup* GetEquationGroup(const EquationGroupId& group_id) const;

    bool IsEquationExist(const std::string &eqn_name) const;

    bool IsEquatoinGroupExist(const EquationGroupId& group_id) const;

    EquationGroupId AddEquationGroup(const std::string &equation_statements);

    void EditEquationGroup(const std::string &old_eqn_name, const std::string &new_eqn_name, const std::string &new_eqn_expr);

    void RemoveEquationGroup(const std::string &eqn_name);

    EvalResult Eval(const std::string &expression) const;

    void Reset();

    void Update();

    void UpdateEquation(const std::string &eqn_name);

    void UpdateMultipleEquations(const std::string &eqn_code);

    const DependencyGraph *graph()
    {
        return graph_.get();
    }

    const EquationContext *context()
    {
        return context_.get();
    }

    const EquationGroupPtrOrderedMap& equation_group_map()
    {
        return equation_group_map_;
    }

    CallbackId RegisterEquationAddedCallback(EquationCallback callback);
    void UnRegisterEquationAddedCallback(CallbackId callback_id);

    CallbackId RegisterEquationRemovingCallback(EquationCallback callback);
    void UnRegisterEquationRemovingCallback(CallbackId callback_id);

    void NotifyEquationAdded(const std::string& equation_name) const;
    void NotifyEquationRemoving(const std::string& equation_name) const;

  private:
    EquationManager(const EquationManager &) = delete;
    EquationManager &operator=(const EquationManager &) = delete;

    EquationManager(EquationManager &&) noexcept = delete;
    EquationManager &operator=(EquationManager &&) noexcept = delete;

    void AddEquationStatement(const std::string &eqn_code);
    void EditEquationStatement(const std::string &old_eqn_code, const std::string &equation_code);
    void RemoveEquationStatement(const std::string &eqn_code);

    Equation* GetEquationInteral(const std::string& equation_name);
    void UpdateEquationInternal(const std::string &var_name);

    void AddNodeToGraph(const std::string &node_name, const std::vector<std::string> &dependencies);
    void RemoveNodeInGraph(const std::string &node_name);

    void UpdateValueToContext(const std::string &equation_name, const Value &old_value);
    void RemoveValueInContext(const std::string &equation_name);

    std::unique_ptr<Equation> ConstructEquationPtr(const ParseResultItem &item);
    void UpdateEquationPtr(const ParseResultItem &item);

  private:
    std::unique_ptr<DependencyGraph> graph_;
    std::unique_ptr<EquationContext> context_;
    std::vector<std::string> equation_order_;
    EquationGroupPtrOrderedMap equation_group_map_;
    std::unordered_map<std::string, boost::uuids::uuid> equation_name_to_group_id_map_;

    CallbackId next_callback_id = 0;
    std::unordered_map<CallbackId, EquationCallback> equation_callback_map_;
    std::unordered_set<CallbackId> equation_add_callback_set_;
    std::unordered_set<CallbackId> equation_remove_callback_set_;
    ExecHandler exec_handler_ = nullptr;
    ParseHandler parse_handler_ = nullptr;
    EvalHandler eval_handler_ = nullptr;
};
} // namespace xequation