#pragma once
#include <string>
#include <memory>
#include <unordered_map>
#include <functional>
#include "eval_result.hpp"
#include "expr_value.h"
#include "expression.h"

namespace xexprengine 
{
    class ExprEngine;
    class ExprContext
    {
        public:
            ExprContext() = default;
            ExprValue GetValue(const std::string& var_name) const;
            virtual ExprValue GetVariable(const std::string& var_name) const = 0;
            virtual void SetVariable(const std::string& var_name, const ExprValue& value);
            virtual void RemoveVariable(const std::string& var_name);
            virtual void RenameVariable(const std::string& old_name, const std::string& new_name);

            void SetVariable(const std::string& var_name, std::unique_ptr<Expression> value);
            void SetExpression(const std::string& expr_name, std::string expr_str);
            Expression* GetExpression(const std::string& expr_name) const;
            bool IsVariableDirty(const std::string& var_name) const;
            std::unordered_set<std::string> GetVariableDependents(const std::string& var_name) const;
            std::unordered_set<std::string> GetVariableDependencies(const std::string& var_name) const;
            bool IsVariableExist(const std::string& var_name) const;

            void TraverseVariable(const std::string& var_name, const std::function<void(ExprContext*, const std::string&)>& func);
            void UpdateVariableGraph();
            EvalResult Evaluate(const std::string& expr_str);
            ExprValue Evaluate(Expression* expr);
        private:
            struct ExprNode {
                std::unordered_set<std::string> dependencies; // Variables this node depends on
                std::unordered_set<std::string> dependents;   // Variables that depend on this node
                bool is_dirty = false;
            };
            std::unordered_map<std::string, std::unique_ptr<Expression>> expr_map_;
            std::unordered_set<std::string> var_set_;
            std::unordered_map<std::string, ExprNode> var_graph_;
            ExprEngine* engine_ = nullptr;
    };
}