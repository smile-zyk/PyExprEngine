#pragma once
#include <stdexcept>

namespace xexprengine
{
// a simple optional implement in c++ 11
template <typename T>
class Optional
{
  public:
    Optional() : has_value_(false) {}

    Optional(const T &value) : has_value_(true)
    {
        new (storage_) T(value);
    }

    Optional(T &&value) : has_value_(true)
    {
        new (storage_) T(std::move(value));
    }

    ~Optional()
    {
        if (has_value_)
        {
            reinterpret_cast<T *>(storage_)->~T();
        }
    }

    explicit operator bool() const {
        return has_value_;
    }
    
    T &value()
    {
        if (!has_value_)
            throw std::logic_error("No value");
        return *reinterpret_cast<T *>(storage_);
    }

  private:
    alignas(T) char storage_[sizeof(T)];
    bool has_value_;
};
} // namespace xexprengine