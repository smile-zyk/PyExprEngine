#include "basic_python_converter.h"

#include <QtVariantPropertyManager>

#include "variable_property_manager.h"

namespace xequation
{
namespace gui
{
namespace python
{
bool DefaultPropertyConverter::CanConvert(pybind11::handle) const
{
    return true;
}

bool BasicPropertyConverter::CanConvert(pybind11::handle obj) const
{
    pybind11::handle complex_type = pybind11::module_::import("builtins").attr("complex");
    return pybind11::isinstance<pybind11::int_>(obj) || pybind11::isinstance<pybind11::float_>(obj) ||
           pybind11::isinstance<pybind11::str>(obj) || pybind11::isinstance<pybind11::bytes>(obj) ||
           pybind11::isinstance<pybind11::bytearray>(obj) || pybind11::isinstance<pybind11::memoryview>(obj) ||
           pybind11::isinstance(obj, complex_type) || pybind11::isinstance<pybind11::bool_>(obj) || obj.is_none();
}

bool ListPropertyConverter::CanConvert(pybind11::handle obj) const
{
    return pybind11::isinstance<pybind11::list>(obj);
}

VariableProperty *
ListPropertyConverter::CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj)
{
    QString type_name = PythonPropertyConverter::GetTypeName(obj);

    VariableProperty *property = manager->addProperty(name);
    pybind11::list list_obj = obj.cast<pybind11::list>();
    pybind11::size_t length = list_obj.size();
    QString value_str = QString("{size = %1}").arg(length);

    manager->setValue(property, value_str);
    manager->setType(property, type_name);

    for (pybind11::size_t i = 0; i < length; ++i)
    {
        pybind11::handle item_obj = list_obj[i];

        QString item_name = QString("[%1]").arg(i);
        VariableProperty *item_property = CreatePythonProperty(manager, item_name, item_obj);
        property->addSubProperty(item_property);
    }

    return property;
}

bool TuplePropertyConverter::CanConvert(pybind11::handle obj) const
{
    return pybind11::isinstance<pybind11::tuple>(obj);
}

VariableProperty *
TuplePropertyConverter::CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj)
{
    QString type_name = PythonPropertyConverter::GetTypeName(obj);

    VariableProperty *property = manager->addProperty(name);
    pybind11::tuple tuple_obj = obj.cast<pybind11::tuple>();
    pybind11::size_t length = tuple_obj.size();
    QString value_str = QString("{size = %1}").arg(length);

    manager->setValue(property, value_str);
    manager->setType(property, type_name);

    for (pybind11::size_t i = 0; i < length; ++i)
    {
        pybind11::handle item_obj = tuple_obj[i];

        QString item_name = QString("[%1]").arg(i);
        VariableProperty *item_property = CreatePythonProperty(manager, item_name, item_obj);
        property->addSubProperty(item_property);
    }

    return property;
}

bool SetPropertyConverter::CanConvert(pybind11::handle obj) const
{
    return pybind11::isinstance<pybind11::set>(obj);
}

VariableProperty *
SetPropertyConverter::CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj)
{
    QString type_name = PythonPropertyConverter::GetTypeName(obj);

    VariableProperty *property = manager->addProperty(name);
    pybind11::set set_obj = obj.cast<pybind11::set>();
    pybind11::size_t size = set_obj.size();
    QString value_str = QString("{size = %1}").arg(size);

    manager->setValue(property, value_str);
    manager->setType(property, type_name);

    pybind11::size_t index = 0;
    for (auto item : set_obj)
    {
        pybind11::handle item_obj = item;

        QString item_name = QString("[%1]").arg(index++);
        VariableProperty *item_property = CreatePythonProperty(manager, item_name, item_obj);
        property->addSubProperty(item_property);
    }

    return property;
}

bool DictPropertyConverter::CanConvert(pybind11::handle obj) const
{
    return pybind11::isinstance<pybind11::dict>(obj);
}

VariableProperty *
DictPropertyConverter::CreateProperty(VariablePropertyManager *manager, const QString &name, pybind11::handle obj)
{
    QString type_name = PythonPropertyConverter::GetTypeName(obj);

    VariableProperty *property = manager->addProperty(name);
    pybind11::dict dict_obj = obj.cast<pybind11::dict>();
    pybind11::size_t size = dict_obj.size();
    QString value_str = QString("{size = %1}").arg(size);

    manager->setValue(property, value_str);
    manager->setType(property, type_name);

    for (auto item : dict_obj)
    {
        pybind11::handle key_obj = item.first;
        pybind11::handle value_obj = item.second;

        QString key_str = PythonPropertyConverter::GetObjectStr(key_obj);
        QString value_str = PythonPropertyConverter::GetObjectStr(value_obj);
        VariableProperty *item_property = manager->addProperty(key_str);
        manager->setValue(item_property, value_str);
        property->addSubProperty(item_property);
        VariableProperty *key_property = CreatePythonProperty(manager, "key", key_obj);
        VariableProperty *value_property = CreatePythonProperty(manager, "value", value_obj);
        item_property->addSubProperty(key_property);
        item_property->addSubProperty(value_property);
    }
    return property;
}

} // namespace python
} // namespace gui
} // namespace xequation