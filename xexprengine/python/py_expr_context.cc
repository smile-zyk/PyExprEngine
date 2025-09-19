#include "py_expr_context.h"
#include "py_expr_engine.h"

using namespace xexprengine;

PyExprContext::PyExprContext() 
{
    auto evaluate_callback = [this](const std::string& expression) -> EvalResult
    {
        return PyExprEngine::GetInstance().Evaluate(expression, this);
    };

    auto parse_callback = [this](const std::string& expression) -> ParseResult
    {
        return PyExprEngine::GetInstance().Parse(expression);
    };

    set_evaluate_callback(evaluate_callback);
    set_parse_callback(parse_callback);
}