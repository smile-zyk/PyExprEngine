#pragma once
#include "python_variable_converter.h"

namespace xequation
{
namespace gui
{
namespace python
{
class DefaultVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
};

class BasicVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
};

class ListVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class TupleVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class SetVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class DictVariableConverter : public PythonVariableConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};
} // namespace python
} // namespace gui
} // namespace xequation