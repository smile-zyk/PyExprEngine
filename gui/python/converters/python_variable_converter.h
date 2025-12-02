#pragma once

#include <QString>
#include <memory>
#include <vector>

#include "python/python_qt_wrapper.h"

namespace xequation
{
namespace gui
{
class Variable;
class VariableManager;

namespace python
{

class PythonVariableConverter
{
  public:
    virtual ~PythonVariableConverter() = default;

    virtual bool CanConvert(pybind11::handle obj) const = 0;

    virtual Variable *
    CreateData(VariableManager *manager, const QString &name, pybind11::handle obj);

    static QString GetTypeName(pybind11::handle obj, bool qualified = false);
    static QString GetObjectStr(pybind11::handle obj);

    PythonVariableConverter(const PythonVariableConverter &) = delete;
    PythonVariableConverter &operator=(const PythonVariableConverter &) = delete;
    PythonVariableConverter(PythonVariableConverter &&) = delete;
    PythonVariableConverter &operator=(PythonVariableConverter &&) = delete;

  protected:
    PythonVariableConverter() = default;
};

class PythonVariableConverterRegistry
{
  public:
    static PythonVariableConverterRegistry &GetInstance();

    void RegisterConverter(std::unique_ptr<PythonVariableConverter> converter, int priority = 0);

    void UnRegisterConverter(PythonVariableConverter *converter);

    PythonVariableConverter *FindConverter(pybind11::handle obj);
    Variable *CreateData(VariableManager *manager, const QString &name, pybind11::handle obj);

    void Clear();

  private:
    PythonVariableConverterRegistry() = default;
    ~PythonVariableConverterRegistry() = default;

    PythonVariableConverterRegistry(const PythonVariableConverterRegistry &) = delete;
    PythonVariableConverterRegistry &operator=(const PythonVariableConverterRegistry &) = delete;
    PythonVariableConverterRegistry(PythonVariableConverterRegistry &&) = delete;
    PythonVariableConverterRegistry &operator=(PythonVariableConverterRegistry &&) = delete;

  private:
    struct ConverterEntry
    {
        std::unique_ptr<PythonVariableConverter> converter;
        int priority;

        bool operator<(const ConverterEntry &other) const
        {
            return priority > other.priority;
        }
    };

    std::vector<ConverterEntry> converters_;
};

inline void RegisterPythonVariableConverter(std::unique_ptr<PythonVariableConverter> converter, int priority = 0)
{
    PythonVariableConverterRegistry::GetInstance().RegisterConverter(std::move(converter), priority);
}

inline void UnRegisterPythonVariableConverter(PythonVariableConverter *converter)
{
    PythonVariableConverterRegistry::GetInstance().UnRegisterConverter(converter);
}

inline Variable *
CreatePythonVariableData(VariableManager *manager, const QString &name, pybind11::handle obj)
{
    return PythonVariableConverterRegistry::GetInstance().CreateData(manager, name, obj);
}

inline PythonVariableConverter *FindPythonVariableConverter(pybind11::handle obj)
{
    return PythonVariableConverterRegistry::GetInstance().FindConverter(obj);
}

template <typename T>
class PythonVariableConverterAutoRegister
{
  public:
    PythonVariableConverterAutoRegister(int priority = 0)
    {
        RegisterPythonVariableConverter(std::make_unique<T>(), priority);
    }
};

#define REGISTER_PYTHON_VARIABLE_DATA_CONVERTER_WITH_PRIORITY(ConverterClass, priority)        \
    static xequation::gui::python::PythonVariableConverterAutoRegister<ConverterClass>     \
        s_autoRegister_##ConverterClass(priority)

#define REGISTER_PYTHON_VARIABLE_DATA_CONVERTER(ConverterClass)                                \
    static xequation::gui::python::PythonVariableConverterAutoRegister<ConverterClass>     \
        s_autoRegister_##ConverterClass(0)

} // namespace python
} // namespace gui
} // namespace xequation