#include "py_expr_context.h"
#include "py_expr_engine.h"
#include "core/value.h"
#include <pybind11/cast.h>
#include <pybind11/gil.h>
#include <pybind11/pytypes.h>

using namespace xexprengine;

PyExprContext::PyExprContext()
{
    auto evaluate_callback = [this](const std::string &expression) -> EvalResult {
        return PyExprEngine::GetInstance().Evaluate(expression, this);
    };

    auto parse_callback = [this](const std::string &expression) -> ParseResult {
        return PyExprEngine::GetInstance().Parse(expression);
    };

    set_evaluate_callback(evaluate_callback);
    set_parse_callback(parse_callback);
}

Value PyExprContext::GetContextValue(const std::string &var_name) const
{
    py::gil_scoped_acquire acquire;
    if (context_dict_.contains(var_name))
    {
        return py::cast<Value>(context_dict_[var_name.c_str()]);
    }
    return Value::Null();
}

bool PyExprContext::IsContextValueExist(const std::string &var_name) const
{
    py::gil_scoped_acquire acquire;
    return context_dict_.contains(var_name);
}

std::unordered_set<std::string> PyExprContext::GetContextExistVariables() const
{
    py::gil_scoped_acquire acquire;

    py::object keys_obj = context_dict_.attr("keys")();
    std::unordered_set<std::string> keys;

    for (auto key : keys_obj)
    {
        keys.insert(key.cast<std::string>());
    }
    return keys;
}

void PyExprContext::SetContextValue(const std::string &var_name, const Value &value)
{
    py::gil_scoped_acquire acquire;

    context_dict_[var_name.c_str()] = value;
}

bool PyExprContext::RemoveContextValue(const std::string &var_name)
{
    py::gil_scoped_acquire acquire;

    if (context_dict_.contains(var_name))
    {
        context_dict_.attr("__delitem__")(var_name);
        return true;
    }
    return false;
}

void PyExprContext::ClearContextValue() 
{
    py::gil_scoped_acquire acquire;
    
    context_dict_.clear();
}