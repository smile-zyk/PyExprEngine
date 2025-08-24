#pragma once
#include <string>
#include "expr_value.h"

namespace xexprengine {

    enum class ErrorCode {
        Success = 0,
        ParseError,
        EvaluationError,
        TypeMismatch,
        UnknownVariable,
        DivisionByZero,
        InvalidOperation,
        Overflow,
        Underflow,
        OutOfMemory,
        InternalError
    };

    class EvalResult
    {
    public:
        EvalResult() noexcept = default;
        ~EvalResult() noexcept = default;

        bool IsSuccess() const noexcept { return error_code_ == ErrorCode::Success; }
        ErrorCode error_code() const noexcept { return error_code_; }
        const std::string& error_message() const noexcept { return error_message_; }
        const ExprValue& value() const noexcept { return value_; }

        void SetError(ErrorCode code, const std::string& message) {
            error_code_ = code;
            error_message_ = message;
            value_ = ExprValue::Null();
        }

        void SetValue(const ExprValue& value) {
            value_ = value;
            error_code_ = ErrorCode::Success;
            error_message_.clear();
        }

    private:
        ExprValue value_;
        ErrorCode error_code_ = ErrorCode::Success;
        std::string error_message_;
    };
}
