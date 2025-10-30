#pragma once
#include "value.h"
#include <map>
#include <string>
#include <functional>
#include <unordered_map>
#include <unordered_set>

namespace xexprengine
{
enum class VariableStatus
{
    kInit,
    kInvalidContext,
    kRawVar,
    kMissingDependency,
    kCycleDependency,
    kParseSuccess,
    kParseSyntaxError,
    kExprEvalSuccess,
    kExprEvalSyntaxError,
    kExprEvalNameError,
    kExprEvalTypeError,
    kExprEvalZeroDivisionError,
    kExprEvalValueError,
    kExprEvalMemoryError,
    kExprEvalOverflowError,
    kExprEvalRecursionError,
    kExprEvalIndexError,
    kExprEvalKeyError,
    kExprEvalAttributeError,
};

struct EvalResult
{
    Value value;
    VariableStatus status;
    std::string eval_error_message;
};

enum class ParseType
{
    kExpression,
    kFunction,
    kModule
};

struct ParseResult
{
    ParseType type;
    std::string content;
    std::vector<std::string> generated_symbols;
    std::vector<std::string> dependency_symbols;
    bool is_success;
    std::string parse_error_message;
};

class ExprContext;

typedef std::function<EvalResult(const std::string &, ExprContext*)> EvalCallback;
typedef std::function<ParseResult(const std::string &)> ParseCallback;
} // namespace xexprengine
