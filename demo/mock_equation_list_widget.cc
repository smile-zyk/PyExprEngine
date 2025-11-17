#include "mock_equation_list_widget.h"

MockEquationListWidget::MockEquationListWidget(xequation::EquationManager *manager, QWidget *parent)
    : QListWidget(parent), manager_(manager)
{
    manager_ = manager;

    manager_->RegisterEquationAddedCallback(
        [this](const xequation::EquationManager *mgr, const std::string &equation_name) { AddEquation(equation_name); }
    );

    manager_->RegisterEquationRemovingCallback(
        [this](const xequation::EquationManager *mgr, const std::string &equation_name) {
            RemoveEquation(equation_name);
        }
    );

    for(const auto& name : manager_->GetEquationNames())
    {
        AddEquation(name);
    }
}

void MockEquationListWidget::AddEquation(const std::string &equation_name)
{
    addItem(QString::fromStdString("Eqn: " + equation_name));
}

void MockEquationListWidget::RemoveEquation(const std::string &equation_name)
{
    QList<QListWidgetItem *> items = findItems(QString::fromStdString("Eqn: " + equation_name), Qt::MatchExactly);
    for (QListWidgetItem *item : items)
    {
        delete takeItem(row(item));
    }
}
