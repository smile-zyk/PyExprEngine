#pragma once

#include <string>
#include <vector>

#include "value.h"

namespace xequation
{
class EquationManager;

class Equation;

class EquationObserver
{
  public:
    virtual ~EquationObserver() = default;
    virtual void
    OnEquationUpdated(const std::string &equation_name, const std::string &field_name, const Value *new_value) = 0;
    virtual void OnEquationRemoved(const std::string &equation_name) = 0;
};

class Equation
{
  public:
    enum class Type
    {
        kError,
        kVariable,
        kFunction,
        kClass,
        kImport,
        kImportFrom,
    };

    enum class Status
    {
        kInit,
        kSuccess,
        kSyntaxError,
        kNameError,
        kTypeError,
        kZeroDivisionError,
        kValueError,
        kMemoryError,
        kOverflowError,
        kRecursionError,
        kIndexError,
        kKeyError,
        kAttributeError,
    };

    Equation() = default;
    Equation(const std::string &name, EquationManager *manager) : name_(name), manager_(manager) {}
    virtual ~Equation() = default;

    const std::string &name() const
    {
        return name_;
    }

    void SetContent(const std::string &content);
    void SetDependencies(const std::vector<std::string> &dependencies);
    void SetType(Type type);
    void SetStatus(Status status);
    void SetMessage(const std::string &message);

    const std::string &content() const
    {
        return content_;
    }
    const std::vector<std::string> &dependencies() const
    {
        return dependencies_;
    }
    Type type() const
    {
        return type_;
    }
    Status status() const
    {
        return status_;
    }
    const std::string &message() const
    {
        return message_;
    }

    Value GetValue();

    bool operator==(const Equation &other) const;
    bool operator!=(const Equation &other) const;

    void NotifyObservers(const std::string &field_name, const Value& new_value);

    static Type StringToType(const std::string &type_str);
    static Status StringToStatus(const std::string &status_str);
    static std::string TypeToString(Type type);
    static std::string StatusToString(Status status);

  private:
    std::string name_;
    std::string content_;
    std::vector<std::string> dependencies_;
    Type type_ = Type::kError;
    Status status_ = Status::kInit;
    std::string message_;
    EquationManager *manager_ = nullptr;
    std::vector<EquationObserver *> observers_;
};
} // namespace xequation