#pragma once
#include <memory>
#include <string>

#include "core/variable.h"
#include "expr_common.h"
#include "expr_context.h"
#include "variable_manager.h"

namespace xexprengine
{
template <typename T>
class ExprEngine
{
  public:
    static T &GetInstance()
    {
        static T instance;
        return instance;
    }

    virtual ExecResult Exec(const std::string& code, const ExprContext *context = nullptr) = 0;
    virtual Variable Parse(const std::string & code) = 0;

    virtual std::unique_ptr<VariableManager> CreateVariableManager()
    {
        ExecCallback exec_callback = [this](const std::string &code, ExprContext *context) -> ExecResult {
            return Exec(code, context);
        };

        ParseCallback parse_callback = [this](const std::string &code) -> ParseResult {
            return Parse(code);
        };

        return std::unique_ptr<VariableManager>(new VariableManager(CreateContext(), exec_callback, parse_callback));
    }

    virtual std::unique_ptr<ExprContext> CreateContext() = 0;

  protected:
    ExprEngine() = default;
    virtual ~ExprEngine() = default;
    ExprEngine(const ExprEngine &) = delete;
    ExprEngine(ExprEngine &&) = delete;
    ExprEngine &operator=(const ExprEngine &) = delete;
    ExprEngine &operator=(ExprEngine &&) = delete;
};
} // namespace xexprengine