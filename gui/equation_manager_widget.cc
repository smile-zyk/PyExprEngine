#include "equation_manager_widget.h"
#include "core/equation.h"
#include "qtvariantproperty.h"
#include <QVBoxLayout>

namespace xequation
{
namespace gui
{
EquationManagerWidget::EquationManagerWidget(EquationManager *manager, QWidget *parent)
    : QWidget(parent), manager_(manager)
{
    SetupUI();

    manager_->RegisterEquationAddedCallback([this](const EquationManager *mgr, const std::string &equation_name) {
        AddEquationPropertyItem(equation_name);
    });

    manager_->RegisterEquationRemovingCallback([this](const EquationManager *mgr, const std::string &equation_name) {
        RemoveEquationPropertyItem(equation_name);
    });

    // Initialize existing equations
    for (const auto &equation_name : manager_->GetEquationNames())
    {
        AddEquationPropertyItem(equation_name);
    }
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

void EquationManagerWidget::AddEquationPropertyItem(const std::string &equation_name)
{
    const Equation *equation = manager_->GetEquation(equation_name);
    if (equation)
    {
        EquationPropertyItem *item = new EquationPropertyItem(equation, property_manager_);
        property_browser_->addProperty(item->main_property());
        equation_item_map_.insert(QString::fromStdString(equation_name), item);
    }
}

void EquationManagerWidget::RemoveEquationPropertyItem(const std::string &equation_name)
{
    QString q_equation_name = QString::fromStdString(equation_name);
    if (equation_item_map_.contains(q_equation_name))
    {
        EquationPropertyItem *item = equation_item_map_.value(q_equation_name);
        property_browser_->removeProperty(item->main_property());
        equation_item_map_.remove(q_equation_name);
        delete item;
    }
}

} // namespace gui
} // namespace xequation
