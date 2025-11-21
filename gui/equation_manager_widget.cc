#include "equation_manager_widget.h"
#include <QLayout>

namespace xequation
{
namespace gui
{
EquationManagerWidget::EquationManagerWidget(const EquationManager *manager, QWidget *parent)
    : QWidget(parent), manager_(manager)
{
    SetupUI();
}

void EquationManagerWidget::OnEquationAdded(const Equation* equation)
{

}

void EquationManagerWidget::OnEquationRemoving(const Equation* equation)
{

}

void EquationManagerWidget::SetupUI()
{
    setWindowFlags(
        Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint
    );

    property_browser_ = new QtTreePropertyBrowser(this);
    property_manager_ = new QtVariantPropertyManager(this);
    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(property_browser_);
    setLayout(main_layout);
}

} // namespace gui
} // namespace xequation
