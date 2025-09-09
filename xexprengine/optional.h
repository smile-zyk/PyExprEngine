#include <type_traits>
#include <utility>
#include <stdexcept>

namespace xexprengine {

class BadOptionalAccess : public std::logic_error {
public:
    BadOptionalAccess() : std::logic_error("Bad optional access") {}
};

struct NullOptType
{
};

constexpr NullOptType NullOpt;

template <typename T>
class Optional {
public:
    Optional(const NullOptType&) noexcept : has_value_(false) {}

    Optional(const T& value) : has_value_(false) {
        construct(value);
    }

    Optional(T&& value) noexcept(std::is_nothrow_move_constructible<T>::value) : has_value_(false) {
        construct(std::move(value));
    }

    Optional(const Optional& other) : has_value_(false) {
        if (other.has_value_) {
            construct(other.value());
        }
    }

    Optional(Optional&& other) noexcept(std::is_nothrow_move_constructible<T>::value) : has_value_(false) {
        if (other.has_value_) {
            construct(std::move(other.value()));
            other.destruct();
        }
    }

    ~Optional() {
        destruct();
    }

    Optional& operator=(const Optional& other) {
        if (this != &other) {
            if (other.has_value_) {
                if (has_value_) {
                    value() = other.value();
                } else {
                    construct(other.value());
                }
            } else {
                destruct();
            }
        }
        return *this;
    }

    Optional& operator=(Optional&& other) noexcept(std::is_nothrow_move_constructible<T>::value && std::is_nothrow_move_assignable<T>::value) {
        if (this != &other) {
            if (other.has_value_) {
                if (has_value_) {
                    value() = std::move(other.value());
                } else {
                    construct(std::move(other.value()));
                }
                other.destruct();
            } else {
                destruct();
            }
        }
        return *this;
    }

    explicit operator bool() const noexcept {
        return has_value_;
    }

    bool has_value() const noexcept {
        return has_value_;
    }

    T& value() {
        if (!has_value_) {
            throw BadOptionalAccess(); 
        }
        return *reinterpret_cast<T*>(storage_);
    }

    const T& value() const {
        if (!has_value_) {
            throw BadOptionalAccess();
        }
        return *reinterpret_cast<const T*>(storage_);
    }

    template <typename U>
    T value_or(U&& default_value) const& {
        static_assert(std::is_convertible<U, T>::value, "U must be convertible to T");
        return has_value_ ? value() : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename U>
    T value_or(U&& default_value) && {
        static_assert(std::is_convertible<U, T>::value, "U must be convertible to T");
        return has_value_ ? std::move(value()) : static_cast<T>(std::forward<U>(default_value));
    }

    template <typename... Args>
    void emplace(Args&&... args) {
        destruct(); 
        construct(std::forward<Args>(args)...);
    }

    void reset() noexcept {
        destruct();
    }

private:
    template <typename... Args>
    void construct(Args&&... args) {
        new (storage_) T(std::forward<Args>(args)...);
        has_value_ = true;
    }

    void destruct() noexcept {
        if (has_value_) {
            reinterpret_cast<T*>(storage_)->~T();
            has_value_ = false;
        }
        
    }

    typename std::aligned_storage<sizeof(T), alignof(T)>::type storage_;
    bool has_value_;
};

} // namespace xexprengine