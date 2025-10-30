#pragma once
#include "expr_common.h"
#include "value.h"
#include <memory>
#include <string>

namespace xexprengine
{

class RawVariable;
class ExprVariable;
class ImportVariable;
class FuncVariable;

class VariableVisitor
{
  public:
    virtual void Parse(RawVariable *) = 0;
    virtual void Parse(ExprVariable *) = 0;
    virtual void Parse(ImportVariable *) = 0;
    virtual void Parse(FuncVariable *) = 0;

    virtual void Update(RawVariable *) = 0;
    virtual void Update(ExprVariable *) = 0;
    virtual void Update(ImportVariable *) = 0;
    virtual void Update(FuncVariable *) = 0;

    virtual void Clear(RawVariable *) = 0;
    virtual void Clear(ExprVariable *) = 0;
    virtual void Clear(ImportVariable *) = 0;
    virtual void Clear(FuncVariable *) = 0;
};

class Variable
{
  public:
    enum class Type
    {
        Raw,
        Expr,
        Import,
        Func,
    };

    Variable(const std::string &name) : name_(name) {}
    virtual ~Variable() = default;

    void set_name(const std::string &name)
    {
        name_ = name;
    }

    const std::string &name() const
    {
        return name_;
    }

    virtual void AcceptParse(VariableVisitor *visitor) = 0;

    virtual void AcceptUpdate(VariableVisitor *visitor) = 0;

    virtual void AcceptClear(VariableVisitor *visitor) = 0;

    void set_error_message(const std::string &message)
    {
        error_message_ = message;
    }

    void set_status(VariableStatus code)
    {
        status_ = code;
    }

    std::string error_message() const
    {
        return error_message_;
    }

    VariableStatus status() const
    {
        return status_;
    }

    virtual Type GetType() const = 0;

  private:
    std::string name_;
    std::string error_message_;
    VariableStatus status_ = VariableStatus::kInit;
};

class VariableFactory
{
  public:
    static std::unique_ptr<Variable> CreateRawVariable(const std::string &name, const Value &value);

    static std::unique_ptr<Variable> CreateExprVariable(const std::string &name, const std::string &expression);

    static std::unique_ptr<Variable> CreateImportVariable(const std::string &import_statement);

    static std::unique_ptr<Variable> CreateFuncVariable(const std::string &func_statement);
};

class RawVariable : public Variable
{
  public:
    void AcceptParse(VariableVisitor *visitor) override
    {
        visitor->Parse(this);
    }

    void AcceptUpdate(VariableVisitor *visitor) override
    {
        visitor->Update(this);
    }

    void AcceptClear(VariableVisitor *visitor) override
    {
        visitor->Clear(this);
    }

    void set_value(const Value &value)
    {
        value_ = value;
    }

    const Value &value() const
    {
        return value_;
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Raw;
    }

  protected:
    RawVariable(const std::string &name, const Value &value) : Variable(name), value_(value)
    {
    }
    friend class VariableFactory;

  private:
    Value value_;
};

class ExprVariable : public Variable
{
  public:
    void AcceptParse(VariableVisitor *visitor) override
    {
        visitor->Parse(this);
    }

    void AcceptUpdate(VariableVisitor *visitor) override
    {
        visitor->Update(this);
    }

    void AcceptClear(VariableVisitor *visitor) override
    {
        visitor->Clear(this);
    }

    void set_expression(const std::string &expression)
    {
        expression_ = expression;
    }

    const std::string &expression() const
    {
        return expression_;
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Expr;
    }

  protected:
    ExprVariable(const std::string &name, const std::string &expression) : Variable(name), expression_(expression) {}
    friend class VariableFactory;

  private:
    std::string expression_;
};

class ImportVariable : public Variable
{
  public:
    void AcceptParse(VariableVisitor *visitor) override
    {
        visitor->Parse(this);
    }

    void AcceptUpdate(VariableVisitor *visitor) override
    {
        visitor->Update(this);
    }

    void AcceptClear(VariableVisitor *visitor) override
    {
        visitor->Clear(this);
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Import;
    }

    const std::string& statement() const { return import_statement_; }    

    const std::vector<std::string>& symbols() const { return import_symbols_; }

  protected:
    ImportVariable(const std::string &statement) : Variable(""), import_statement_(statement) {}
    friend class VariableFactory;

  private:
    std::vector<std::string> import_symbols_;
    std::string import_statement_;
};

class FuncVariable : public Variable
{
  public:
    void AcceptParse(VariableVisitor *visitor) override
    {
        visitor->Parse(this);
    }

    void AcceptUpdate(VariableVisitor *visitor) override
    {
        visitor->Update(this);
    }

    void AcceptClear(VariableVisitor *visitor) override
    {
        visitor->Clear(this);
    }

    Variable::Type GetType() const override
    {
        return Variable::Type::Func;
    }

    const std::string& statement() const { return func_statement_; }

  protected:
    FuncVariable(const std::string &statement) : Variable(""), func_statement_(statement) {}
    friend class VariableFactory;

  private:
    std::string func_statement_;
};
} // namespace xexprengine