#pragma once
#include "value.h"
#include <string>
#include <unordered_set>

namespace xexprengine
{
enum class ErrorCode
{
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
    Value value;
    ErrorCode error_code = ErrorCode::Success;
    std::string error_message;
};

struct ParseResult
{
    std::unordered_set<std::string> variables;
    std::unordered_set<std::string> functions;
    std::unordered_set<std::string> modules;
};
} // namespace xexprengine
