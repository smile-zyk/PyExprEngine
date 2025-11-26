#pragma once
#include <QWidget>
#include <QtVariantPropertyManager>

#include "variable_property_browser.h"
#include "variable_property_manager.h"

namespace xequation
{
namespace gui {
class VariableInspectWidget : public QWidget
{
    Q_OBJECT
public:
    explicit VariableInspectWidget(QWidget *parent = nullptr);
    VariablePropertyBrowser *propertyBrowser() const;
private:
    VariablePropertyBrowser *m_propertyBrowser;
    VariablePropertyManager *m_variablePropertyManager;
};
}
}