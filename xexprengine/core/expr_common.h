#pragma once

#include <functional>
#include <string>
#include <vector>

namespace xexprengine
{
class ExprContext;

enum class ExecStatus
{
    kInit,
    kSuccess,
    kSyntaxError,
    kNameError,
    kTypeError,
    kZeroDivisionError,
    kValueError,
    kMemoryError,
    kOverflowError,
    kRecursionError,
    kIndexError,
    kKeyError,
    kAttributeError,
};

enum class ParseType
{
    kImport,
    kImportFrom,
    kFuncDecl,
    kClassDecl,
    kVarDecl,
};

struct ExecResult
{
    ExecStatus status;
    std::string message;
};

struct ParseResult
{
    std::string name;
    std::string content;
    ParseType type;
    std::vector<std::string> dependencies;
};

typedef std::function<ExecResult(const std::string &, ExprContext *)>ExecCallback;
typedef std::function<ParseResult(const std::string &)> ParseCallback;
} // namespace xexprengine
