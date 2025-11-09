#pragma once
#include <string>
#include <vector>


namespace xequation
{
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
    Equation(const std::string &name) : name_(name) {}
    virtual ~Equation() = default;

    void set_name(const std::string &name)
    {
        name_ = name;
    }

    const std::string &name() const
    {
        return name_;
    }

    void set_content(const std::string &content)
    {
        content_ = content;
    }

    const std::string &content() const
    {
        return content_;
    }

    void set_dependencies(const std::vector<std::string> &dependencies)
    {
        dependencies_ = dependencies;
    }

    const std::vector<std::string> &dependencies() const
    {
        return dependencies_;
    }

    void set_type(Type type)
    {
        type_ = type;
    }

    Type type() const
    {
        return type_;
    }

    void set_status(Status status)
    {
        status_ = status;
    }

    Status status() const
    {
        return status_;
    }

    void set_message(const std::string &message)
    {
        message_ = message;
    }

    const std::string &message() const
    {
        return message_;
    }

    bool operator==(const Equation &other) const
    {
        return name_ == other.name_ && content_ == other.content_ && dependencies_ == other.dependencies_ &&
               type_ == other.type_ && status_ == other.status_ && message_ == other.message_;
    }

    bool operator!=(const Equation &other) const
    {
        return !(*this == other);
    }

    static Type StringToType(const std::string &type_str)
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

    static Status StringToStatus(const std::string &status_str)
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

    static std::string TypeToString(Type type)
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

    static std::string StatusToString(Status status)
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

  private:
    std::string name_;
    std::string content_;
    std::vector<std::string> dependencies_;
    Type type_ = Type::kError;
    Status status_ = Status::kInit;
    std::string message_;
};
} // namespace xequation