#pragma once
#include <string>
#include <unordered_set>
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

    struct EvalResult
    {
        ExprValue value;
        ErrorCode error_code = ErrorCode::Success;
        std::string error_message;
    };

    struct AnalyzeResult
    {
        std::unordered_set<std::string> variables;
        std::unordered_set<std::string> functions;
        std::unordered_set<std::string> modules;
    };
}
