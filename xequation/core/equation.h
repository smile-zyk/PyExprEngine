#pragma once

#include <boost/uuid/uuid.hpp>
#include <memory>
#include <string>
#include <vector>

#include <boost/uuid.hpp>
#include <tsl/ordered_map.h>

#include "value.h"

namespace xequation
{
class Equation;
class EquationGroup;
class EquationManager;

using EquationGroupId = boost::uuids::uuid;
using EquationPtrOrderedMap = tsl::ordered_map<std::string, std::unique_ptr<Equation>>;
using EquationGroupPtrOrderedMap = tsl::ordered_map<boost::uuids::uuid, std::unique_ptr<EquationGroup>>;

class EquationObserver
{
  public:
    enum class ChangeType
    {
      kContent,
      kType,
      kStatus,
      kMessage,
      kDependencies,
      kValue
    };

    virtual ~EquationObserver() = default;
    virtual void OnEquationFieldChanged(const Equation* equation, ChangeType change_type) = 0;
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

    explicit Equation(const std::string &name, const boost::uuids::uuid& group_id, EquationManager *manager);
    virtual ~Equation() = default;

    void SetContent(const std::string &content);
    void SetDependencies(const std::vector<std::string> &dependencies);
    void SetType(Type type);
    void SetStatus(Status status);
    void SetMessage(const std::string &message);
    void UpdateValue();

    const std::string& name() const
    {
        return name_;
    }

    const std::string& content() const
    {
        return content_;
    }

    const std::vector<std::string> dependencies() const
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

    const EquationManager* manager() const
    {
        return manager_;
    }

    const EquationGroupId& group_id() const
    {
        return group_id_;
    }

    Value GetValue();

    bool operator==(const Equation &other) const;
    bool operator!=(const Equation &other) const;

    void NotifyValueChanged();

    static Type StringToType(const std::string &type_str);
    static Status StringToStatus(const std::string &status_str);
    static std::string TypeToString(Type type);
    static std::string StatusToString(Status status);

  private:
    void NotifyObserversFieldChanged(EquationObserver::ChangeType change_type) const;
  
  private:
    std::string name_;
    std::string content_;
    Type type_ = Type::kError;
    Status status_ = Status::kInit;
    std::string message_;
    std::vector<std::string> dependencies_;
    EquationGroupId group_id_;
    EquationManager *manager_ = nullptr;
    std::vector<EquationObserver *> observers_;
};

class EquationGroup
{
  public:
    EquationGroup(const EquationManager *manager);

    void AddEquation(std::unique_ptr<Equation> equation);

    void RemoveEquation(const std::string& equation_name);

    const Equation* GetEquation(const std::string& equation_name);

    const EquationManager* manaegr()
    {
        return manager_;
    }

    const EquationPtrOrderedMap& equation_map()
    {
        return equation_map_;
    }

    const EquationGroupId& id()
    {
        return id_;
    }

  private:
    EquationPtrOrderedMap equation_map_;
    EquationGroupId id_;
    const EquationManager* manager_;
};

std::ostream &operator<<(std::ostream &os, Equation::Type type);
std::ostream &operator<<(std::ostream &os, Equation::Status status);

} // namespace xequation