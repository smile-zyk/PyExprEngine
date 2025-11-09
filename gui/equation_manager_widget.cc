#include "equation_manager_widget.h"
#include "equation_property_manager.h"
#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
void EquationManagerWidget::SetupUI()
{
    property_browser_ = new QtTreePropertyBrowser(this);
    QVBoxLayout* main_layout = new QVBoxLayout(this);
    main_layout->addWidget(property_browser_);
    setLayout(main_layout);

    property_manager_ = new EquationPropertyManager(this);

    auto names = manager_->GetAllEquationNames();

    for(const auto& name : names)
    {
        auto item = property_manager_->addEquationProperty(manager_->GetEquation(name));
        property_browser_->addProperty(item);
    }
    property_browser_->setHeaderVisible(false);
}
} // namespace gui
} // namespace xequation
