#pragma once

#include <QWidget>
#include <QttreePropertyBrowser>
#include <QtvariantPropertyManager>

#include "core/equation_manager.h"
#include "equation_property_manager.h"

namespace xequation
{
namespace gui
{
class EquationManagerWidget : public QWidget
{
  public:
    EquationManagerWidget(EquationManager *manager, QWidget *parent) : manager_(manager), QWidget(parent) { SetupUI(); }
  private:
    void SetupUI();
    EquationManager *manager_;
    QtTreePropertyBrowser* property_browser_;
    EquationPropertyManager* property_manager_;
};
} // namespace gui
} // namespace xequation