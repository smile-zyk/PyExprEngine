#pragma once
#include <exception>
#include <memory>
#include <string>
#include <unordered_map>

#include <boost/uuid/uuid_io.hpp>

#include "dependency_graph.h"
#include "equation.h"
#include "equation_common.h"
#include "equation_context.h"
#include "equation_group.h"

namespace xequation
{

class EquationException : public std::exception
{
  public:
    enum class ErrorCode
    {
        kEquationGroupNotFound,
        kEquationGroupAlreadyExists,
        kEquationNotFound,
        kEquationAlreayExists,
    };

    const char *what() const noexcept override
    {
        if (message_cache_.empty())
        {
            message_cache_ = GenerateErrorMessage();
        }
        return message_cache_.c_str();
    }

    const std::string &equation_name()
    {
        return equation_name_;
    }

    const EquationGroupId &group_id()
    {
        return group_id_;
    }

    ErrorCode error_code()
    {
        return error_code_;
    }

    static EquationException EquationGroupNotFound(EquationGroupId group_id) {
        return EquationException(ErrorCode::kEquationGroupNotFound, group_id);
    }
    
    static EquationException EquationGroupAlreadyExists(EquationGroupId group_id) {
        return EquationException(ErrorCode::kEquationGroupAlreadyExists, group_id);
    }
    
    static EquationException EquationNotFound(const std::string& equation_name) {
        return EquationException(ErrorCode::kEquationNotFound, equation_name);
    }
    
    static EquationException EquationAlreadyExists(const std::string& equation_name) {
        return EquationException(ErrorCode::kEquationAlreayExists, equation_name);
    }

  private:
    std::string GenerateErrorMessage() const
    {
        std::ostringstream oss;

        switch (error_code_)
        {
        case ErrorCode::kEquationGroupNotFound:
            oss << "Equation group not found. Group ID: " << boost::uuids::to_string(group_id_);
            break;

        case ErrorCode::kEquationGroupAlreadyExists:
            oss << "Equation group already exists. Group ID: " << boost::uuids::to_string(group_id_);
            break;

        case ErrorCode::kEquationNotFound:
            oss << "Equation not found. Name: '" << equation_name_ << "'";
            break;

        case ErrorCode::kEquationAlreayExists:
            oss << "Equation already exists. Name: '" << equation_name_ << "'";
            break;

        default:
            oss << "Unknown equation error occurred.";
            break;
        }

        return oss.str();
    }

    EquationException(ErrorCode error_code, const std::string &equation_name)
        : error_code_(error_code), equation_name_(equation_name)
    {
    }

    EquationException(ErrorCode error_code, const EquationGroupId &group_id)
        : error_code_(error_code), group_id_(group_id)
    {
    }

    ErrorCode error_code_;
    std::string equation_name_;
    EquationGroupId group_id_;
    mutable std::string message_cache_;
};

class EquationManager
{
  public:
    using EquationCallback = std::function<void(const EquationManager *manager, const std::string &equation_name)>;
    using EquationGroupCallback = std::function<void(const EquationManager *manager, const EquationGroupId &group_id)>;

    using CallbackId = size_t;

    EquationManager(
        std::unique_ptr<EquationContext> context, ExecHandler exec_handler, ParseHandler parse_callback,
        EvalHandler eval_callback = nullptr
    ) noexcept;

    virtual ~EquationManager() noexcept = default;

    const EquationGroup *GetEquationGroup(const EquationGroupId &group_id) const;

    const Equation *GetEquation(const std::string &equation_name) const;

    bool IsEquationGroupExist(const EquationGroupId &group_id) const;

    bool IsEquationExist(const std::string &eqn_name) const;

    EquationGroupId AddEquationGroup(const std::string &equation_statement);

    void EditEquationGroup(const EquationGroupId &group_id, const std::string &equation_statement);

    void RemoveEquationGroup(const EquationGroupId &group_id);

    EvalResult Eval(const std::string &expression) const;

    void Reset();

    void Update();

    void UpdateEquation(const std::string &equation_name);

    void UpdateEquationGroup(const EquationGroupId &group_id);

    const DependencyGraph *graph()
    {
        return graph_.get();
    }

    const EquationContext *context()
    {
        return context_.get();
    }

    const EquationGroupPtrOrderedMap &equation_group_map()
    {
        return equation_group_map_;
    }

    CallbackId RegisterEquationAddedCallback(EquationCallback callback);
    void UnRegisterEquationAddedCallback(CallbackId callback_id);

    CallbackId RegisterEquationRemovingCallback(EquationCallback callback);
    void UnRegisterEquationRemovingCallback(CallbackId callback_id);

    void NotifyEquationAdded(const std::string &equation_name) const;
    void NotifyEquationRemoving(const std::string &equation_name) const;

    CallbackId RegisterEquationGroupAddedCallback(EquationGroupCallback callback);
    void UnRegisterEquationGroupAddedCallback(CallbackId callback_id);

    CallbackId RegisterEquationGroupRemovingCallback(EquationGroupCallback callback);
    void UnRegisterEquationGroupRemovingCallback(CallbackId callback_id);

    void NotifyEquationGroupAdded(const EquationGroupId &group_id) const;
    void NotifyEquationGroupRemoving(const EquationGroupId &group_id) const;
    void NotifyEquationGroupUpdated(const EquationGroupId &group_id) const;

  private:
    EquationManager(const EquationManager &) = delete;
    EquationManager &operator=(const EquationManager &) = delete;

    EquationManager(EquationManager &&) noexcept = delete;
    EquationManager &operator=(EquationManager &&) noexcept = delete;

    Equation *GetEquationInteral(const std::string &equation_name);
    void UpdateEquationInternal(const std::string &var_name);

    void AddNodeToGraph(const std::string &node_name, const std::vector<std::string> &dependencies);
    void RemoveNodeInGraph(const std::string &node_name);

    void UpdateValueToContext(const std::string &equation_name, const Value &old_value);
    void RemoveValueInContext(const std::string &equation_name);

  private:
    std::unique_ptr<DependencyGraph> graph_;
    std::unique_ptr<EquationContext> context_;

    EquationGroupPtrOrderedMap equation_group_map_;
    std::unordered_map<std::string, boost::uuids::uuid> equation_name_to_group_id_map_;

    ExecHandler exec_handler_ = nullptr;
    ParseHandler parse_handler_ = nullptr;
    EvalHandler eval_handler_ = nullptr;

    CallbackId next_callback_id = 0;

    std::unordered_map<CallbackId, EquationCallback> equation_callback_map_;
    std::unordered_set<CallbackId> equation_added_callback_set_;
    std::unordered_set<CallbackId> equation_removing_callback_set_;

    std::unordered_map<CallbackId, EquationGroupCallback> equation_group_callback_map_;
    std::unordered_set<CallbackId> equation_group_add_callback_set_;
    std::unordered_set<CallbackId> equation_group_removing_callback_set_;
    std::unordered_set<CallbackId> equation_group_update_callback_set_;
};
} // namespace xequation