#include "equation.h"
#include "equation_manager.h"

namespace xequation
{

bool Equation::operator==(const Equation &other) const
{
    return name_ == other.name_ && content_ == other.content_ && dependencies_ == other.dependencies_ &&
           type_ == other.type_ && status_ == other.status_ && message_ == other.message_;
}

bool Equation::operator!=(const Equation &other) const
{
    return !(*this == other);
}

void Equation::SetContent(const std::string &content)
{
    content_ = content;
    NotifyObservers("content", content_);
}
void Equation::SetDependencies(const std::vector<std::string> &dependencies)
{
    dependencies_ = dependencies;
    NotifyObservers("dependencies", Value::FromVector(dependencies_));
}

void Equation::SetType(Type type)
{
    type_ = type;
    NotifyObservers("type", Value::FromString(TypeToString(type_)));
}

void Equation::SetStatus(Status status)
{
    status_ = status;
    NotifyObservers("status", Value::FromString(StatusToString(status_)));
}

void Equation::NotifyObservers(const std::string &field_name, const Value& new_value)
{
    for (auto observer : observers_)
    {
        observer->OnEquationUpdated(name_, field_name, &new_value);
    }
}

Value Equation::GetValue()
{
    return manager_->context()->Get(name_);
}

Equation::Type Equation::StringToType(const std::string &type_str)
{
    if (type_str == "Variable")
        return Type::kVariable;
    else if (type_str == "Function")
        return Type::kFunction;
    else if (type_str == "Class")
        return Type::kClass;
    else if (type_str == "Import")
        return Type::kImport;
    else if (type_str == "ImportFrom")
        return Type::kImportFrom;
    else
        return Type::kError;
}

Equation::Status Equation::StringToStatus(const std::string &status_str)
{
    if (status_str == "Init")
        return Status::kInit;
    else if (status_str == "Success")
        return Status::kSuccess;
    else if (status_str == "SyntaxError")
        return Status::kSyntaxError;
    else if (status_str == "NameError")
        return Status::kNameError;
    else if (status_str == "TypeError")
        return Status::kTypeError;
    else if (status_str == "ZeroDivisionError")
        return Status::kZeroDivisionError;
    else if (status_str == "ValueError")
        return Status::kValueError;
    else if (status_str == "MemoryError")
        return Status::kMemoryError;
    else if (status_str == "OverflowError")
        return Status::kOverflowError;
    else if (status_str == "RecursionError")
        return Status::kRecursionError;
    else if (status_str == "IndexError")
        return Status::kIndexError;
    else if (status_str == "KeyError")
        return Status::kKeyError;
    else if (status_str == "AttributeError")
        return Status::kAttributeError;
    else
        return Status::kInit;
}

std::string Equation::TypeToString(Type type)
{
    switch (type)
    {
    case Type::kVariable:
        return "Variable";
    case Type::kFunction:
        return "Function";
    case Type::kClass:
        return "Class";
    case Type::kImport:
        return "Import";
    case Type::kImportFrom:
        return "ImportFrom";
    default:
        return "Error";
    }
}

std::string Equation::StatusToString(Status status)
{
    switch (status)
    {
    case Status::kInit:
        return "Init";
    case Status::kSuccess:
        return "Success";
    case Status::kSyntaxError:
        return "SyntaxError";
    case Status::kNameError:
        return "NameError";
    case Status::kTypeError:
        return "TypeError";
    case Status::kZeroDivisionError:
        return "ZeroDivisionError";
    case Status::kValueError:
        return "ValueError";
    case Status::kMemoryError:
        return "MemoryError";
    case Status::kOverflowError:
        return "OverflowError";
    case Status::kRecursionError:
        return "RecursionError";
    case Status::kIndexError:
        return "IndexError";
    case Status::kKeyError:
        return "KeyError";
    case Status::kAttributeError:
        return "AttributeError";
    default:
        return "Unknown";
    }
}

} // namespace xequation