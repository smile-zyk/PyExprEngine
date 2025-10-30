#include "variable.h"
#include "expr_common.h"

using namespace xexprengine;

std::unique_ptr<Variable> VariableFactory::CreateRawVariable(const std::string &name, const Value &value)
{
    return std::unique_ptr<Variable>(new RawVariable(name, value));
}

std::unique_ptr<Variable> VariableFactory::CreateExprVariable(const std::string &name, const std::string &expression)
{
    return std::unique_ptr<Variable>(new ExprVariable(name, expression));
}

std::unique_ptr<Variable> VariableFactory::CreateImportVariable(const std::string &statement)
{
    return std::unique_ptr<Variable>(new ImportVariable(statement));
}

std::unique_ptr<Variable> VariableFactory::CreateFuncVariable(const std::string& statement)
{
    return std::unique_ptr<Variable>(new FuncVariable(statement));
}