#include "variable_inspect_widget.h"
#include "value_model_view/value_item.h"
#include "value_model_view/value_item_builder.h"

#include <QVBoxLayout>
#include <core/equation_manager.h>
#include <core/equation_signals_manager.h>

namespace xequation
{
namespace gui
{

VariableInspectWidget::VariableInspectWidget(const EquationManager* manager, QWidget *parent) : QWidget(parent), model_(nullptr), view_(nullptr), manager_(manager)
{
    SetupUI();
    SetupConnections();
}

void VariableInspectWidget::SetupUI()
{
    setWindowTitle("Variable Inspector");
    setWindowFlags(
        Qt::Window | Qt::WindowMinimizeButtonHint | Qt::WindowMaximizeButtonHint | Qt::WindowCloseButtonHint
    );
    setContextMenuPolicy(Qt::CustomContextMenu);

    view_ = new ValueTreeView(this);
    model_ = new ValueTreeModel(view_);

    view_->SetValueModel(model_);
    view_->SetHeaderSectionResizeRatio(0, 1);
    view_->SetHeaderSectionResizeRatio(1, 3);
    view_->SetHeaderSectionResizeRatio(2, 1);

    QVBoxLayout *main_layout = new QVBoxLayout(this);
    main_layout->addWidget(view_);
    setLayout(main_layout);

    setMinimumSize(800, 600);
}

void VariableInspectWidget::SetupConnections()
{
    connect(
        view_, &ValueTreeView::customContextMenuRequested, this, &VariableInspectWidget::OnContextMenuRequested
    );

    manager_->signals_manager().Connect<EquationEvent::kEquationRemoving>(
        std::bind(&VariableInspectWidget::OnEquationRemoving, this, std::placeholders::_1)
    );

    manager_->signals_manager().Connect<EquationEvent::kEquationUpdated>(
        std::bind(&VariableInspectWidget::OnEquationUpdated, this, std::placeholders::_1, std::placeholders::_2)
    );
}

void VariableInspectWidget::OnCurrentEquationChanged(const Equation *equation)
{
    SetCurrentEquation(equation);
}

void VariableInspectWidget::SetCurrentEquation(const Equation *equation)
{
    if (current_equation_ == equation)
    {
        return;
    }
    current_equation_ = equation;
    model_->Clear();
    if (current_equation_)
    {
        QString name = QString::fromStdString(current_equation_->name());
        if (variable_items_cache_.find(current_equation_->name()) == variable_items_cache_.end())
        {
            Value value = current_equation_->GetValue();
            ValueItem::UniquePtr item;
            if (value.IsNull())
            {
                item = ValueItem::Create(name, QString::fromStdString(current_equation_->message()), "error");
            }
            else
            {
                item = gui::BuilderUtils::CreateValueItem(name, current_equation_->GetValue());
            }
            model_->AddRootItem(item.get());
            variable_items_cache_[current_equation_->name()] = std::move(item);
        }
        else
        {
            ValueItem *item = variable_items_cache_[current_equation_->name()].get();
            model_->AddRootItem(item);
        }
    }
}

void VariableInspectWidget::OnEquationRemoving(const Equation *equation)
{
    if (variable_items_cache_.find(equation->name()) != variable_items_cache_.end())
    {
        variable_items_cache_.erase(equation->name());
    }
    if (equation == current_equation_)
    {
        SetCurrentEquation(nullptr);
    }
}

void VariableInspectWidget::OnEquationUpdated(
    const Equation *equation, bitmask::bitmask<EquationUpdateFlag> change_type
)
{
    if (change_type & EquationUpdateFlag::kValue)
    {
        if (variable_items_cache_.find(equation->name()) != variable_items_cache_.end())
        {
            variable_items_cache_.erase(equation->name());
        }
    }

    if (equation == current_equation_ && change_type & EquationUpdateFlag::kValue)
    {
        current_equation_ = nullptr;
        SetCurrentEquation(equation);
    }
}

void VariableInspectWidget::OnContextMenuRequested(const QPoint& pos)
{

}

void VariableInspectWidget::OnCopyVariableValue()
{

}

void VariableInspectWidget::OnAddVariableToWatch()
{

}

} // namespace gui
} // namespace xequation