#pragma once

#include "core/equation.h"
#include "core/equation_group.h"
#include "core/equation_manager.h"
#include "core/equation_signals_manager.h"

#include <QListWidget>

class MockEquationListWidget : public QListWidget
{
    Q_OBJECT
public:
    MockEquationListWidget(xequation::EquationManager* manager, QWidget* parent = nullptr);
    ~MockEquationListWidget() override = default;

private:
    void SetupUI();

    void OnEquationGroupAdded(const xequation::EquationGroup* group);
    void OnEquationGroupRemoving(const xequation::EquationGroup* group);

private:
    xequation::EquationManager* manager_;
    QMap<xequation::EquationGroupId, QListWidgetItem*> item_map_;

    xequation::ScopedConnection group_added_connection_;
    xequation::ScopedConnection group_removing_connection_;
};