#include <string>
#include <typeinfo>

#include "value_helper.hpp"

namespace xexprengine 
{
    class ValueBase {
    public:
        virtual ~ValueBase() = default;
        virtual const std::type_info& Type() const = 0;
        virtual std::string ToString() const = 0;
        virtual bool IsNull() const { return false; } 
    };


    template <typename T>
    class Value : public ValueBase {
    public:
        explicit Value(const T& val) : value_(val) {}
        const std::type_info& Type() const override { return typeid(T); }
        std::string ToString() const override
        {
            return ValueHelper::ToString(value_);
        }
        bool IsNull() const override { return false; }
        const T& value() const { return value_; }
    private:
        T value_;
    };

    template <>
    class Value<void> : public ValueBase {
    public:
        Value() = default;

        const std::type_info& Type() const override {
            return typeid(void);
        }
        std::string ToString() const override {
            return "null";
        }
        bool IsNull() const override { return true; }
    };

    using NullValue = Value<void>;
}