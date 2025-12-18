#pragma once

#include <exception>
#include <functional>
#include <string>
#include <vector>

#include "value.h"

namespace xequation
{
class EquationContext;

enum class ResultStatus
{
    kPending,
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

enum class InterpretMode
{
    kExec,
    kEval
};

struct InterpretResult
{
    InterpretMode mode;
    ResultStatus status;
    std::string message;
    Value value;
};

struct ParseResultItem
{
    enum class Type
    {
        kUnknown,
        kExpression,
        kVariable,
        kFunction,
        kClass,
        kImport,
        kImportFrom,
    };
    std::string name;
    std::string code;
    Type type;
    std::vector<std::string> dependencies;

    bool operator==(const ParseResultItem &other) const
    {
        return name == other.name && code == other.code && type == other.type &&
               dependencies == other.dependencies;
    }

    bool operator!=(const ParseResultItem &other) const
    {
        return !(*this == other);
    }
};

enum class ParseMode
{
    kStatement,
    kExpression,
};

struct ParseResult
{
    ParseMode mode;
    std::vector<ParseResultItem> items;
    std::string message;
    ResultStatus status;
};

class ParseException : public std::exception
{
  private:
    std::string error_message_;

  public:
    ParseException(const std::string &message) : error_message_(message) {}

    const char *what() const noexcept override
    {
        return error_message_.c_str();
    }

    const std::string &error_message() const
    {
        return error_message_;
    }
};

inline std::ostream &operator<<(std::ostream &os, ParseResultItem::Type type)
{
    std::string res;
    switch (type)
    {
    case ParseResultItem::Type::kVariable:
        res = "Variable";
    case ParseResultItem::Type::kFunction:
        res = "Function";
    case ParseResultItem::Type::kClass:
        res = "Class";
    case ParseResultItem::Type::kImport:
        res = "Import";
    case ParseResultItem::Type::kImportFrom:
        res = "ImportFrom";
    default:
        res = "Unknown";
    }

    return os << res;
}

inline std::ostream &operator<<(std::ostream &os, ResultStatus status)
{
    std::string res;
    switch (status)
    {
    case ResultStatus::kPending:
        res = "Pending";
    case ResultStatus::kSuccess:
        res = "Success";
    case ResultStatus::kSyntaxError:
        res = "SyntaxError";
    case ResultStatus::kNameError:
        res = "NameError";
    case ResultStatus::kTypeError:
        res = "TypeError";
    case ResultStatus::kZeroDivisionError:
        res = "ZeroDivisionError";
    case ResultStatus::kValueError:
        res = "ValueError";
    case ResultStatus::kMemoryError:
        res = "MemoryError";
    case ResultStatus::kOverflowError:
        res = "OverflowError";
    case ResultStatus::kRecursionError:
        res = "RecursionError";
    case ResultStatus::kIndexError:
        res = "IndexError";
    case ResultStatus::kKeyError:
        res = "KeyError";
    case ResultStatus::kAttributeError:
        res = "AttributeError";
    default:
        res = "Unknown";
    }
    return os << res;
}

using InterpretHandler = std::function<InterpretResult(const std::string &, EquationContext *, InterpretMode)>;
using ParseHandler = std::function<ParseResult(const std::string &, ParseMode)>;
} // namespace xequation

namespace std
{
template <>
struct hash<xequation::ParseResultItem>
{
    size_t operator()(const xequation::ParseResultItem &item) const
    {
        size_t h = 0;
        std::hash<std::string> string_hasher;
        std::hash<xequation::ParseResultItem::Type> type_hasher;

        h ^= string_hasher(item.name) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= string_hasher(item.code) + 0x9e3779b9 + (h << 6) + (h >> 2);
        h ^= type_hasher(item.type) + 0x9e3779b9 + (h << 6) + (h >> 2);

        for (const auto &dep : item.dependencies)
        {
            h ^= string_hasher(dep) + 0x9e3779b9 + (h << 6) + (h >> 2);
        }

        return h;
    }
};
} // namespace std