#pragma once
#include "expr_common.h"
#include <string>

namespace xexprengine
{
class Variable
{
  public:
    Variable(const std::string &name) : name_(name) {}
    virtual ~Variable() = default;

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

    void set_type(ParseType type)
    {
        type_ = type;
    }

    ParseType type() const
    {
        return type_;
    }

    void set_status(ExecStatus status)
    {
        status_ = status;
    }

    ExecStatus status() const
    {
        return status_;
    }

  private:
    std::string name_;
    std::string content_;
    std::vector<std::string> dependencies_;
    ParseType type_;
    ExecStatus status_;
    std::string message_;
};
} // namespace xexprengine