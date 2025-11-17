#pragma once

#include "core/equation.h"
#include "core/equation_manager.h"

#include <QListWidget>

class MockEquationListWidget : public QListWidget
{
    Q_OBJECT
public:
    MockEquationListWidget(xequation::EquationManager* manager, QWidget* parent = nullptr);
    ~MockEquationListWidget() override = default;

    void AddEquation(const std::string& equation_name);
    void RemoveEquation(const std::string& equation_name);
    
private:
    xequation::EquationManager* manager_;
};