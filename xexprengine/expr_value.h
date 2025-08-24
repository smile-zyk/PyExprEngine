#pragma once
#include "value.hpp"

#include <stdexcept>
#include <memory>

namespace xexprengine 
{
    class ExprValue {
    public:
        ExprValue() noexcept : value_ptr_(new NullValue())  {}
        ~ExprValue() noexcept{}

        template <typename T>
        ExprValue(const T& val) noexcept : value_ptr_(new Value<T>(val)) {}

        ExprValue(const char* val) noexcept : value_ptr_(new Value<std::string>(val)) {}

        static ExprValue Null() {
            return ExprValue();
        }

        operator bool() const {
            return !IsNull();
        }

        bool operator==(const ExprValue& other) const;

        bool operator<(const ExprValue& other) const;

        ExprValue(const ExprValue& other) = default;
        ExprValue& operator=(const ExprValue& other) = default;

        ExprValue(ExprValue&&);
        ExprValue& operator=(ExprValue&&);

        bool IsNull() const;

        const std::type_info& Type() const;

        template <typename T>
        T Cast() const {
            if (IsNull()) throw std::runtime_error("Cannot cast null");
            auto derived = dynamic_cast<Value<T>*>(value_ptr_.get());
            if (!derived) throw std::runtime_error("Bad cast");
            return derived->value();
        }

        std::string ToString() const;

    private:
        std::shared_ptr<ValueBase> value_ptr_;
    };
}

namespace std {
    template <>
    struct hash<xexprengine::ExprValue> {
        size_t operator()(const xexprengine::ExprValue& value) const {
            return std::hash<std::string>()(value.ToString());
        }
    };
}
