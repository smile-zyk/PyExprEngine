#pragma once
#include <string>
#include <unordered_map>
#include <memory>
#include "expr_context.h"
#include "eval_result.hpp"

namespace xexprengine {

    class ExprEngine {
        public:
            ExprEngine() = default;
            ~ExprEngine() = default;

            virtual EvalResult Evaluate(const std::string& expr, ExprContext* context = nullptr) = 0;
            virtual std::vector<std::string> GetDependencies(const std::string& expr, ExprContext* context = nullptr) = 0;
            
            void RegisterContext(const std::string& name, std::unique_ptr<ExprContext> context);
            void SetCurrentContext(ExprContext* context);
            void SetCurrentContext(const std::string& name);
            void RemoveContext(ExprContext* context);
            void RemoveContext(const std::string& name);

            ExprContext* GetCurrentContext() const;
            ExprContext* GetContext(const std::string& name) const;
        private:
            std::unordered_map<std::string, std::unique_ptr<ExprContext>> context_map_;
            ExprContext* current_context_ = nullptr;
    };
}