#pragma once


#include <QWidget>
#include <QMainWindow>
#include <memory>

#include "core/equation_manager.h"
#include "equation_manager_widget.h"

class QMenu;
class QAction;
class QTextEdit;
class QLabel;

class DemoWidget : public QMainWindow
{
    Q_OBJECT

public:
    explicit DemoWidget(QWidget *parent = nullptr);

private slots:
    void onOpen();
    void onInsertEquation();
    void onInsertMultiEquations();
    void onShowDependencyGraph();
    void onShowEquationInspector();
    void onShowEquationResultInspector() {}

private:
    void createMenus();
    void createActions();
    
    QMenu *fileMenu;
    QMenu *editMenu;
    QMenu *viewMenu;
    
    QAction *openAction;
    QAction *exitAction;
    QAction *insertEquationAction;
    QAction *insertMultiEquationsAction;
    QAction *dependencyGraphAction;
    QAction *equationInspectorAction;
    QAction *equationResultInspectorAction;

    xequation::gui::EquationManagerWidget* equation_manager_widget_;
    std::unique_ptr<xequation::EquationManager> equation_manager_;
};
