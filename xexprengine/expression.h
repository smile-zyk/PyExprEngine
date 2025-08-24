#pragma once
#include <string>
#include "eval_result.hpp"
#include "expr_value.h"
#include "eval_result.hpp"

namespace xexprengine {

    class ExprContext;

    class Expression
    {
    public:
        enum class Status
        {
            Initial,
            Valid,
            Invalid,
        };

        Expression(const std::string& name, const std::string& expression_str) noexcept
            : name_(name), expression_str_(expression_str) {}

        ~Expression() = default;

        ExprValue Evaluate() const;

        void set_context(ExprContext* context) noexcept { context_ = context; }
        void set_expression_str(const std::string& expression_str) noexcept 
        {
            expression_str_ = expression_str;
            status_ = Status::Initial; 
        }

        const ExprContext* context() const noexcept { return context_; }
        const std::string& name() const noexcept { return name_; }
        const std::string& expression_str() const noexcept { return expression_str_; }

        Status status() const noexcept { return status_; }

    private:
        std::string name_;
        std::string expression_str_;
        ExprContext* context_;
        Status status_ = Status::Initial;
        std::string error_message_;
        ErrorCode error_code_ = ErrorCode::Success;
    };
}