#pragma once

#include <QWidget>
#include <QttreePropertyBrowser>
#include <QtvariantPropertyManager>

#include "core/equation_manager.h"
#include "equation_property_item.h"
#include "qtvariantproperty.h"

namespace xequation
{
namespace gui
{
class EquationManagerWidget : public QWidget
{
  public:
    EquationManagerWidget(EquationManager *manager, QWidget *parent);
  private:
    void AddEquationPropertyItem(const std::string& equation_name);
    void RemoveEquationPropertyItem(const std::string& equation_name);
  private:
    void SetupUI();
    EquationManager *manager_;
    QtTreePropertyBrowser* property_browser_;
    QtVariantPropertyManager* property_manager_;
    QMap<QString, EquationPropertyItem*> equation_item_map_;
};
} // namespace gui
} // namespace xequation