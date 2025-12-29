#include "value.h"
#include <memory>

using namespace xequation;

std::unordered_map<std::type_index, std::vector<Value::BeforeConstructCallback>>
    Value::before_construct_callbacks_by_type_;
std::unordered_map<std::type_index, std::vector<Value::AfterConstructCallback>>
    Value::after_construct_callbacks_by_type_;
std::unordered_map<std::type_index, std::vector<Value::BeforeDestructCallback>>
    Value::before_destruct_callbacks_by_type_;
std::unordered_map<std::type_index, std::vector<Value::AfterDestructCallback>>
    Value::after_destruct_callbacks_by_type_;
std::mutex Value::callbacks_mutex_;

void Value::NotifyBeforeConstruct(const std::type_info &typeInfo)
{
    std::vector<BeforeConstructCallback> cbs;
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto it = before_construct_callbacks_by_type_.find(std::type_index(typeInfo));
        if (it != before_construct_callbacks_by_type_.end())
        {
            cbs = it->second;
        }
    }
    for (auto &cb : cbs)
    {
        cb(typeInfo);
    }
}

void Value::NotifyAfterConstruct(const Value &value)
{
    std::vector<AfterConstructCallback> cbs;
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto it = after_construct_callbacks_by_type_.find(std::type_index(value.Type()));
        if (it != after_construct_callbacks_by_type_.end())
        {
            cbs = it->second;
        }
    }
    for (auto &cb : cbs)
    {
        cb(value);
    }
}

void Value::NotifyBeforeDestruct(const Value &value)
{
    std::vector<BeforeDestructCallback> cbs;
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto it = before_destruct_callbacks_by_type_.find(std::type_index(value.Type()));
        if (it != before_destruct_callbacks_by_type_.end())
        {
            cbs = it->second;
        }
    }
    for (auto &cb : cbs)
    {
        cb(value);
    }
}

void Value::NotifyAfterDestruct(const std::type_info &typeInfo)
{
    std::vector<AfterDestructCallback> cbs;
    {
        std::lock_guard<std::mutex> lock(callbacks_mutex_);
        auto it = after_destruct_callbacks_by_type_.find(std::type_index(typeInfo));
        if (it != after_destruct_callbacks_by_type_.end())
        {
            cbs = it->second;
        }
    }
    for (auto &cb : cbs)
    {
        cb(typeInfo);
    }
}

Value::Value(const Value &other)
{
    NotifyBeforeConstruct(other.Type());
    value_ptr_ = other.value_ptr_ != nullptr ? other.value_ptr_->Clone() : nullptr;
    NotifyAfterConstruct(other);
}

Value& Value::operator=(const Value &other)
{
    if(&other != this)
    {
        Value(other).swap(*this);
    }
    return *this;
}

Value::Value(Value &&other) noexcept
{
    // Begin construction using other's type
    NotifyBeforeConstruct(other.Type());

    // Trigger destructor callbacks for other's original content (before transfer)
    const std::type_info &originalType = other.Type();
    NotifyBeforeDestruct(other);

    // Transfer ownership to this instance
    value_ptr_ = std::move(other.value_ptr_);

    // Reset other to a valid null immediately to avoid null deref during callbacks
    other.value_ptr_.reset(new ValueHolder<void>());

    // Trigger destructor callbacks after transfer using original type
    NotifyAfterDestruct(originalType);

    // Construction finished callback using moved-from other
    NotifyAfterConstruct(other);
}

Value &Value::operator=(Value &&other) noexcept
{
    if (&other != this)
    {
        // Trigger destructor callbacks for other's original content (before transfer)
        const std::type_info &originalType = other.Type();
        NotifyBeforeDestruct(other);

        // Transfer ownership
        value_ptr_ = std::move(other.value_ptr_);

        // Reset other to a valid null to avoid null deref during callbacks
        other.value_ptr_.reset(new ValueHolder<void>());

        // Trigger destructor callbacks after transfer
        NotifyAfterDestruct(originalType);
    }
    return *this;
}

bool Value::operator==(const Value &other) const
{
    if (IsNull() != other.IsNull())
        return false;
    if (IsNull())
        return true;

    if (value_ptr_->Type() != other.value_ptr_->Type())
        return false;

    return ToString() == other.ToString();
}

void Value::swap(Value& other) noexcept
{
    using std::swap;
    swap(value_ptr_, other.value_ptr_);
}

bool Value::operator<(const Value &other) const
{
    if (IsNull() != other.IsNull())
    {
        return IsNull();
    }

    if (IsNull())
        return false;

    if (value_ptr_->Type() != other.value_ptr_->Type())
    {
        return value_ptr_->Type().before(other.value_ptr_->Type());
    }

    return ToString() < other.ToString();
}

bool Value::operator!=(const Value &other) const
{
    return !(*this == other);
}
bool Value::operator>(const Value &other) const
{
    return other < *this;
}
bool Value::operator<=(const Value &other) const
{
    return !(other < *this);
}
bool Value::operator>=(const Value &other) const
{
    return !(*this < other);
}

bool Value::IsNull() const
{
    return value_ptr_->IsNull();
}

const std::type_info &Value::Type() const
{
    return value_ptr_->Type();
}

std::string Value::ToString() const
{
    return value_ptr_.get()->ToString();
}

Value::~Value() noexcept
{
    const std::type_info &typeInfo = value_ptr_ ? value_ptr_->Type() : typeid(void);
    NotifyBeforeDestruct(*this);
    // Ensure underlying storage is released before after-destruction callbacks
    std::unique_ptr<ValueBase> old;
    old.swap(value_ptr_);
    old.reset();
    NotifyAfterDestruct(typeInfo);
}