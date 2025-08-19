#include "exprvalue.h"
#include <utility>

ExprValue::ExprValue(ExprValue&& other)
{
    value_ptr_ = std::move(other.value_ptr_);
}

bool ExprValue::operator==(const ExprValue& other) const
{
    if (IsNull() && other.IsNull()) return true;
    if (IsNull() || other.IsNull()) return false;
    return value_ptr_->Type() == other.value_ptr_->Type() &&
           ToString() == other.ToString();
}

bool ExprValue::operator<(const ExprValue& other) const
{
    if (IsNull() && other.IsNull()) return false;
    if (IsNull()) return true;
    if (other.IsNull()) return false;
    return value_ptr_->Type().before(other.value_ptr_->Type()) ||
           (value_ptr_->Type() == other.value_ptr_->Type() && ToString() < other.ToString());
}

ExprValue& ExprValue::operator=(ExprValue&& other)
{
    if(&other != this)
    {
        value_ptr_ = std::move(other.value_ptr_);
    }
    return *this;
}

bool ExprValue::IsNull() const
{
    return value_ptr_->IsNull();
}

const std::type_info& ExprValue::Type() const
{
    return value_ptr_->Type();
}

std::string ExprValue::ToString() const
{
    return value_ptr_.get()->ToString();
}