#pragma once
#include "python_property_converter.h"

namespace xequation
{
namespace gui
{
namespace python
{
class DefaultPropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
};

class BasicPropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
};

class ListPropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class TuplePropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class SetPropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};

class DictPropertyConverter : public PythonPropertyConverter
{
  public:
    bool CanConvert(pybind11::handle obj) const override;
    VariableProperty *CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj) override;
};
} // namespace python
} // namespace gui
} // namespace xequation