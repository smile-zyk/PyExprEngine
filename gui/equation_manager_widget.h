#pragma once

#include "core/equation_manager.h"
#include <QWidget>
#include <QttreePropertyBrowser>
#include <QtvariantPropertyManager>

namespace xequation
{
class EquationManager;
namespace gui
{
class EquationManagerWidget : public QWidget
{
  public:
    EquationManagerWidget(const EquationManager *manager, QWidget *parent);

  private:
    void SetupUI();
    void OnEquationAdded(const Equation* equation);
    void OnEquationRemoving(const Equation* equation);
  
  private:
    QtTreePropertyBrowser* property_browser_;
    QtVariantPropertyManager* property_manager_;
    const EquationManager* manager_; 
};
} // namespace gui
} // namespace xequation