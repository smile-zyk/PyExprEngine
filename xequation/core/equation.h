#pragma once

#include <memory>
#include <string>
#include <vector>
#include <unordered_map>

#include "value.h"

namespace xequation
{
class EquationManager;

class Equation;

class EquationObserver
{
  public:
    virtual ~EquationObserver() = default;
    virtual void OnEquationFieldChanged(const Equation *equation, const std::string &field_name) = 0;
};

class EquationBase
{
  public:
    enum class Category
    {
        kSingle,
        kGroup
    };

    EquationBase() = default;
    EquationBase(const std::string &id, EquationManager *manager, Category category)
        : id_(id), manager_(manager)
    {
    }

    const std::string &id() const
    {
        return id_;
    }

    const std::string &content() const
    {
        return content_;
    }

    const std::vector<std::string> &dependencies() const
    {
        return dependencies_;
    }

    EquationManager *manager() const
    {
        return manager_;
    }

    virtual Category category() const = 0;

  protected:
    std::string id_;
    std::string content_;
    std::vector<std::string> dependencies_;
    EquationManager *manager_ = nullptr;
    std::string group_id_;
};

class Equation : public EquationBase
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
    Equation(const std::string &id, EquationManager *manager)
        : EquationBase(id, manager, EquationBase::Category::kSingle)
    {
    }

    virtual ~Equation() = default;

    void SetContent(const std::string &content);
    void SetDependencies(const std::vector<std::string> &dependencies);
    void SetType(Type type);
    void SetStatus(Status status);
    void SetMessage(const std::string &message);
    void UpdateValue();

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

    Category category() const override
    {
        return EquationBase::Category::kSingle;
    }

    Value GetValue();

    const EquationBase *GetDependencyEquation(const std::string &equation_name);

    bool operator==(const Equation &other) const;
    bool operator!=(const Equation &other) const;

    void NotifyObserversFieldChanged(const std::string &field_name) const;

    static Type StringToType(const std::string &type_str);
    static Status StringToStatus(const std::string &status_str);
    static std::string TypeToString(Type type);
    static std::string StatusToString(Status status);

  private:
    Type type_ = Type::kError;
    Status status_ = Status::kInit;
    std::string message_;
    std::vector<EquationObserver *> observers_;
};

class EquationGroup : public EquationBase
{
  public:
    EquationGroup(const std::string &id, EquationManager *manager)
        : EquationBase(id, manager, EquationBase::Category::kGroup)
    {
    }

    void AddSubEquation(const std::string& sub_equation_name, std::unique_ptr<Equation> sub_equation);

    void RemoveSubEquation(const std::string& sub_equation_name);

    Category category() const override
    {
        return EquationBase::Category::kGroup;
    }

    static std::string GenerateGroupId();

  private:
    std::unordered_map<std::string, std::unique_ptr<Equation>> sub_equations_map;
};

std::ostream &operator<<(std::ostream &os, Equation::Type type);
std::ostream &operator<<(std::ostream &os, Equation::Status status);

} // namespace xequation