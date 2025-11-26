#include "variable_inspect_widget.h"

#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
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

    QtProperty *test_property = m_variablePropertyManager->addProperty("Test Property");
    test_property->setVisible(true);
    m_variablePropertyManager->setValue(test_property, "0");
    m_variablePropertyManager->setType(test_property, "int");
    m_propertyBrowser->addProperty(test_property);
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