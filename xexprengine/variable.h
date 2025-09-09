#pragma once
#include "expr_common.h"
#include "value.h"

namespace xexprengine
{
class ExprContext;

class Variable
{
  public:
    enum class Type
    {
        Raw,
        Expr,
        Func,
    };

    Variable(const std::string &name, ExprContext *context = nullptr) : name_(name), context_(context) {}
    virtual ~Variable() = default;
    const std::string &name() const
    {
        return name_;
    }

    const ExprContext *context() const
    {
        return context_;
    }

    template <typename T, typename std::enable_if<std::is_base_of<Variable, T>::value, int>::type = 0>
    T* As() noexcept
    {
        return dynamic_cast<T *>(this);
    }

    Value GetValue() const;
    virtual ParseResult GetParseResult() const = 0;
    virtual Value Evaluate()  = 0;
    virtual Type GetType() const = 0;

  protected:
    void set_name(const std::string &name)
    {
        name_ = name;
    }

    void set_context(ExprContext *context)
    {
        context_ = context;
    }
    
    friend class ExprContext;

  private:
    std::string name_;
    bool dirty_flag_;
    ExprContext *context_;
};

class RawVariable : public Variable
{
  public:
    Value Evaluate() override
    {
        return value_;
    }

    ParseResult GetParseResult() const override
    {
        return {};
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Raw;
    }

  protected:
    RawVariable(const std::string &name, const Value &value, ExprContext *context = nullptr)
        : Variable(name, context), value_(value)
    {
    }
    friend class VariableFactory;

  private:
    Value value_;
};

class ExprVariable : public Variable
{
  public:
    const std::string &expression() const
    {
        return expression_;
    }

    std::string error_message() const
    {
        return error_message_;
    }

    ErrorCode error_code() const
    {
        return error_code_;
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Expr;
    }

    Value Evaluate() override;

    ParseResult GetParseResult() const override
    {
        return parse_result_;
    }

  protected:
    ExprVariable(const std::string &name, const std::string &expression, ExprContext *context = nullptr)
        : Variable(name, context), expression_(expression)
    {
        Parse();
    }

    friend class VariableFactory;

  private:
    void Parse();
    std::string expression_;
    std::string error_message_;
    ErrorCode error_code_ = ErrorCode::Success;
    ParseResult parse_result_;
};
} // namespace xexprengine