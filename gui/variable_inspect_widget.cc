#include "variable_inspect_widget.h"
#include "python/converters/basic_python_converter.h"
#include "python/converters/python_property_converter.h"


#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
namespace python
{
REGISTER_PYTHON_PROPERTY_CONVERTER(BasicPythonPropertyConverter, -100);
REGISTER_PYTHON_PROPERTY_CONVERTER(DefaultPythonPropertyConverter, 100);
} // namespace python
VariableInspectWidget::VariableInspectWidget(QWidget *parent)
    : QWidget(parent),
      m_propertyBrowser(new VariablePropertyBrowser(this)),
      m_variablePropertyManager(new VariablePropertyManager(this))
{
    setWindowTitle("Variable Inspector");
    setWindowFlags(
        Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint
    );

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(m_propertyBrowser);
    setLayout(main_layout);

    pybind11::list my_list;
    my_list.append(1);
    my_list.append(3.14);
    my_list.append("hello");
    my_list.append(pybind11::bool_(true));

    auto float_property = python::CreatePythonProperty(m_variablePropertyManager, "test", pybind11::float_(3.14));
    auto list_property = python::CreatePythonProperty(m_variablePropertyManager, "my_list", my_list);
    m_propertyBrowser->addProperty(float_property);
    m_propertyBrowser->addProperty(list_property);

    setMinimumSize(800, 600);
    m_propertyBrowser->setHeaderSectionResizeRatio(0, 2);
    m_propertyBrowser->setHeaderSectionResizeRatio(1, 2);
    m_propertyBrowser->setHeaderSectionResizeRatio(2, 1);
}

VariablePropertyBrowser *VariableInspectWidget::propertyBrowser() const
{
    return m_propertyBrowser;
}
} // namespace gui
} // namespace xequation