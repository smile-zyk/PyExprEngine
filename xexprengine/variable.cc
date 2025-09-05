#include "variable.h"
#include "expr_common.h"
#include "expr_context.h"
#include "value.h"

using namespace xexprengine;

Value Variable::GetValue() const
{
    if (context_ != nullptr)
        return context_->GetValue(this);
    return Value::Null();
}

Value ExprVariable::Evaluate()
{
    const ExprContext *ctx = context();
    if (ctx != nullptr)
    {
        EvalResult result = ctx->Evaluate(expression_);
        error_code_ = result.error_code;
        error_message_ = result.error_message;
        return result.value;
    }
    return Value::Null();
}

void ExprVariable::Parse()
{
    const ExprContext *ctx = context();
    if (ctx != nullptr)
    {
        ParseResult parse_result = ctx->Parse(expression_);
        parse_result_ = parse_result;
    }
}