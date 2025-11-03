#pragma once
#include <string>
#include <vector>
#include <exception>

#include "py_base.h"
#include "core/expr_common.h"

namespace xexprengine
{
enum class ParseErrorCode
{
    kSuccess,
    kSyntaxError,
    kMultipleStatements,
    kUnsupportedType,
    kImportError,
    kAssignmentError,
    kAnalysisError
};

class ParseException : public std::exception {
private:
    ParseErrorCode error_code_;
    std::string error_message_;

public:
    ParseException(ParseErrorCode code, const std::string& message)
        : error_code_(code), error_message_(message) {}

    const char* what() const noexcept override {
        return error_message_.c_str();
    }

    ParseErrorCode error_code() const { return error_code_; }
    const std::string& error_message() const { return error_message_; }
    
    bool IsSyntaxError() const { return error_code_ == ParseErrorCode::kSyntaxError; }
    bool IsMultipleStatements() const { return error_code_ == ParseErrorCode::kMultipleStatements; }
    bool IsUnsupportedType() const { return error_code_ == ParseErrorCode::kUnsupportedType; }
    bool IsImportError() const { return error_code_ == ParseErrorCode::kImportError; }
    bool IsAssignmentError() const { return error_code_ == ParseErrorCode::kAssignmentError; }
};

class PyParser
{
public:
    PyParser();
    Variable AnalyzeStatement(const std::string& code);
private:
    static const std::string kExceptionCode;
    static const std::string kAnalysisCode;
    py::object analyzer_;
};
} // namespace xexprengine