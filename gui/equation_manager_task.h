#pragma once

#include "task/task.h"
#include "core/equation_manager.h"

namespace xequation 
{
namespace gui 
{
class EquationManagerTask : public Task 
{
    Q_OBJECT
public:
    EquationManagerTask(EquationManager* manager) : equation_manager_(manager) {}
    ~EquationManagerTask() override = default;

    virtual QVariant Execute() override;
    virtual void RequestCancel() override;
    virtual void Cleanup() override;
    EquationManager* equation_manager() const 
    {
        return equation_manager_;
    }
private:
    EquationManager* equation_manager_;
};

class UpdateEquationGroupTask : public EquationManagerTask 
{
    Q_OBJECT
  public:
    UpdateEquationGroupTask(EquationManager* manager, EquationGroupId group_id);
    ~UpdateEquationGroupTask() override = default;

    QVariant Execute() override;

  private:
    EquationGroupId group_id_;
};

// class UpdateAllEquationGroupsTask : public EquationManagerTask 
// {
//     Q_OBJECT
//   public:
//     UpdateAllEquationGroupsTask(EquationManager* manager);
//     ~UpdateAllEquationGroupsTask() override = default;

//     QVariant Execute() override;
//     void Cleanup() override;
// };

// class UpdateAfterRemoveEquationGroupTask : public EquationManagerTask 
// {
//     Q_OBJECT
//   public:
//     UpdateAfterRemoveEquationGroupTask(EquationManager* manager, const std::vector<std::string>& removed_equations);
//     ~UpdateAfterRemoveEquationGroupTask() override = default;

//     QVariant Execute() override;
//     void Cleanup() override;

//   private:
//     std::vector<std::string> removed_equations_;
// };

// class EvalExpressionTask : public EquationManagerTask 
// {
//     Q_OBJECT
//   public:
//     EvalExpressionTask(EquationManager* manager, const std::string& expression);
//     ~EvalExpressionTask() override = default;

//     QVariant Execute() override;
//     void Cleanup() override;

//     const std::string& expression() const
//     {
//         return expression_;
//     }

//     const Value& value() const
//     {
//         return value_;
//     }

//     const QString& message() const
//     {
//         return message_;
//     }

//     ResultStatus status() const
//     {
//         return status_;
//     }

//   private:
//     std::string expression_;
//     Value value_;
//     QString message_;
//     ResultStatus status_{ResultStatus::kUnknownError};
// };
}
}